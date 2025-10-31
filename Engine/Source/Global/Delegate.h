#pragma once
#include <functional>
#include "Global/Types.h"
#include "Global/WeakObjectPtr.h"
//#include "Render/UI/Window/Public/ConsoleWindow.h"


template<typename... Args>
class TDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;

    // 람다, 일반 함수 등록
    void Add(const HandlerType& Handler);

    // 클래스 멤버 함수 등록
    template<typename T>
    void AddDynamic(T* Instance, void(T::*Func)(Args...));

    // 등록된 모든 핸들러 실행
    void BroadCast(Args... InArgs);

    void Clear();
    uint32 Num() const;

private:
    TArray<HandlerType> Handlers;
};

template <typename ... Args>
void TDelegate<Args...>::Add(const HandlerType& Handler)
{
    Handlers.emplace_back(Handler);
}

template <typename ... Args>
template <typename T>
void TDelegate<Args...>::AddDynamic(T* Instance, void(T::* Func)(Args...))
{
    if (Instance == nullptr)
    {
        //UE_LOG("AddDynamic : Instance nullptr");
        return;
    }

    TWeakObjectPtr<T> WeakInstance(Instance);

    HandlerType BoundHandler = [WeakInstance, Func](Args... InArgs)
    {
        if (WeakInstance.IsValid())
        {
            (WeakInstance.Get()->*Func)(InArgs...);
        }
        else
        {
            //UE_LOG("Deleagate Skip Dead UObject");
        }
    };

    Handlers.emplace_back(std::move(BoundHandler));
}

template <typename ... Args>
void TDelegate<Args...>::BroadCast(Args... InArgs)
{
    for (const auto& Handler : Handlers)
    {
        if (Handler)
        {
            Handler(InArgs...);
        }
    }
    
}

template <typename ... Args>
void TDelegate<Args...>::Clear()
{
    Handlers.clear();
}

template <typename ... Args>
uint32 TDelegate<Args...>::Num() const
{
    return Handlers.size();
}
