#include "pch.h"

#include "Source/Manager/Lua/Public/LuaScriptManager.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Actor/Public/Actor.h"
#include "Global/Vector.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/Coroutine/Public/LuaCoroutineManager.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <system_error>

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
        addCandidate(base, candidates);

        fs::path search = base;
        for (int depth = 0; depth < 5 && search.has_parent_path(); ++depth)
        {
            search = search.parent_path();
            addCandidate(search, candidates);
        }

        for (const fs::path& candidate : candidates)
        {
            if (!candidate.empty() && fs::exists(candidate, ec) && !ec)
            {
                return candidate;
            }
        }

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

    std::cout << "[Shutdown] FLuaScriptManager destroyed safely." << std::endl;
}

void FLuaScriptManager::StartUp()
{
    LuaState = std::make_unique<sol::state>();

    // Open standard libraries (io, string, math, coroutine, etc.)
    LuaState->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::coroutine);

    std::cout << "LuaManager started." << std::endl;

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

    std::cout << "[Shutdown] FLuaScriptManager shutting down..." << std::endl;

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

    std::cout << "[Shutdown] FLuaScriptManager shut down successfully." << std::endl;
}

void FLuaScriptManager::Tick(float deltaTime)
{
    // Hot reload check (optional)
    HotReloadLuaScript();

    // Update coroutines
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
        std::cerr << "Failed to load script: " << resolvedPath << "\nError: " << err.what() << std::endl;
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

    fs::path resolvedPath = ResolveLuaScriptPath(ScriptName);
    std::error_code ec;

    if (!fs::exists(resolvedPath, ec) || ec)
    {
        std::cerr << "[ERROR] CreateLuaTable: Script file does not exist: " << resolvedPath << std::endl;
        ScriptCache.erase(ScriptName);
        return sol::table();
    }

    auto nowWriteTime = fs::last_write_time(resolvedPath, ec);
    if (ec)
    {
        std::cerr << "[ERROR] CreateLuaTable: Failed to get file timestamp for " << resolvedPath << " (" << ec.message() << ")" << std::endl;
        return sol::table();
    }

    auto it = ScriptCache.find(ScriptName);
    if (it == ScriptCache.end() || it->second.LastWriteTime != nowWriteTime)
    {
        sol::protected_function_result result = LuaState->script_file(resolvedPath.string());

        if (!result.valid())
        {
            sol::error err = result;
            std::cerr << "[ERROR] Lua script execution failed (" << ScriptName << "): " << err.what() << std::endl;
            return sol::table();
        }

        sol::object returnValue = result.get<sol::object>();

        // Script can return either a table (old pattern) or a factory function (new pattern)
        if (!returnValue.is<sol::table>() && !returnValue.is<sol::function>())
        {
            std::cerr << "[ERROR] CreateLuaTable: Script must return a table or factory function." << std::endl;
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
        // Call the factory function to create a new instance
        sol::function factory = scriptResult.as<sol::function>();
        sol::protected_function_result callResult = factory();

        if (!callResult.valid())
        {
            sol::error err = callResult;
            std::cerr << "[ERROR] Factory function failed for " << ScriptName << ": " << err.what() << std::endl;
            return sol::table();
        }

        if (!callResult.get<sol::object>().is<sol::table>())
        {
            std::cerr << "[ERROR] Factory function did not return a table for " << ScriptName << std::endl;
            return sol::table();
        }

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

    std::cerr << "[ERROR] Script did not return a table or factory function: " << ScriptName << std::endl;
    return sol::table();
}

void FLuaScriptManager::HotReloadLuaScript()
{
    namespace fs = std::filesystem;

    TSet<FString> Changed;
    auto CopyCache = ScriptCache; // copy keys and times

    for (auto& [Path, Info] : CopyCache)
    {
        fs::path resolvedPath = Info.ResolvedPath;
        if (resolvedPath.empty())
        {
            resolvedPath = ResolveLuaScriptPath(Path);
        }

        std::error_code ec;
        if (!fs::exists(resolvedPath, ec) || ec)
        {
            ScriptCache.erase(Path);
            Changed.insert(Path);
            continue;
        }

        auto currentWriteTime = fs::last_write_time(resolvedPath, ec);
        if (ec)
        {
            continue;
        }

        if (currentWriteTime != Info.LastWriteTime)
        {
            ScriptCache.erase(Path);
            Changed.insert(Path);
        }
    }

    if (Changed.empty())
    {
        return;
    }

    // Make a copy of ActiveComponents to safely iterate
    // This prevents crashes from dangling pointers that may exist in the list
    TArray<ULuaScriptComponent*> ComponentsCopy = ActiveComponents;

    // Clear the original list - we'll rebuild it with valid components only
    ActiveComponents.clear();

    for (ULuaScriptComponent* Comp : ComponentsCopy)
    {
        // Skip null pointers
        if (!Comp)
        {
            continue;
        }

        // Safely get owner - returns nullptr if access violation occurs
        AActor* Owner = SafeGetOwner(Comp);
        if (!Owner)
        {
            // Component is invalid (dangling pointer or access violation)
            continue;
        }

        // Safely check if actor has begun play - returns false if access violation occurs
        bool bHasBegunPlay = SafeHasBegunPlay(Owner);
        if (!bHasBegunPlay)
        {
            // Owner is invalid OR hasn't called BeginPlay yet (editor-only actor)
            // Either way, we don't want to hot-reload this component
            continue;
        }

        // Component and Owner are valid and have begun play
        // Add back to active list (rebuilds the list with only verified valid components)
        ActiveComponents.push_back(Comp);

        const FString& ScriptName = Comp->GetScriptName();
        if (ScriptName.empty()) // 경로가 비어있으면 건너뛴다
        {
            continue;
        }

        if (Changed.count(Comp->GetScriptName()) > 0)
        {
            if (AActor* Owner = Comp->GetOwner())
            {
                std::cout << "[HOT RELOAD] Reloading script: " << Comp->GetScriptName() << std::endl;

                bool bBindSuccess = Owner->BindSelfLuaProperties();
                if (bBindSuccess)
                {
                    sol::table& LuaTable = Comp->GetLuaSelfTable();
                    std::cout << "[HOT RELOAD] Script reloaded successfully!" << std::endl;

                    // Call BeginPlay to reinitialize the script state
                    if (LuaTable.valid() && LuaTable["BeginPlay"].valid())
                    {
                        std::cout << "[HOT RELOAD] Calling BeginPlay for: " << Comp->GetScriptName() << std::endl;
                        Comp->ActivateFunction(FString("BeginPlay"));
                    }
                }
                else
                {
                    std::cout << "[HOT RELOAD] Failed to reload script: " << Comp->GetScriptName() << std::endl;
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

    // --- AActor Binding ---
    LuaState->new_usertype<AActor>("AActor",
        sol::no_constructor,
        "ActorLocation", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
        "Location", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
        "SetActorLocation", &AActor::SetActorLocation,
        "GetActorLocation", &AActor::GetActorLocation,
        "GetName", [](const AActor& actor) { return actor.GetName().ToString(); },
        "GetUUID", &AActor::GetUUID,
        "UUID", sol::property([](AActor& actor) { return actor.GetUUID(); }),
        "PrintLocation", &AActor::PrintLocation
    );

    // --- Global Functions ---
    LuaState->set_function("Print", [](const std::string& message) {
        std::cout << "[LUA] " << message << std::endl;
    });
}
