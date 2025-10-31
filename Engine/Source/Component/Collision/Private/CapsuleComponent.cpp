#include "pch.h"
#include "Component/Collision/Public/CapsuleComponent.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UCapsuleComponent, UShapeComponent)

static void UpdateCapsuleAABB(IBoundingVolume*& InOutVolume, bool& InOutOwns, float InHalfHeight, float InRadius)
{
    const FVector Extents = FVector(InRadius, InRadius, InHalfHeight + InRadius);
    if (InOutVolume == nullptr)
    {
        InOutVolume = new FAABB(-Extents, Extents);
        InOutOwns = true;
        return;
    }
    FAABB* AABB = static_cast<FAABB*>(InOutVolume);
    AABB->Min = FVector(-Extents.X, -Extents.Y, -Extents.Z);
    AABB->Max = FVector(Extents.X, Extents.Y, Extents.Z);
}

UCapsuleComponent::UCapsuleComponent()
{
    UpdateCapsuleAABB(BoundingBox, bOwnsBoundingBox, CapsuleHalfHeight, CapsuleRadius);
}

void UCapsuleComponent::SetCapsuleHalfHeight(float InHalfHeight)
{
    CapsuleHalfHeight = InHalfHeight;
    UpdateCapsuleAABB(BoundingBox, bOwnsBoundingBox, CapsuleHalfHeight, CapsuleRadius);
    MarkAsDirty();
}

void UCapsuleComponent::SetCapsuleRadius(float InRadius)
{
    CapsuleRadius = InRadius;
    UpdateCapsuleAABB(BoundingBox, bOwnsBoundingBox, CapsuleHalfHeight, CapsuleRadius);
    MarkAsDirty();
}

void UCapsuleComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        float HalfHeight = GetCapsuleHalfHeight();
        float Radius = GetCapsuleRadius();

        FJsonSerializer::ReadArrayFloat(InOutHandle, "CapsuleHalfHeight", HalfHeight, GetCapsuleHalfHeight());
        FJsonSerializer::ReadArrayFloat(InOutHandle, "CapsuleRadius", Radius, GetCapsuleRadius());

        SetCapsuleHalfHeight(HalfHeight);
        SetCapsuleRadius(Radius);
    }
    else
    {
        JSON H = json::Array(); 
        JSON R = json::Array(); 
        H.append(static_cast<double>(GetCapsuleHalfHeight()));
        R.append(static_cast<double>(GetCapsuleRadius()));
        InOutHandle["CapsuleHalfHeight"] = H;
        InOutHandle["CapsuleRadius"] = R;
    }
}

UObject* UCapsuleComponent::Duplicate()
{
    UCapsuleComponent* Duplicated = Cast<UCapsuleComponent>(Super::Duplicate());
    Duplicated->SetCapsuleHalfHeight(CapsuleHalfHeight);
    Duplicated->SetCapsuleRadius(CapsuleRadius);
    return Duplicated;
}

void UCapsuleComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}