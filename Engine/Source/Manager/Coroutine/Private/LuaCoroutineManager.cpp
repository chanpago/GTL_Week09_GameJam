#include "pch.h"

#include "Manager/Coroutine/Public/LuaCoroutineManager.h"
#include "Manager/Lua/Public/LuaScriptManager.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ULuaScriptComponent.h"

#include <iostream>
#include <algorithm>

// Helper function to safely check if an Actor pointer is valid
// Separated to avoid C2712 error (can't use __try with objects that have destructors)
namespace
{
    bool IsActorPointerValid(AActor* actor)
    {
        if (!actor)
        {
            return false;
        }

#ifdef _WIN32
        __try
        {
            // Try to access a member to verify it's not freed memory
            return !actor->IsPendingDestroy();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Access violation - actor is freed memory
            return false;
        }
#else
        return !actor->IsPendingDestroy();
#endif
    }
}

// Singleton instance
FLuaCoroutineManager& FLuaCoroutineManager::GetInstance()
{
    static FLuaCoroutineManager instance;
    return instance;
}

FLuaCoroutineManager::FLuaCoroutineManager()
{
}

FLuaCoroutineManager::~FLuaCoroutineManager()
{
    // Ensure all coroutines are stopped before destruction
    if (!ActiveCoroutines.empty())
    {
        std::cout << "[Coroutine] Destroying manager with " << ActiveCoroutines.size() << " active coroutines" << std::endl;
        StopAllCoroutines();
    }
}

void FLuaCoroutineManager::StartUp()
{
    if (bIsStartedUp)
    {
        return;
    }

    std::cout << "[Coroutine] FLuaCoroutineManager started." << std::endl;

    // Bind coroutine functions to Lua
    BindCoroutineFunctions();

    bIsStartedUp = true;
}

void FLuaCoroutineManager::ShutDown()
{
    StopAllCoroutines();
    std::cout << "[Coroutine] FLuaCoroutineManager shut down." << std::endl;
    bIsStartedUp = false;
}

void FLuaCoroutineManager::Tick(float deltaTime)
{
    if (!bIsStartedUp)
    {
        return;
    }

    // Process all active coroutines
    for (auto it = ActiveCoroutines.begin(); it != ActiveCoroutines.end();)
    {
        FCoroutineInfo& info = *it;

        // Check if OwnerTable is still valid (actor might be destroyed)
        if (!info.OwnerTable.valid())
        {
            std::cout << "[Coroutine] Coroutine " << info.ID << " stopped (owner table invalid)" << std::endl;
            it = ActiveCoroutines.erase(it);
            continue;
        }

        // Check if 'this' (the actor pointer) is still valid
        sol::object thisObj = info.OwnerTable["this"];
        if (!thisObj.valid() || !thisObj.is<AActor*>())
        {
            std::cout << "[Coroutine] Coroutine " << info.ID << " stopped (actor destroyed)" << std::endl;
            it = ActiveCoroutines.erase(it);
            continue;
        }

        // CRITICAL: Check if the Actor pointer is actually valid (not freed)
        AActor* actor = thisObj.as<AActor*>();
        if (!IsActorPointerValid(actor))
        {
            std::cout << "[Coroutine] Coroutine " << info.ID << " stopped (actor freed or pending destroy)" << std::endl;
            it = ActiveCoroutines.erase(it);
            continue;
        }

        // Handle different coroutine states
        switch (info.State)
        {
        case ECoroutineState::WaitingForTime:
            {
                // Decrease wait timer
                info.WaitTime -= deltaTime;
                if (info.WaitTime <= 0.0f)
                {
                    // Timer expired, resume coroutine
                    info.State = ECoroutineState::Running;

                    auto result = info.Coroutine(info.OwnerTable);  // ✅ Pass self
                    if (!result.valid())
                    {
                        sol::error err = result;
                        std::cerr << "[Coroutine] Error resuming coroutine " << info.ID << ": " << err.what() << std::endl;
                        info.State = ECoroutineState::Dead;
                    }
                    else if (info.Coroutine.status() == sol::call_status::ok)
                    {
                        // Coroutine finished
                        info.State = ECoroutineState::Dead;
                    }
                }
                ++it;
            }
            break;

        case ECoroutineState::WaitingForCondition:
            {
                // Check if condition is met
                if (info.WaitCondition.valid())
                {
                    auto conditionResult = info.WaitCondition();
                    if (conditionResult.valid() && conditionResult.get_type() == sol::type::boolean)
                    {
                        bool conditionMet = conditionResult.get<bool>();
                        if (conditionMet)
                        {
                            // Condition met, resume coroutine
                            info.State = ECoroutineState::Running;

                            auto result = info.Coroutine(info.OwnerTable);  // ✅ Pass self
                            if (!result.valid())
                            {
                                sol::error err = result;
                                std::cerr << "[Coroutine] Error resuming coroutine " << info.ID << ": " << err.what() << std::endl;
                                info.State = ECoroutineState::Dead;
                            }
                            else if (info.Coroutine.status() == sol::call_status::ok)
                            {
                                // Coroutine finished
                                info.State = ECoroutineState::Dead;
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "[Coroutine] wait_until condition function must return boolean" << std::endl;
                        info.State = ECoroutineState::Dead;
                    }
                }
                ++it;
            }
            break;

        case ECoroutineState::Running:
            {
                // Resume the coroutine
                auto result = info.Coroutine(info.OwnerTable);  // ✅ Pass self
                if (!result.valid())
                {
                    sol::error err = result;
                    std::cerr << "[Coroutine] Error running coroutine " << info.ID << ": " << err.what() << std::endl;
                    info.State = ECoroutineState::Dead;
                }
                else if (info.Coroutine.status() == sol::call_status::ok)
                {
                    // Coroutine finished
                    info.State = ECoroutineState::Dead;
                }
                ++it;
            }
            break;

        case ECoroutineState::Dead:
            {
                // Remove dead coroutines
                std::cout << "[Coroutine] Coroutine " << info.ID << " finished." << std::endl;
                it = ActiveCoroutines.erase(it);
            }
            break;

        default:
            ++it;
            break;
        }
    }
}

int FLuaCoroutineManager::StartCoroutine(sol::table scriptTable, const std::string& functionName, ULuaScriptComponent* ownerComponent)
{
    if (!bIsStartedUp)
    {
        StartUp();
    }

    // Get the function from the script table
    sol::object funcObj = scriptTable[functionName];
    if (!funcObj.valid() || funcObj.get_type() != sol::type::function)
    {
        std::cerr << "[Coroutine] Function '" << functionName << "' not found or is not a function" << std::endl;
        return -1;
    }

    sol::function func = funcObj.as<sol::function>();

    // Get Lua state
    sol::state& lua = FLuaScriptManager::GetInstance().GetLuaState();

    // Create coroutine thread
    sol::thread coThread = sol::thread::create(lua.lua_state());
    sol::state_view coState = coThread.state();
    sol::coroutine co = sol::coroutine(coState, func);

    int coroutineID = NextCoroutineID++;

    // Add to active coroutines (store thread to keep it alive!)
    ActiveCoroutines.emplace_back(coroutineID, std::move(coThread), std::move(co), scriptTable);

    // Register coroutine with component if provided
    if (ownerComponent)
    {
        ownerComponent->RegisterCoroutine(coroutineID);
    }

    std::cout << "[Coroutine] Started coroutine " << coroutineID << " with function '" << functionName << "'" << std::endl;

    return coroutineID;
}

void FLuaCoroutineManager::StopCoroutine(int coroutineID)
{
    auto it = std::find_if(ActiveCoroutines.begin(), ActiveCoroutines.end(),
        [coroutineID](const FCoroutineInfo& info) { return info.ID == coroutineID; });

    if (it != ActiveCoroutines.end())
    {
        std::cout << "[Coroutine] Stopping coroutine " << coroutineID << std::endl;
        ActiveCoroutines.erase(it);
    }
}

void FLuaCoroutineManager::StopAllCoroutines()
{
    std::cout << "[Coroutine] Stopping all " << ActiveCoroutines.size() << " coroutines" << std::endl;
    ActiveCoroutines.clear();
    NextCoroutineID = 1;
}

bool FLuaCoroutineManager::IsCoroutineRunning(int coroutineID) const
{
    return std::find_if(ActiveCoroutines.begin(), ActiveCoroutines.end(),
        [coroutineID](const FCoroutineInfo& info) { return info.ID == coroutineID; }) != ActiveCoroutines.end();
}

FLuaCoroutineManager::FCoroutineInfo* FLuaCoroutineManager::GetCurrentCoroutineByThread(lua_State* L)
{
    // Find the coroutine by comparing lua_State
    for (auto& info : ActiveCoroutines)
    {
        if (info.Thread.thread_state() == L)
        {
            return &info;
        }
    }

    std::cerr << "[Coroutine] Warning: Could not find coroutine for thread!" << std::endl;
    return nullptr;
}

void FLuaCoroutineManager::BindCoroutineFunctions()
{
    sol::state& lua = FLuaScriptManager::GetInstance().GetLuaState();

    // Bind StartCoroutine
    lua.set_function("StartCoroutine", [](sol::table self, const std::string& functionName) -> int
    {
        // Try to find the owning component
        ULuaScriptComponent* ownerComponent = nullptr;

        sol::object thisObj = self["this"];
        if (thisObj.valid() && thisObj.is<AActor*>())
        {
            AActor* actor = thisObj.as<AActor*>();
            if (actor)
            {
                ownerComponent = actor->GetLuaScriptComponent();
            }
        }

        return FLuaCoroutineManager::GetInstance().StartCoroutine(self, functionName, ownerComponent);
    });

    // Bind StopCoroutine
    lua.set_function("StopCoroutine", [](int coroutineID)
    {
        FLuaCoroutineManager::GetInstance().StopCoroutine(coroutineID);
    });

    // Bind internal wait functions (C++ sets state, Lua does yield)
    lua.set_function("_wait_internal", [](float seconds, sol::this_state s)
    {
        FLuaCoroutineManager& manager = FLuaCoroutineManager::GetInstance();
        FCoroutineInfo* currentCo = manager.GetCurrentCoroutineByThread(s.lua_state());

        if (currentCo)
        {
            currentCo->State = ECoroutineState::WaitingForTime;
            currentCo->WaitTime = seconds;
        }
        else
        {
            std::cerr << "[Coroutine] wait() called outside of coroutine!" << std::endl;
        }
    });

    lua.set_function("_wait_until_internal", [](sol::function condition, sol::this_state s)
    {
        FLuaCoroutineManager& manager = FLuaCoroutineManager::GetInstance();
        FCoroutineInfo* currentCo = manager.GetCurrentCoroutineByThread(s.lua_state());

        if (currentCo)
        {
            currentCo->State = ECoroutineState::WaitingForCondition;
            currentCo->WaitCondition = condition;
        }
        else
        {
            std::cerr << "[Coroutine] wait_until() called outside of coroutine!" << std::endl;
        }
    });

    // Create Lua wrapper functions that call coroutine.yield()
    lua.script(R"(
        function wait(seconds)
            _wait_internal(seconds)
            coroutine.yield()
        end

        function wait_until(condition)
            _wait_until_internal(condition)
            coroutine.yield()
        end
    )");

    std::cout << "[Coroutine] Bound coroutine functions to Lua: StartCoroutine, StopCoroutine, wait, wait_until" << std::endl;
}
