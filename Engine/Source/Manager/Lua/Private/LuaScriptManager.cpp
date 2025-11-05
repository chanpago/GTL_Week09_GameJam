#include "pch.h"

#include "Source/Manager/Lua/Public/LuaScriptManager.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/MissileActor.h"
#include "Pawn/Public/Pawn.h"
#include "Player/Public/PlayerCharacter.h"
#include "Player/Public/EnemyCharacter.h"
#include "GamePlay/Public/PlayerController.h"
#include "GameMode/Public/GameModeBase.h"
#include "Global/Vector.h"
#include "Global/Quaternion.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/Coroutine/Public/LuaCoroutineManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Level/Public/World.h"
#include "Editor/Public/EditorEngine.h"
#include "Manager/UI/Public/GameUIManager.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"
#include "Render/UI/Widget/Public/Widget.h"
#include "Render/UI/Widget/Public/GameHUDWidget.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <system_error>

#include "GameMode/Public/GameMode.h"

// Helper functions for safe pointer access with SEH
// These functions are separated to avoid C2712 error (can't use __try with objects that have destructors)
namespace
{
    // Safely get owner from component, returns nullptr if access violation occurs
    AActor* SafeGetOwner(ULuaScriptComponent* Comp)
    {
        if (!Comp)
        {
            return nullptr;
        }

#ifdef _WIN32
        AActor* Owner = nullptr;
        __try
        {
            Owner = Comp->GetOwner();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Access violation - component is dangling pointer
            return nullptr;
        }
        return Owner;
#else
        return Comp->GetOwner();
#endif
    }

    // Safely check if actor has begun play, returns false if access violation occurs
    bool SafeHasBegunPlay(AActor* Owner)
    {
        if (!Owner)
        {
            return false;
        }

#ifdef _WIN32
        bool bHasBegunPlay = false;
        __try
        {
            bHasBegunPlay = Owner->HasBegunPlay();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Access violation - owner is dangling pointer
            return false;
        }
        return bHasBegunPlay;
#else
        return Owner->HasBegunPlay();
#endif
    }
}

// Lua Binding Helper Templates
namespace LuaBindingHelpers
{
    // Enum을 문자열로 받는 메서드 바인딩 헬퍼
    template<typename ClassType, typename EnumType, typename MethodType>
    auto BindEnumMethod(MethodType method)
    {
        return [method](ClassType* obj, const std::string& enumName) -> bool {
            auto enumValue = TEnumReflector<EnumType>::FromString(enumName.c_str());
            if (enumValue.has_value()) {
                return (obj->*method)(enumValue.value());
            }
            return false;
        };
    }

    // FName을 문자열로 반환하는 GetName 헬퍼
    template<typename T>
    std::string GetNameAsString(const T& obj)
    {
        return obj.GetName().ToString();
    }

    // UUID를 int로 반환하는 헬퍼
    template<typename T>
    int GetUUIDAsInt(T& obj)
    {
        return obj.GetUUID();
    }
}

namespace
{
    std::filesystem::path ResolveLuaScriptPath(const std::string& ScriptName)
    {
        namespace fs = std::filesystem;

        fs::path inputPath(ScriptName);
        std::error_code ec;

        if (inputPath.is_absolute() && fs::exists(inputPath, ec) && !ec)
        {
            return inputPath;
        }

        fs::path base = UPathManager::GetInstance().GetRootPath();
        if (base.empty())
        {
            base = fs::current_path();
        }

        // DEBUG: 경로 로깅
        UE_LOG("[ResolveLuaScriptPath] Looking for: '%s'", ScriptName.c_str());
        UE_LOG("[ResolveLuaScriptPath] Base path: '%s'", base.string().c_str());

        auto addCandidate = [&](const fs::path& root, std::vector<fs::path>& out)
        {
            if (root.empty())
            {
                return;
            }
            out.emplace_back(root / inputPath);
            out.emplace_back(root / "Engine" / inputPath);
        };

        std::vector<fs::path> candidates;

#ifdef _DEVELOP
        // In development mode, prioritize Engine source folder for hot reload
        fs::path search = base;
        for (int depth = 0; depth < 5 && search.has_parent_path(); ++depth)
        {
            search = search.parent_path();
            addCandidate(search, candidates);
        }
        // Add base path last (Build/Debug) so Engine/Scripts is checked first
        addCandidate(base, candidates);
#else
        // In release mode, use Build folder first
        addCandidate(base, candidates);

        fs::path search = base;
        for (int depth = 0; depth < 5 && search.has_parent_path(); ++depth)
        {
            search = search.parent_path();
            addCandidate(search, candidates);
        }
#endif

        for (const fs::path& candidate : candidates)
        {
            UE_LOG("[ResolveLuaScriptPath] Trying: '%s' - %s",
                   candidate.string().c_str(),
                   fs::exists(candidate, ec) ? "EXISTS" : "NOT FOUND");

            if (!candidate.empty() && fs::exists(candidate, ec) && !ec)
            {
                UE_LOG("[ResolveLuaScriptPath] Found at: '%s'", candidate.string().c_str());
                return candidate;
            }
        }

        UE_LOG_ERROR("[ResolveLuaScriptPath] NOT FOUND, returning default: '%s'", (base / "Engine" / inputPath).string().c_str());
        return base / "Engine" / inputPath;
    }

    //std::filesystem::path ResolveLuaScriptPath(const FString& ScriptName)
    //{
    //    return ResolveLuaScriptPath(std::string(ScriptName));
    //}
}

// --- LuaManager Implementation ---

FLuaScriptManager& FLuaScriptManager::GetInstance()
{
    static FLuaScriptManager instance;
    return instance;
}

FLuaScriptManager::FLuaScriptManager()
{
}

FLuaScriptManager::~FLuaScriptManager()
{
    // Ensure proper cleanup order to prevent Access Violations during shutdown

    // Only cleanup if we haven't already shut down
    if (bIsStartedUp)
    {
        // ShutDown() handles all cleanup properly
        ShutDown();
    }
    else if (LuaState)
    {
        // If somehow LuaState exists but we're not started up, clean it anyway
        ScriptCache.clear();
        ActiveComponents.clear();
        LuaState.reset();
    }
    
    UE_LOG_TERMINAL("[Shutdown] FLuaScriptManager destroyed safely.");
}

void FLuaScriptManager::StartUp()
{
    // Lua 가상 머신생성 
    LuaState = std::make_unique<sol::state>();

    // Lua 표준 라이브러리 로드 (io, string, math, coroutine, etc.)
    LuaState->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::coroutine);
    
    UE_LOG_SYSTEM("LuaManager started.");
    
    // Bind all C++ types
    BindTypes();

    // Start coroutine manager
    FLuaCoroutineManager::GetInstance().StartUp();

    bIsStartedUp = true;
}

void FLuaScriptManager::ShutDown()
{
    if (!bIsStartedUp)
    {
        return; // Already shut down
    }
    
    UE_LOG_TERMINAL("[Shutdown] FLuaScriptManager shutting down...");

    // 1. Shutdown coroutine manager first (stops all coroutines)
    FLuaCoroutineManager::GetInstance().ShutDown();

    // 2. Clear all Lua objects BEFORE destroying LuaState
    //    This is critical - sol::object destructors need valid LuaState
    ScriptCache.clear();
    ActiveComponents.clear();

    // 3. Now it's safe to destroy the Lua state
    LuaState.reset();

    // 4. Mark as shut down
    bIsStartedUp = false;
    
    UE_LOG_TERMINAL("[Shutdown] FLuaScriptManager shut down successfully.");
}

void FLuaScriptManager::Tick(float deltaTime)
{
    // 1. Lua 파일이 수정됐는지 체크 → 수정됐으면 자동 리로드
    HotReloadLuaScript();

    // 2. Lua 코루틴들 업데이트 (비동기 로직 처리)
    FLuaCoroutineManager::GetInstance().Tick(deltaTime);
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
    namespace fs = std::filesystem;

    fs::path resolvedPath = ResolveLuaScriptPath(filePath);
    sol::load_result script = LuaState->load_file(resolvedPath.string());

    if (!script.valid())
    {
        sol::error err = script;
        UE_LOG_ERROR("Failed to load script: %s\nError: %s", resolvedPath.string().c_str(), err.what());
    }

    return script;
}

sol::table FLuaScriptManager::CreateLuaTable(const FString& ScriptName)
{
    if (!bIsStartedUp)
    {
        StartUp();
    }

    namespace fs = std::filesystem;

    // 1. 스크립트 파일 경로 찾기
    fs::path resolvedPath = ResolveLuaScriptPath(ScriptName);
    std::error_code ec;

    if (!fs::exists(resolvedPath, ec) || ec)
    {
        UE_LOG_ERROR("CreateLuaTable: Script file does not exist: %s", resolvedPath.string().c_str());
        ScriptCache.erase(ScriptName);
        return sol::table();
    }

    auto nowWriteTime = fs::last_write_time(resolvedPath, ec);
    if (ec)
    {
        UE_LOG_ERROR("CreateLuaTable: Failed to get file timestamp for %s (%s)", resolvedPath.string().c_str(), ec.message().c_str());
        return sol::table();
    }

    auto it = ScriptCache.find(ScriptName);
    if (it == ScriptCache.end() || it->second.LastWriteTime != nowWriteTime)
    {
        // 3. 스크립트 실행
        sol::protected_function_result result = LuaState->script_file(resolvedPath.string());

        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG_ERROR("Lua script execution failed (%s): %s", ScriptName.c_str(), err.what());
            return sol::table();
        }

        sol::object returnValue = result.get<sol::object>();

        // Script can return either a table (old pattern) or a factory function (new pattern)
        if (!returnValue.is<sol::table>() && !returnValue.is<sol::function>())
        {
            UE_LOG_ERROR("CreateLuaTable: Script must return a table or factory function.");
            return sol::table();
        }

        FLuaScriptCacheInfo info;
        info.ScriptTable = returnValue;  // Store as sol::object (can be table or function)
        info.LastWriteTime = nowWriteTime;
        info.ResolvedPath = fs::canonical(resolvedPath, ec);
        if (ec)
        {
            info.ResolvedPath = resolvedPath;
            ec.clear();
        }
        ScriptCache[ScriptName] = std::move(info);
    }
    else
    {
        std::error_code canonEc;
        fs::path canonicalPath = fs::canonical(resolvedPath, canonEc);
        ScriptCache[ScriptName].ResolvedPath = canonEc ? resolvedPath : canonicalPath;
    }

    // Get the cached script result (could be a table or a factory function)
    sol::object& scriptResult = ScriptCache[ScriptName].ScriptTable;

    // Check if the script returned a function (factory pattern)
    if (scriptResult.is<sol::function>())
    {
        UE_LOG("[CreateLuaTable] Calling factory function for '%s'", ScriptName.c_str());

        // Call the factory function to create a new instance
        sol::function factory = scriptResult.as<sol::function>();
        sol::protected_function_result callResult = factory();

        if (!callResult.valid())
        {
            sol::error err = callResult;
            UE_LOG_ERROR("Factory function failed for %s: %s", ScriptName.c_str(), err.what());
            return sol::table();
        }

        if (!callResult.get<sol::object>().is<sol::table>())
        {
            UE_LOG_ERROR("Factory function did not return a table for %s", ScriptName.c_str());
            return sol::table();
        }

        UE_LOG("[CreateLuaTable] Factory function returned table successfully for '%s'", ScriptName.c_str());
        return callResult.get<sol::table>();
    }
    // Otherwise, it's a table (old pattern) - do shallow copy
    else if (scriptResult.is<sol::table>())
    {
        sol::table scriptClass = scriptResult.as<sol::table>();

        // Create a new environment by copying all members from scriptClass
        sol::table newEnv = LuaState->create_table();
        for (auto& pair : scriptClass)
        {
            newEnv.set(pair.first, pair.second);
        }

        return newEnv;
    }

    UE_LOG_ERROR("Script did not return a table or factory function: %s", ScriptName.c_str());
    return sol::table();
}

void FLuaScriptManager::HotReloadLuaScript()
{
    namespace fs = std::filesystem;

    // 변경된 스크립트 목록
    TSet<FString> Changed;

    // 캐시 복사
    auto CopyCache = ScriptCache; 

    // 캐시된 모든 스크립트 검사
    for (auto& [Path, Info] : CopyCache)
    {
        fs::path resolvedPath = Info.ResolvedPath;
        if (resolvedPath.empty())
        {
            resolvedPath = ResolveLuaScriptPath(Path);
        }

        // 파일이 삭제되었는지 확인
        std::error_code ec;
        if (!fs::exists(resolvedPath, ec) || ec)
        {
            ScriptCache.erase(Path);
            Changed.insert(Path);
            continue;
        }

        // 파일 수정 시간 확인
        auto currentWriteTime = fs::last_write_time(resolvedPath, ec);
        if (ec)
        {
            continue;
        }

        // 수정 시간이 바뀌었다 = 파일이 수정됨!
        if (currentWriteTime != Info.LastWriteTime)
        {
            ScriptCache.erase(Path); // 캐시에서 제거
            Changed.insert(Path);    // 변경 목록에 추가
        }
    }

    if (Changed.empty())  // 변경 없으면 종료
    {
        return;
    }

    // ActiveComponents 복사 (안전한 순회를 위해)
    TArray<ULuaScriptComponent*> ComponentsCopy = ActiveComponents;

    // 원본 리스트 초기화 (유효한 것만 다시 추가할 예정)
    ActiveComponents.clear();

    for (ULuaScriptComponent* Comp : ComponentsCopy)
    {
        // Skip null pointers
        if (!Comp)
        {
            continue;
        }

        // 안전하게 Owner 가져오기 (댕글링 포인터 방지)
        AActor* Owner = SafeGetOwner(Comp);
        if (!Owner)
        {
            continue;
        }

        // BeginPlay 호출되었는지 확인
        bool bHasBegunPlay = SafeHasBegunPlay(Owner);
        if (!bHasBegunPlay)
        {
            // Owner is invalid OR hasn't called BeginPlay yet (editor-only actor)
            // Either way, we don't want to hot-reload this component
            continue;
        }

        // 유효한 컴포넌트만 다시 추가
        ActiveComponents.push_back(Comp);

        const FString& ScriptName = Comp->GetScriptName();
        if (ScriptName.empty()) // 경로가 비어있으면 건너뛴다
        {
            continue;
        }

        // 이 컴포넌트의 스크립트가 변경되었는지 확인
        if (Changed.count(Comp->GetScriptName()) > 0)
        {
            if (AActor* Owner = Comp->GetOwner())
            {
                UE_LOG_SUCCESS("[HOT RELOAD] Reloading script: ");

                // C++와 Lua 연결 다시 설정
                bool bBindSuccess = Owner->BindSelfLuaProperties();
                if (bBindSuccess)
                {
                    sol::table& LuaTable = Comp->GetLuaSelfTable();
                    UE_LOG_SUCCESS("[HOT RELOAD] Script reloaded successfully!");

                    // BeginPlay 다시 호출 (초기화)
                    if (LuaTable.valid() && LuaTable["BeginPlay"].valid())
                    {
                        UE_LOG_SUCCESS("[HOT RELOAD] Calling BeginPlay for:");
                        Comp->ActivateFunction(FString("BeginPlay"));
                    }
                }
                else
                {
                    UE_LOG_ERROR("[HOT RELOAD] Failed to reload script: %s", Comp->GetScriptName().c_str());
                }
            }
        }
    }
}

void FLuaScriptManager::ReloadScript(const std::string& filePath)
{
    UE_LOG_INFO("Hot-reloading script: %s", filePath.c_str());
    // Note: Now handled by HotReloadLuaScript
}

void FLuaScriptManager::BindTypes()
{
    using namespace LuaBindingHelpers;

    // Create EngineTypes table
    sol::table EngineTypes = LuaState->create_table("EngineTypes");

    // --- FVector Binding ---
    EngineTypes.new_usertype<FVector>("FVector",
        sol::call_constructor,
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

    // --- FQuaternion Binding ---
    EngineTypes.new_usertype<FQuaternion>("FQuaternion",
        sol::call_constructor,
        sol::constructors<FQuaternion(), FQuaternion(float, float, float, float)>(),
        // Variables
        "X", &FQuaternion::X,
        "Y", &FQuaternion::Y,
        "Z", &FQuaternion::Z,
        "W", &FQuaternion::W,
        // Static functions
        "Identity", &FQuaternion::Identity,
        "FromEuler", &FQuaternion::FromEuler,
        "FromAxisAngle", &FQuaternion::FromAxisAngle,
        "MakeFromDirection", &FQuaternion::MakeFromDirection,
        // Methods
        "ToEuler", &FQuaternion::ToEuler,
        "Normalize", &FQuaternion::Normalize,
        "Conjugate", &FQuaternion::Conjugate,
        "Inverse", &FQuaternion::Inverse,
        "RotateVector", sol::resolve<FVector(const FVector&) const>(&FQuaternion::RotateVector),
        // Operators
        sol::meta_function::multiplication, &FQuaternion::operator*
    );

    // --- FVector4 Binding ---
    EngineTypes.new_usertype<FVector4>("FVector4",
        sol::call_constructor,
        sol::constructors<FVector4(), FVector4(float, float, float, float)>(),
        // Variables
        "X", &FVector4::X,
        "Y", &FVector4::Y,
        "Z", &FVector4::Z,
        "W", &FVector4::W,
        // Operators
        sol::meta_function::addition, sol::resolve<FVector4(const FVector4&) const>(&FVector4::operator+),
        sol::meta_function::subtraction, sol::resolve<FVector4(const FVector4&) const>(&FVector4::operator-),
        sol::meta_function::multiplication, sol::resolve<FVector4(float) const>(&FVector4::operator*),
        sol::meta_function::division, &FVector4::operator/,
        // Static functions
        "Zero", &FVector4::ZeroVector,
        "One", &FVector4::OneVector
    );

    // Expose common types globally for convenience
    (*LuaState)["FVector"] = EngineTypes["FVector"];
    (*LuaState)["FVector4"] = EngineTypes["FVector4"];
    (*LuaState)["FQuaternion"] = EngineTypes["FQuaternion"];

    // --- FRotator Binding (as global) ---
    LuaState->new_usertype<FRotator>("FRotator",
        sol::call_constructor,
        sol::constructors<FRotator(), FRotator(float, float, float)>(),
        // Variables (Pitch, Yaw, Roll in degrees)
        "Pitch", &FRotator::Pitch,
        "Yaw", &FRotator::Yaw,
        "Roll", &FRotator::Roll,
        // Methods
        "Quaternion", &FRotator::Quaternion,
        "GetNormalized", &FRotator::GetNormalized,
        "IsZero", &FRotator::IsZero,
        // Static methods
        "NormalizeAxis", &FRotator::NormalizeAxis
    );

    // --- AActor Binding ---
    LuaState->new_usertype<AActor>("AActor",
        sol::no_constructor,
        // Location
        "ActorLocation", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
        "Location", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
        "SetActorLocation", &AActor::SetActorLocation,
        "GetActorLocation", &AActor::GetActorLocation,
        // Rotation
        "ActorRotation", sol::property(&AActor::GetActorRotation, &AActor::SetActorRotation),
        "SetActorRotation", &AActor::SetActorRotation,
        "GetActorRotation", &AActor::GetActorRotation,
        // Scale
        "ActorScale3D", sol::property(&AActor::GetActorScale3D, &AActor::SetActorScale3D),
        "SetActorScale3D", &AActor::SetActorScale3D,
        "GetActorScale3D", &AActor::GetActorScale3D,
        // Time Dilation
        "CustomTimeDilation", sol::property(&AActor::GetCustomTimeDilation, &AActor::SetCustomTimeDilation),
        "SetCustomTimeDilation", &AActor::SetCustomTimeDilation,
        "GetCustomTimeDilation", &AActor::GetCustomTimeDilation,
        "GetActorDeltaTime", &AActor::GetActorDeltaTime,
        // Components
        "GetOwnedComponents", &AActor::GetOwnedComponents,
        "GetRootComponent", &AActor::GetRootComponent,
        "GetStaticMeshComponent", &AActor::GetStaticMeshComponent,
        // Lua Script
        "SetUseScript", &AActor::SetUseScript,
        "InitLuaScriptComponent", &AActor::InitLuaScriptComponent,
        "GetLuaScriptComponent", &AActor::GetLuaScriptComponent,
        // Damage system - C++ 전용으로 변경, Lua에서 호출 불가
        // "TakeDamage"는 C++에서만 사용 (Release 빌드 안정성)
        // Misc
        "GetName", &GetNameAsString<AActor>,
        "GetUUID", &AActor::GetUUID,
        "UUID", sol::property(&GetUUIDAsInt<AActor>),
        "PrintLocation", &AActor::PrintLocation,
        "SetIsPendingDestroy", &AActor::SetIsPendingDestroy,
        "IsPendingDestroy", &AActor::IsPendingDestroy,
        "GetLuaScriptComponent", &AActor::GetLuaScriptComponent,
        "SetUseScript", &AActor::SetUseScript
    );

    // --- Global Functions ---
    LuaState->set_function("Print", [](const std::string& message) {
        UE_LOG("[LUA] %s", message.c_str());
    });

    // --- InputManager Binding ---
    LuaState->set_function("GetInputManager", []() -> UInputManager* {
        return &UInputManager::GetInstance();
    });

    LuaState->new_usertype<UInputManager>("UInputManager",
        sol::no_constructor,
        // Keyboard input (템플릿 헬퍼 사용)
        "IsKeyDown", BindEnumMethod<UInputManager, EKeyInput>(&UInputManager::IsKeyDown),
        "IsKeyPressed", BindEnumMethod<UInputManager, EKeyInput>(&UInputManager::IsKeyPressed),
        "IsKeyReleased", BindEnumMethod<UInputManager, EKeyInput>(&UInputManager::IsKeyReleased),
        // Mouse input
        "GetMousePosition", &UInputManager::GetMousePosition,
        "GetMouseDelta", &UInputManager::GetMouseDelta,
        "GetMouseWheelDelta", &UInputManager::GetMouseWheelDelta,
        "GetMouseNDCPosition", &UInputManager::GetMouseNDCPosition,
        "IsWindowFocused", &UInputManager::IsWindowFocused
    );

    // --- UWorld Binding ---
    LuaState->set_function("GetWorld", []() {
        return GWorld;
    });

    LuaState->new_usertype<UWorld>("UWorld",
        sol::no_constructor,
        // Actor spawning (by class name string)
        "SpawnActor", [](UWorld* world, const std::string& className) {
            UClass* ActorClass = UClass::FindClass(className);
            if (!ActorClass) {
                UE_LOG_ERROR("[Lua] Failed to find class: %s", className.c_str());
                return (AActor*)nullptr;
            }
            return world->SpawnActor(ActorClass);
        },
        // Actor destruction (deferred via pending destroy)
        "DestroyActor", &UWorld::DestroyActor,
        // Get PlayerCameraManager
        "GetPlayerCameraManager", &UWorld::GetPlayerCameraManager
    );

    // --- UActorComponent Binding ---
    LuaState->new_usertype<UActorComponent>("UActorComponent",
        sol::no_constructor,
        "GetName", &GetNameAsString<UActorComponent>,
        "GetOwner", &UActorComponent::GetOwner,
        "CanEverTick", &UActorComponent::CanEverTick,
        "SetCanEverTick", &UActorComponent::SetCanEverTick,
        "IsEditorOnly", &UActorComponent::IsEditorOnly,
        "SetIsEditorOnly", &UActorComponent::SetIsEditorOnly,
        "IsVisualizationComponent", &UActorComponent::IsVisualizationComponent
    );

    // --- USceneComponent Binding (inherits from UActorComponent) ---
    LuaState->new_usertype<USceneComponent>("USceneComponent",
        sol::no_constructor,
        sol::base_classes, sol::bases<UActorComponent>(),
        "GetName", &GetNameAsString<USceneComponent>,
        "GetOwner", &USceneComponent::GetOwner,
        // Relative transforms (local to parent)
        "GetRelativeLocation", &USceneComponent::GetRelativeLocation,
        "GetRelativeRotation", &USceneComponent::GetRelativeRotation,
        "GetRelativeScale3D", &USceneComponent::GetRelativeScale3D,
        "SetRelativeLocation", &USceneComponent::SetRelativeLocation,
        "SetRelativeRotation", &USceneComponent::SetRelativeRotation,
        "SetRelativeScale3D", &USceneComponent::SetRelativeScale3D,
        // World transforms (absolute)
        "GetWorldLocation", &USceneComponent::GetWorldLocation,
        "GetWorldRotation", &USceneComponent::GetWorldRotation,  // Returns FVector (Euler angles)
        "GetWorldRotationAsQuaternion", &USceneComponent::GetWorldRotationAsQuaternion,
        "GetWorldScale3D", &USceneComponent::GetWorldScale3D
    );

    // --- UPrimitiveComponent Binding (inherits from USceneComponent) ---
    LuaState->new_usertype<UPrimitiveComponent>("UPrimitiveComponent",
        sol::no_constructor,
        sol::base_classes, sol::bases<USceneComponent>(),
        "GetName", &GetNameAsString<UPrimitiveComponent>,
        "GetOwner", &UPrimitiveComponent::GetOwner,
        // Visibility
        "IsVisible", &UPrimitiveComponent::IsVisible,
        "SetVisibility", &UPrimitiveComponent::SetVisibility,
        // Picking
        "CanPick", &UPrimitiveComponent::CanPick,
        "SetCanPick", &UPrimitiveComponent::SetCanPick,
        // Color
        "GetColor", &UPrimitiveComponent::GetColor,
        "SetColor", &UPrimitiveComponent::SetColor,
        // Collision/Overlap
        "IsOverlappingComponent", &UPrimitiveComponent::IsOverlappingComponent,
        "IsOverlappingActor", &UPrimitiveComponent::IsOverlappingActor,
        // Collision settings
        "bGenerateOverlapEvents", &UPrimitiveComponent::bGenerateOverlapEvents,
        "bGenerateHitEvents", &UPrimitiveComponent::bGenerateHitEvents
    );

    // --- FHitResult Binding ---
    LuaState->new_usertype<FHitResult>("FHitResult",
        "Component", &FHitResult::Component,
        "Actor", &FHitResult::Actor,
        "ImpactPoint", &FHitResult::ImpactPoint,
        "ImpactNormal", &FHitResult::ImpactNormal,
        "Distance", &FHitResult::Distance
        );

    // --- UMeshComponent Binding (inherits from UPrimitiveComponent) ---
    LuaState->new_usertype<UMeshComponent>("UMeshComponent",
        sol::no_constructor,
        sol::base_classes, sol::bases<UPrimitiveComponent>(),
        "GetName", &GetNameAsString<UMeshComponent>,
        "GetOwner", &UMeshComponent::GetOwner
    );

    // --- UStaticMeshComponent Binding (inherits from UMeshComponent) ---
    LuaState->new_usertype<UStaticMeshComponent>("UStaticMeshComponent",
        sol::no_constructor,
        sol::base_classes, sol::bases<UMeshComponent>(),
        "GetName", &GetNameAsString<UStaticMeshComponent>,
        "GetOwner", &UStaticMeshComponent::GetOwner,
        // Static Mesh
        "GetStaticMesh", &UStaticMeshComponent::GetStaticMesh,
        "SetStaticMesh", &UStaticMeshComponent::SetStaticMeshFromString, // Lua uses string version
        "SetStaticMeshFromFName", &UStaticMeshComponent::SetStaticMesh,  // Keep FName version accessible
        // Material
        "GetMaterial", &UStaticMeshComponent::GetMaterial,
        "SetMaterial", &UStaticMeshComponent::SetMaterial,
        // Scroll
        "EnableScroll", &UStaticMeshComponent::EnableScroll,
        "DisableScroll", &UStaticMeshComponent::DisableScroll,
        "IsScrollEnabled", &UStaticMeshComponent::IsScrollEnabled,
        "GetElapsedTime", &UStaticMeshComponent::GetElapsedTime,
        "SetElapsedTime", &UStaticMeshComponent::SetElapsedTime,
        // Normal Map
        "EnableNormalMap", &UStaticMeshComponent::EnableNormalMap,
        "DisableNormalMap", &UStaticMeshComponent::DisableNormalMap,
        "IsNormalMapEnabled", &UStaticMeshComponent::IsNormalMapEnabled
    );

    // --- APawn Binding (inherits from AActor) ---
    LuaState->new_usertype<APawn>("APawn",
        sol::no_constructor,
        sol::base_classes, sol::bases<AActor>(),
        "GetName", &GetNameAsString<APawn>,
        "GetController", &APawn::GetController,
        "IsControlled", &APawn::IsControlled,
        "PossessedBy", &APawn::PossessedBy,
        "UnPossessed", &APawn::UnPossessed
    );

    // --- APlayerCharacter Binding (inherits from APawn) ---
    LuaState->new_usertype<APlayerCharacter>("APlayerCharacter",
        sol::no_constructor,
        sol::base_classes, sol::bases<APawn>(),
        "GetName", &GetNameAsString<APlayerCharacter>,
        "MoveForward", &APlayerCharacter::MoveForward,
        "MoveRight", &APlayerCharacter::MoveRight,
        "OnBeginOverlap", &APlayerCharacter::OnBeginOverlap,
        "OnEndOverlap", &APlayerCharacter::OnEndOverlap,
        "OnHit", &APlayerCharacter::OnHit
    );

    // --- AEnemyCharacter Binding (inherits from APawn) ---
    LuaState->new_usertype<AEnemyCharacter>("AEnemyCharacter",
        sol::no_constructor,
        sol::base_classes, sol::bases<APawn>(),
        "GetName", &GetNameAsString<AEnemyCharacter>,
        "MoveForward", &AEnemyCharacter::MoveForward,
        "MoveRight", &AEnemyCharacter::MoveRight,
        "OnBeginOverlap", &AEnemyCharacter::OnBeginOverlap,
        "OnEndOverlap", &AEnemyCharacter::OnEndOverlap,
        "OnHit", &AEnemyCharacter::OnHit
    );

    // --- APlayerController Binding (inherits from AActor) ---
    LuaState->new_usertype<APlayerController>("APlayerController",
        sol::no_constructor,
        sol::base_classes, sol::bases<AActor>(),
        "GetName", &GetNameAsString<APlayerController>,
        "Possess", &APlayerController::Possess,
        "UnPossess", &APlayerController::UnPossess,
        "GetControlledActor", &APlayerController::GetControlledActor,
        "GetControlledPawn", &APlayerController::GetControlledPawn,
        "GetPlayerCameraManager", &APlayerController::GetPlayerCameraManager
    );

    // --- AGameModeBase Binding (inherits from AActor) ---
    LuaState->new_usertype<AGameModeBase>("AGameModeBase",
        sol::no_constructor,
        sol::base_classes, sol::bases<AActor>(),
        "GetName", &GetNameAsString<AGameModeBase>,
        "GetPlayerController", &AGameModeBase::GetPlayerController,
        "GetDefaultPawnClass", &AGameModeBase::GetDefaultPawnClass,
        "SetDefaultPawnClass", &AGameModeBase::SetDefaultPawnClass,
        "InitGame", &AGameModeBase::InitGame,
        "StartPlay", &AGameModeBase::StartPlay
    );

    // --- AGameMode Binding (inherits from AGameModeBase) ---
    LuaState->new_usertype<AGameMode>("AGameMode",
        sol::no_constructor,
        sol::base_classes, sol::bases<AGameModeBase>(),
        "ChangeState", &AGameMode::ChangeState,
        "SpawnPlayerCharacter", &AGameMode::SpawnPlayerCharacter,
        "InitializeEnemyPool", &AGameMode::InitializeEnemyPool,
        "SpawnEnemies", &AGameMode::SpawnEnemies,
        "GetPlayerController", &AGameMode::GetPlayerController,
        "BroadCastPlayerLocation", &AGameMode::BroadCastPlayerLocation
        );

    // --- UWorld Binding Extension (add GameMode support) ---
    LuaState->set_function("GetGameMode", []() {
        if (GWorld) {
            return GWorld->GetGameMode();
        }
        return (AGameMode*)nullptr;
    });

    // --- Cast Functions (safe type casting) ---
    LuaState->set_function("Cast_APlayerCharacter", [](AActor* Actor) -> APlayerCharacter* {
        if (!Actor) return nullptr;
        return dynamic_cast<APlayerCharacter*>(Actor);
    });

    LuaState->set_function("Cast_AEnemyCharacter", [](AActor* Actor) -> AEnemyCharacter* {
        if (!Actor) return nullptr;
        return dynamic_cast<AEnemyCharacter*>(Actor);
    });

    // --- UWidget Binding (base class for all widgets) ---
    LuaState->new_usertype<UWidget>("UWidget",
        sol::no_constructor,
        sol::base_classes, sol::bases<UObject>()
    );

    // --- GameUIManager Binding ---
    LuaState->new_usertype<UGameUIManager>("UGameUIManager",
        sol::no_constructor,
        sol::base_classes, sol::bases<UObject>(),
        "GetHUDWidget", &UGameUIManager::GetHUDWidget
    );

    LuaState->set_function("GetGameUIManager", []() {
        return &UGameUIManager::GetInstance();
    });

    // --- UTimeManager Binding ---
    LuaState->new_usertype<UTimeManager>("UTimeManager",
        sol::no_constructor,
        sol::base_classes, sol::bases<UObject>(),
        // Basic Time Functions
        "GetGameTime", &UTimeManager::GetGameTime,
        "GetDeltaTime", &UTimeManager::GetDeltaTime,
        "GetRawDeltaTime", &UTimeManager::GetRawDeltaTime,
        "GetFPS", &UTimeManager::GetFPS,
        "IsPaused", &UTimeManager::IsPaused,
        "PauseGame", &UTimeManager::PauseGame,
        "ResumeGame", &UTimeManager::ResumeGame,

        // Time Dilation System
        "SetGlobalTimeDilation", &UTimeManager::SetGlobalTimeDilation,
        "GetGlobalTimeDilation", &UTimeManager::GetGlobalTimeDilation,
        "ResetTimeDilation", &UTimeManager::ResetTimeDilation,

        // Hit Stop System - Support both with and without parameter
        "StartHitStop", sol::overload(
            [](UTimeManager& Manager) { Manager.StartHitStop(); },
            [](UTimeManager& Manager, float Duration) { Manager.StartHitStop(Duration); }
        ),
        "IsHitStopActive", &UTimeManager::IsHitStopActive,

        // Slow Motion System
        "StartSlowMotion", &UTimeManager::StartSlowMotion,
        "StopSlowMotion", &UTimeManager::StopSlowMotion,
        "IsSlowMotionActive", &UTimeManager::IsSlowMotionActive
    );

    LuaState->set_function("GetTimeManager", []() {
        return &UTimeManager::GetInstance();
    });

    // --- EKeyInput Enum Binding ---
    LuaState->new_enum<EKeyInput>("EKeyInput",
        {
            // Movement keys
            {"W", EKeyInput::W},
            {"A", EKeyInput::A},
            {"S", EKeyInput::S},
            {"D", EKeyInput::D},
            {"Q", EKeyInput::Q},
            {"E", EKeyInput::E},
            {"R", EKeyInput::R},
            {"F", EKeyInput::F},
            {"P", EKeyInput::P},
            {"G", EKeyInput::G},
            {"C", EKeyInput::C},
            {"T", EKeyInput::T},
            // Arrow keys
            {"Up", EKeyInput::Up},
            {"Down", EKeyInput::Down},
            {"Left", EKeyInput::Left},
            {"Right", EKeyInput::Right},
            // Action keys
            {"Space", EKeyInput::Space},
            {"Enter", EKeyInput::Enter},
            {"Esc", EKeyInput::Esc},
            {"Tab", EKeyInput::Tab},
            {"Shift", EKeyInput::Shift},
            {"Ctrl", EKeyInput::Ctrl},
            {"Alt", EKeyInput::Alt},
            // Number keys
            {"Num0", EKeyInput::Num0},
            {"Num1", EKeyInput::Num1},
            {"Num2", EKeyInput::Num2},
            {"Num3", EKeyInput::Num3},
            {"Num4", EKeyInput::Num4},
            {"Num5", EKeyInput::Num5},
            {"Num6", EKeyInput::Num6},
            {"Num7", EKeyInput::Num7},
            {"Num8", EKeyInput::Num8},
            {"Num9", EKeyInput::Num9},
            // Mouse
            {"MouseLeft", EKeyInput::MouseLeft},
            {"MouseRight", EKeyInput::MouseRight},
            {"MouseMiddle", EKeyInput::MouseMiddle},
            // Other
            {"Backtick", EKeyInput::Backtick},
            {"F1", EKeyInput::F1},
            {"F2", EKeyInput::F2},
            {"F3", EKeyInput::F3},
            {"F4", EKeyInput::F4},
            {"Backspace", EKeyInput::Backspace},
            {"Delete", EKeyInput::Delete}
        }
    );

    // --- ECameraEaseType Enum Binding ---
    LuaState->new_enum<ECameraEaseType>("ECameraEaseType",
        {
            {"Linear", ECameraEaseType::Linear},
            {"EaseIn", ECameraEaseType::EaseIn},
            {"EaseOut", ECameraEaseType::EaseOut},
            {"EaseInOut", ECameraEaseType::EaseInOut},
            {"SmoothStep", ECameraEaseType::SmoothStep},
            {"SmootherStep", ECameraEaseType::SmootherStep},
            {"Bezier", ECameraEaseType::Bezier}
        }
    );

    // --- APlayerCameraManager Binding ---
    LuaState->new_usertype<APlayerCameraManager>("APlayerCameraManager",
        sol::no_constructor,
        sol::base_classes, sol::bases<AActor>(),
        "StartCameraShake", &APlayerCameraManager::StartCameraShake,
        "GetFadeAmount", &APlayerCameraManager::GetFadeAmount,
        "GetLetterBoxAlpha", &APlayerCameraManager::GetLetterBoxAlpha,

        // Camera Transition System
        "StartTransitionToLocation", [](APlayerCameraManager* Mgr,
                                        const FVector& TargetLoc,
                                        const FRotator& TargetRot,
                                        float Duration,
                                        ECameraEaseType EaseType) {
            if (Mgr) {
                Mgr->StartTransitionToLocation(TargetLoc, TargetRot, Duration, EaseType, -1.0f);
            }
        },
        "StartTransitionToActor", [](APlayerCameraManager* Mgr,
                                     AActor* TargetActor,
                                     float Duration,
                                     ECameraEaseType EaseType,
                                     const FVector& Offset) {
            if (Mgr && TargetActor) {
                Mgr->StartTransitionToActor(TargetActor, Duration, EaseType, Offset);
            }
        },
        "StopCameraTransition", &APlayerCameraManager::StopCameraTransition,
        "IsCameraTransitioning", &APlayerCameraManager::IsCameraTransitioning,
        "GetTransitionProgress", &APlayerCameraManager::GetTransitionProgress,
        "SetTransitionBezierControlPoints", [](APlayerCameraManager* Mgr,
                                                float P1x, float P1y, float P2x, float P2y) {
            if (Mgr) {
                float CP[4] = { P1x, P1y, P2x, P2y };
                Mgr->SetTransitionBezierControlPoints(CP);
            }
        }
    );

    // --- UGameHUDWidget Binding ---
    LuaState->new_usertype<UGameHUDWidget>("UGameHUDWidget",
        sol::no_constructor,
        sol::base_classes, sol::bases<UWidget, UObject>(),
        "TakeDamage", &UGameHUDWidget::TakeDamage,
        "SetHealth", &UGameHUDWidget::SetHealth,
        "GetHealth", &UGameHUDWidget::GetHealth,
        "IsPlayerDead", &UGameHUDWidget::IsPlayerDead,
        "AddScore", &UGameHUDWidget::AddScore,
        "GetScore", &UGameHUDWidget::GetScore
    );

    // --- ULuaScriptComponent Binding (inherits from UActorComponent) ---
    LuaState->new_usertype<ULuaScriptComponent>("ULuaScriptComponent",
        sol::no_constructor,
        sol::base_classes, sol::bases<UActorComponent>(),
        "GetOwner", &ULuaScriptComponent::GetOwner,
        "LoadScript", &ULuaScriptComponent::LoadScript,
        "SetScriptName", [](ULuaScriptComponent* Comp, const std::string& ScriptName) {
            if (Comp) {
                Comp->SetScriptName(FString(ScriptName.c_str()));
            }
        },
        "ActivateFunction", &ULuaScriptComponent::ActivateFunctionLua,
        "GetLuaSelfTable", [](ULuaScriptComponent* Comp) -> sol::table
        {
            if (!Comp)
            {
                return sol::table();
            }
            return Comp->GetLuaSelfTable();
        }
        );
}
