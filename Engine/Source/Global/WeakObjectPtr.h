#pragma once
//#include "Render/UI/Window/Public/ConsoleWindow.h"

template<typename T>
class TWeakObjectPtr
{
public:
    TWeakObjectPtr() : ObjectIndex(InValidIndex), SerialNumber(0) {}
    TWeakObjectPtr(T* InObject) { Set(InObject); }

    void Set(T* InObject);
    bool IsValid() const;
    T* Get() const;
    void Reset();    

    T* operator->() { return Get(); }
    T& operator*() const { return *Get(); }
    bool operator==(const TWeakObjectPtr<T>& Other) const;
    bool operator!=(const TWeakObjectPtr<T>& Other) const;

private:
    uint32 ObjectIndex;
    uint32 SerialNumber;

    // nullptr인 경우 InValidIndex
    static constexpr uint32 InValidIndex = std::numeric_limits<uint32>::max();
};

template <typename T>
void TWeakObjectPtr<T>::Set(T* InObject)
{
    // 유효 객체의 인덱스와 시리얼번호 등록
    if (InObject)
    {
        ObjectIndex = InObject->GetInternalIndex();
        SerialNumber = GetUObjectSlotArray()[ObjectIndex].SerialNumber;
    }
    // nullptr의 경우 초기화
    else
    {
        ObjectIndex = InValidIndex;
        SerialNumber = 0;
    }
    
}

template <typename T>
bool TWeakObjectPtr<T>::IsValid() const
{
    const auto& Slots = GetUObjectSlotArray();
    const auto& Objects = GetUObjectArray();

    // nullptr이거나 현재 포인터의 Index와 Slot의 크기가 일치하지 않으면 유효하지 않음
    if (ObjectIndex >= Slots.size() || ObjectIndex == InValidIndex)
    {
        //UE_LOG("TWeakObjectPtr : Index Out of Range or Empty");
        return false;
    }
    
    const FObjectSlot& Slot = Slots[ObjectIndex];

    // Slot 검사
    if (!Slot.bIsAlive || Slot.SerialNumber != SerialNumber)
    {
        //UE_LOG("TWeakObjectPtr : Dead or SerialNumber mismatch");
        return false;
    }

    return Objects[ObjectIndex] != nullptr;
}

template <typename T>
T* TWeakObjectPtr<T>::Get() const
{
    if (!IsValid())
    {
        return nullptr;
    }

    return static_cast<T*>(GetUObjectArray()[ObjectIndex]);
}

template <typename T>
void TWeakObjectPtr<T>::Reset()
{
    ObjectIndex = InValidIndex;
    SerialNumber = 0;
}

template <typename T>
bool TWeakObjectPtr<T>::operator==(const TWeakObjectPtr<T>& Other) const
{
    return (ObjectIndex == Other.ObjectIndex) && (SerialNumber == Other.SerialNumber);
}

template <typename T>
bool TWeakObjectPtr<T>::operator!=(const TWeakObjectPtr<T>& Other) const
{
    return !(*this == Other);
}
