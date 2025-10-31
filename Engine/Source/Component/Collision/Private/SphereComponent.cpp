#include "pch.h"
#include "Component/Collision/Public/SphereComponent.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(USphereComponent, UShapeComponent)

USphereComponent::USphereComponent()
{
    const FVector Extents = FVector(SphereRadius, SphereRadius, SphereRadius);
    BoundingBox = new FAABB(-Extents, Extents);
    bOwnsBoundingBox = true;
}

void USphereComponent::SetSphereRadius(float InRadius)
{
    SphereRadius = InRadius;

    if (bOwnsBoundingBox && BoundingBox)
    {
        const FVector Extents = FVector(SphereRadius, SphereRadius, SphereRadius);
        FAABB* AABB = static_cast<FAABB*>(BoundingBox);
        AABB->Min = FVector(-Extents.X, -Extents.Y, -Extents.Z);
        AABB->Max = FVector(Extents.X, Extents.Y, Extents.Z);
        MarkAsDirty();
    }
}

void USphereComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        float Radius = GetSphereRadius();
        FJsonSerializer::ReadArrayFloat(InOutHandle, "SphereRadius", Radius, GetSphereRadius());
        SetSphereRadius(Radius);
    }
    else
    {
        JSON RadiusArray = json::Array();
        RadiusArray.append(static_cast<double>(GetSphereRadius()));
        InOutHandle["SphereRadius"] = RadiusArray;
    }
}

UObject* USphereComponent::Duplicate()
{
    USphereComponent* Duplicated = Cast<USphereComponent>(Super::Duplicate());
    Duplicated->SetSphereRadius(SphereRadius);
    return Duplicated;
}

void USphereComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}