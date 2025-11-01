#include "pch.h"
#include "Source/Manager/Lua/Public/LuaScriptManager.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Actor/Public/Actor.h"
#include "Global/Vector.h"
#include <iostream>
#include <filesystem>


// --- LuaManager Implementation ---

FLuaScriptManager& FLuaScriptManager::GetInstance()
{
    static FLuaScriptManager instance;
    return instance;
}

FLuaScriptManager::FLuaScriptManager()
{
    //LuaState->open_libraries(
    //sol::lib::base,       // Lua를 사용하기 위한 기본 라이브러리 (print, type, pcall 등)
    //// sol::lib::package,    // 모듈 로딩(require) 및 패키지 관리 기능
    //sol::lib::coroutine,  // 코루틴 생성 및 관리 기능 (yield, resume 등)
    //sol::lib::string,     // 문자열 검색, 치환, 포맷팅 등 문자열 처리 기능
    //sol::lib::math,       // 수학 함수 (sin, random, pi 등) 및 상수 제공
    //sol::lib::table,      // 테이블(배열/딕셔너리) 생성 및 조작 기능 (insert, sort 등)
    //// sol::lib::io,         // 파일 읽기/쓰기 등 입출력 관련 기능
    //// sol::lib::os,         // 운영체제 관련 기능 (시간, 날짜, 파일 시스템 접근 등)
    //sol::lib::debug,      // 디버깅 및 introspection 기능 (traceback, getinfo 등)
    //sol::lib::bit32,      // 32비트 정수 대상 비트 연산 기능 (Lua 5.2 이상)
    //// sol::lib::jit,        // LuaJIT의 JIT 컴파일러 제어 기능 (LuaJIT 전용)
    //// sol::lib::ffi,        // 외부 C 함수 및 데이터 구조 접근 기능 (LuaJIT 전용)
    //sol::lib::utf8        // UTF-8 인코딩 문자열 처리 기능 (Lua 5.3 이상)
    //);
}
FLuaScriptManager::~FLuaScriptManager() = default;

void FLuaScriptManager::StartUp()
{
    LuaState = std::make_unique<sol::state>();
    // Open standard libraries (io, string, math, etc.)
    LuaState->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table);

    std::cout << "LuaManager started." << std::endl;

    // Bind all C++ types
    BindTypes();
}

void FLuaScriptManager::ShutDown()
{
    LuaState.reset();
    std::cout << "LuaManager shut down." << std::endl;
}

void FLuaScriptManager::Tick(float deltaTime)
{
    // Hot reload check (optional)
    HotReloadLuaScript();
}

void FLuaScriptManager::RegisterComponent(ULuaScriptComponent* component)
{
    ActiveComponents.push_back(component);
}

void FLuaScriptManager::UnregisterComponent(ULuaScriptComponent* component)
{
    // Remove the component from the active list
    ActiveComponents.erase(
        std::remove(ActiveComponents.begin(), ActiveComponents.end(), component),
        ActiveComponents.end()
    );
}

sol::load_result FLuaScriptManager::LoadScript(const std::string& filePath)
{
    // Load from file (not using cache structure here since it's sol::load_result)
    sol::load_result script = LuaState->load_file(filePath);
    if (!script.valid()) {
        sol::error err = script;
        std::cerr << "Failed to load script: " << filePath << "\nError: " << err.what() << std::endl;
    }
    return script;
}

sol::table FLuaScriptManager::CreateLuaTable(const FString& ScriptName)
{
    namespace fs = std::filesystem;

    std::cout << "[DEBUG] CreateLuaTable: Current working directory = " << fs::current_path() << std::endl;
    std::cout << "[DEBUG] CreateLuaTable: Checking file " << ScriptName << std::endl;
    std::cout << "[DEBUG] CreateLuaTable: File exists? " << fs::exists(ScriptName) << std::endl;
    
    // Check absolute path
    fs::path AbsPath = fs::absolute(ScriptName);
    std::cout << "[DEBUG] CreateLuaTable: Absolute path = " << AbsPath << std::endl;

    std::wstring WideScriptNameExists(ScriptName.begin(), ScriptName.end());
    if (!fs::exists(WideScriptNameExists))
    {
        std::cout << "[DEBUG] CreateLuaTable: File does NOT exist, returning empty table" << std::endl;
        return sol::table();
    }

    // Use wstring for filesystem path (Week8 pattern)
    std::wstring WideScriptName(ScriptName.begin(), ScriptName.end());
    auto NowWriteTime = fs::last_write_time(WideScriptName);

    // Load and cache if missing or outdated
    auto It = ScriptCache.find(ScriptName);
    if (It == ScriptCache.end() || It->second.LastWriteTime != NowWriteTime)
    {
        std::cout << "[DEBUG] CreateLuaTable: Executing script_file()..." << std::endl;
        sol::protected_function_result Result = LuaState->script_file(ScriptName);
        
        std::cout << "[DEBUG] CreateLuaTable: Result.valid() = " << Result.valid() << std::endl;
        
        if (!Result.valid())
        {
            sol::error err = Result;
            std::cerr << "[ERROR] CreateLuaTable: Lua script execution failed: " << err.what() << std::endl;
            return sol::table();
        }

        sol::object ReturnValue = Result.get<sol::object>();
        
        std::cout << "[DEBUG] CreateLuaTable: ReturnValue is table? " << ReturnValue.is<sol::table>() << std::endl;
        
        if (!ReturnValue.is<sol::table>())
        {
            std::cerr << "[ERROR] CreateLuaTable: Script file did not return a table." << std::endl;
            return sol::table();
        }

        FLuaScriptCacheInfo Info;
        Info.ScriptTable = ReturnValue.as<sol::table>();
        Info.LastWriteTime = NowWriteTime;
        ScriptCache[ScriptName] = Info;
    }

    // Instance environment
    sol::table& ScriptClass = ScriptCache[ScriptName].ScriptTable;
    sol::table NewEnv = LuaState->create_table();
    for (auto& pair : ScriptClass)
    {
        NewEnv.set(pair.first, pair.second);
    }
    return NewEnv;
}

void FLuaScriptManager::HotReloadLuaScript()
{
    namespace fs = std::filesystem;

    static int CheckCount = 0;
    CheckCount++;
    if (CheckCount % 60 == 0)  // Every 60 frames
    {
        std::cout << "[DEBUG] HotReload: Checking " << ScriptCache.size() << " scripts (count=" << CheckCount << ")" << std::endl;
        for (auto& [Path, Info] : ScriptCache)
        {
            std::cout << "[DEBUG] HotReload: Cached script: " << Path << std::endl;
        }
    }

    TSet<FString> Changed;
    auto CopyCache = ScriptCache; // copy keys and times
    for (auto& [Path, Info] : CopyCache)
    {
        std::wstring WidePath(Path.begin(), Path.end());
        
        if (CheckCount % 60 == 0)
        {
            std::cout << "[DEBUG] HotReload: Checking path: " << Path << ", exists: " << fs::exists(WidePath) << std::endl;
        }
        
        if (fs::exists(WidePath))
        {
            auto Cur = fs::last_write_time(WidePath);
            
            if (CheckCount % 60 == 0)
            {
                std::cout << "[DEBUG] HotReload: Path: " << Path << std::endl;
                std::cout << "[DEBUG] HotReload: Current time: " << Cur.time_since_epoch().count() << std::endl;
                std::cout << "[DEBUG] HotReload: Cached time: " << Info.LastWriteTime.time_since_epoch().count() << std::endl;
                std::cout << "[DEBUG] HotReload: Equal? " << (Cur == Info.LastWriteTime) << std::endl;
            }
            
            if (Cur != Info.LastWriteTime)
            {
                std::cout << "[HOT RELOAD] Script changed: " << Path << std::endl;
                std::cout << "[HOT RELOAD] Old time: " << Info.LastWriteTime.time_since_epoch().count() << std::endl;
                std::cout << "[HOT RELOAD] New time: " << Cur.time_since_epoch().count() << std::endl;
                ScriptCache.erase(Path);
                Changed.insert(Path);
            }
        }
    }

    if (Changed.empty()) return;

    std::cout << "[HOT RELOAD] Reloading " << Changed.size() << " script(s)" << std::endl;

    // Notify active components (Week8 behavior: re-bind on owner)
    for (ULuaScriptComponent* Comp : ActiveComponents)
    {
        if (!Comp) continue;
        if (Changed.count(Comp->GetScriptName()) > 0)
        {
            std::cout << "[HOT RELOAD] Reloading component script: " << Comp->GetScriptName() << std::endl;
            if (AActor* Owner = Comp->GetOwner())
            {
                bool bBindSuccess = Owner->BindSelfLuaProperties();
                std::cout << "[HOT RELOAD] Lua Script Reloaded: " << Comp->GetScriptName() << std::endl;
                if (bBindSuccess)
                {
                    sol::table& LuaTable = Comp->GetLuaSelfTable();
                    if (LuaTable.valid())
                    {
                        if (LuaTable["BeginPlay"].valid())
                        {
                            std::cout << "[HOT RELOAD] Invoking Lua BeginPlay after reload: " << Comp->GetScriptName() << std::endl;
                            Comp->ActivateFunction(FString("BeginPlay"));
                        }
                    }
                }
            }
        }
    }
}

void FLuaScriptManager::ReloadScript(const std::string& filePath)
{
    std::cout << "Hot-reloading script: " << filePath << std::endl;
    // Note: Now handled by HotReloadLuaScript
}

void FLuaScriptManager::BindTypes()
{
    // Create EngineTypes table
    sol::table EngineTypes = LuaState->create_table("EngineTypes");

    // --- FVector Binding ---
    EngineTypes.new_usertype<FVector>("FVector",
        // Constructors
        sol::constructors<FVector(), FVector(float, float, float)>(),
        // Variables
        "X", &FVector::X,
        "Y", &FVector::Y,
        "Z", &FVector::Z,
        // Functions
        "Length", &FVector::Length,
        "Normalize", &FVector::Normalize,
        "GetNormalized", &FVector::GetNormalized,
        // Operators
        sol::meta_function::addition, sol::resolve<FVector(const FVector&) const>(&FVector::operator+),
        sol::meta_function::subtraction, sol::resolve<FVector(const FVector&) const>(&FVector::operator-),
        sol::meta_function::multiplication, sol::resolve<FVector(float) const>(&FVector::operator*),
        // Static functions for creating vectors like FVector.Zero()
        "Zero", &FVector::ZeroVector,
        "One", &FVector::OneVector
    );

    // --- AActor Binding ---
    LuaState->new_usertype<AActor>("AActor",
        sol::no_constructor,
        "ActorLocation", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
        "SetActorLocation", &AActor::SetActorLocation,
        "GetActorLocation", &AActor::GetActorLocation,
        "GetName", [](const AActor& actor) { return actor.GetName().ToString(); }
    );

    // --- Global Functions ---
    LuaState->set_function("Print", [](const std::string& message) {
        std::cout << "[LUA] " << message << std::endl;
    });
}
