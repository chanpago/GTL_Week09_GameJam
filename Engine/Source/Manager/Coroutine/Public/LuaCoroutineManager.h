#pragma once
#include "sol/sol.hpp"
#include <vector>
#include <memory>

// Forward declaration
class ULuaScriptComponent;

class FLuaCoroutineManager
{
public:
    // Singleton access
    static FLuaCoroutineManager& GetInstance();

    // Lifecycle management
    void StartUp();
    void ShutDown();

    // Called every frame
    void Tick(float deltaTime);

    // Coroutine management
    int StartCoroutine(sol::table scriptTable, const std::string& functionName, ULuaScriptComponent* ownerComponent = nullptr);
    void StopCoroutine(int coroutineID);
    void StopAllCoroutines();
    bool IsCoroutineRunning(int coroutineID) const;

private:
    // Private constructor for Singleton
    FLuaCoroutineManager();
    ~FLuaCoroutineManager();

    // Delete copy/move constructors and assignments
    FLuaCoroutineManager(const FLuaCoroutineManager&) = delete;
    FLuaCoroutineManager& operator=(const FLuaCoroutineManager&) = delete;
    FLuaCoroutineManager(FLuaCoroutineManager&&) = delete;
    FLuaCoroutineManager& operator=(FLuaCoroutineManager&&) = delete;

    // Bind coroutine functions to Lua
    void BindCoroutineFunctions();

    // Coroutine state
    enum class ECoroutineState
    {
        Running,
        WaitingForTime,
        WaitingForCondition,
        Dead
    };

    // Coroutine information
    struct FCoroutineInfo
    {
        int ID;
        sol::thread Thread;                 // Must keep the thread alive!
        sol::coroutine Coroutine;
        sol::table OwnerTable;              // The 'self' table (SelfTable from component)
        ECoroutineState State;
        float WaitTime;                     // For wait(seconds)
        sol::function WaitCondition;        // For wait_until(condition)

        FCoroutineInfo(int InID, sol::thread InThread, sol::coroutine InCo, sol::table InOwner)
            : ID(InID)
            , Thread(std::move(InThread))
            , Coroutine(std::move(InCo))
            , OwnerTable(std::move(InOwner))
            , State(ECoroutineState::Running)
            , WaitTime(0.0f)
        {}
    };

    std::vector<FCoroutineInfo> ActiveCoroutines;
    int NextCoroutineID = 1;
    bool bIsStartedUp = false;

    // Helper to find current running coroutine by thread (for wait functions)
    FCoroutineInfo* GetCurrentCoroutineByThread(lua_State* L);
};
