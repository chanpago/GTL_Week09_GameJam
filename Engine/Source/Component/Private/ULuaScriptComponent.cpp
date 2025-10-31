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
    
    if (ScriptName.empty())
    {
        FString ClassName = GetOwner() ? GetOwner()->GetClass()->GetName().ToString() : FString("Actor");
        ScriptName = "Scripts/DefaultLevel/" + ClassName + ".lua";
    }
    
    // Load script first
    LoadScript();
    
    // Register with LuaScriptManager
    FLuaScriptManager::GetInstance().RegisterComponent(this);
    std::cout << "[DEBUG] ULuaScriptComponent::BeginPlay: Registered with LuaScriptManager" << std::endl;
    
    if (SelfTable.valid() && SelfTable["BeginPlay"].valid())
    {
        ActivateFunction("BeginPlay");
    }
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    
    std::cout << "[DEBUG] ULuaScriptComponent::TickComponent: DeltaTime = " << DeltaTime << std::endl;
    
    if (SelfTable.valid() && SelfTable["Tick"].valid())
    {
        std::cout << "[DEBUG] ULuaScriptComponent::TickComponent: Calling Lua Tick" << std::endl;
        ActivateFunction("Tick", DeltaTime);
    }
    else
    {
        std::cout << "[DEBUG] ULuaScriptComponent::TickComponent: SelfTable or Tick function not valid" << std::endl;
    }
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
    std::cout << "[DEBUG] ULuaScriptComponent::EndPlay: Unregistered from LuaScriptManager" << std::endl;
}

bool ULuaScriptComponent::LoadScript()
{
    if (ScriptName.empty())
    {
        FString ClassName = GetOwner() ? GetOwner()->GetClass()->GetName().ToString() : FString("Actor");
        ScriptName = "Scripts/DefaultLevel/" + ClassName + ".lua";
        std::cout << "[DEBUG] LoadScript: ScriptName = " << ScriptName << std::endl;
    }
    else
    {
        std::cout << "[DEBUG] LoadScript: Using existing ScriptName = " << ScriptName << std::endl;
    }
    
    // Load from LuaScriptManager
    SelfTable = FLuaScriptManager::GetInstance().CreateLuaTable(ScriptName);
    
    std::cout << "[DEBUG] LoadScript: SelfTable.valid() = " << SelfTable.valid() << std::endl;
    
    if (!SelfTable.valid())
    {
        std::cout << "[DEBUG] LoadScript: Failed to load script!" << std::endl;
        return false;
    }
    
    // Bind self.this to the owning Actor
    AActor* Owner = GetOwner();
    if (Owner)
    {
        SelfTable["this"] = Owner;
        SelfTable["Name"] = Owner->GetName().ToString();
        std::cout << "[DEBUG] LoadScript: Bound self.this and self.Name" << std::endl;
    }
    
    return true;
}
