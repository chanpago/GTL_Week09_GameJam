#include "pch.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Manager/Lua/Public/LuaScriptManager.h"
#include "Actor/Public/Actor.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(ULuaScriptComponent, UActorComponent)

ULuaScriptComponent::ULuaScriptComponent()
{
    bCanEverTick = true;
}

ULuaScriptComponent::~ULuaScriptComponent()
{
}

void ULuaScriptComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    
    if (bInIsLoading)
    {
        FJsonSerializer::ReadString(InOutHandle, "ScriptName", ScriptName, "");
    }
    else
    {
        InOutHandle["ScriptName"] = ScriptName;
    }
}

UObject* ULuaScriptComponent::Duplicate()
{
    ULuaScriptComponent* NewComponent = Cast<ULuaScriptComponent>(Super::Duplicate());
    if (!NewComponent)
        return nullptr;
    
    NewComponent->ScriptName = ScriptName;
    NewComponent->SelfTable = SelfTable;
    
    return NewComponent;
}

void ULuaScriptComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}

void ULuaScriptComponent::BeginPlay()
{
    Super::BeginPlay();

    // Safety check: Ensure we have a valid owner
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        std::cerr << "[ERROR] ULuaScriptComponent::BeginPlay: No owner!" << std::endl;
        return;
    }

    if (ScriptName.empty())
    {
        FString ClassName = Owner->GetClass()->GetName().ToString();
        ScriptName = "Scripts/DefaultLevel/" + ClassName + ".lua";
    }

    // Load script first
    if (!LoadScript())
    {
        std::cerr << "[ERROR] ULuaScriptComponent::BeginPlay: Failed to load script: " << ScriptName << std::endl;
        return;
    }

    // Register with LuaScriptManager for hot-reload tracking
    FLuaScriptManager::GetInstance().RegisterComponent(this);

    if (SelfTable.valid() && SelfTable["BeginPlay"].valid())
    {
        ActivateFunction("BeginPlay");
    }
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // Safety check: Only tick if we have a valid owner
    AActor* Owner = GetOwner();
    if (!Owner || !SelfTable.valid() || !SelfTable["Tick"].valid())
    {
        return;
    }

    // Additional safety: Only tick if the actor has begun play
    // This prevents ticking during shutdown
    if (!Owner->HasBegunPlay())
    {
        return;
    }

    ActivateFunction("Tick", DeltaTime);
}

void ULuaScriptComponent::EndPlay()
{
    Super::EndPlay();

    if (SelfTable.valid() && SelfTable["EndPlay"].valid())
    {
        ActivateFunction("EndPlay");
    }

    // Unregister from LuaScriptManager
    FLuaScriptManager::GetInstance().UnregisterComponent(this);

    // Invalidate the Lua table to prevent any further access
    // This ensures that if Tick is somehow called after EndPlay,
    // it will be safely ignored
    SelfTable = sol::nil;
}

void ULuaScriptComponent::SetScriptName(const FString& InScriptName)
{
    ScriptName = InScriptName;
    SelfTable = sol::table();
}

bool ULuaScriptComponent::LoadScript()
{
    if (ScriptName.empty())
    {
        FString ClassName = GetOwner() ? GetOwner()->GetClass()->GetName().ToString() : FString("Actor");
        ScriptName = "Scripts/DefaultLevel/" + ClassName + ".lua";
    }

    // Load from LuaScriptManager
    SelfTable = FLuaScriptManager::GetInstance().CreateLuaTable(ScriptName);

    if (!SelfTable.valid())
    {
        std::cerr << "[ERROR] LoadScript: Failed to load script: " << ScriptName << std::endl;
        return false;
    }

    // Bind self.this to the owning Actor
    AActor* Owner = GetOwner();
    if (Owner)
    {
        SelfTable["this"] = Owner;
        SelfTable["Name"] = Owner->GetName().ToString();
    }

    return true;
}
