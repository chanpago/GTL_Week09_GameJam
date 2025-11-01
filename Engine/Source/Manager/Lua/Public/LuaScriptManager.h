#pragma once
#include "sol/sol.hpp"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

// Forward declaration to avoid circular dependency
class ULuaScriptComponent;

class FLuaScriptManager
{
public:
    // Singleton access
    static FLuaScriptManager& GetInstance();

    // Lifecycle management
    void StartUp();
    void ShutDown();

    // Called every frame
    void Tick(float deltaTime);

    // For LuaComponent to register/unregister themselves
    void RegisterComponent(ULuaScriptComponent* component);
    void UnregisterComponent(ULuaScriptComponent* component);

    // Loads a script file, caches it, and returns the result
    sol::load_result LoadScript(const std::string& filePath);

    // Creates a Lua table from script file
    sol::table CreateLuaTable(const FString& ScriptName);

    // Hot-reloading support
    void HotReloadLuaScript();
    void ReloadScript(const std::string& filePath);

    // Provides direct access to the Lua state if needed
    sol::state& GetLuaState() { return *LuaState; }

private:
    // Private constructor for Singleton
    FLuaScriptManager();
    ~FLuaScriptManager();

    // Delete copy/move constructors and assignments
    FLuaScriptManager(const FLuaScriptManager&) = delete;
    FLuaScriptManager& operator=(const FLuaScriptManager&) = delete;
    FLuaScriptManager(FLuaScriptManager&&) = delete;
    FLuaScriptManager& operator=(FLuaScriptManager&&) = delete;

    // Binds C++ types and functions to Lua
    void BindTypes();

    std::unique_ptr<sol::state> LuaState;
    TArray<ULuaScriptComponent*> ActiveComponents;
    bool bIsStartedUp = false;

    // Cache for loaded script files with file modification time
    struct FLuaScriptCacheInfo
    {
        sol::object ScriptTable;  // Can be a table OR a factory function
        std::filesystem::file_time_type LastWriteTime;
        std::filesystem::path ResolvedPath;
    };
    std::unordered_map<FString, FLuaScriptCacheInfo> ScriptCache;
};

