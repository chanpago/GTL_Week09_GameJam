#include "pch.h"
#include "Component/Collision/Public/BoxComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/UI/Widget/Collision/Public/BoxComponentWidget.h"

IMPLEMENT_CLASS(UBoxComponent, UShapeComponent)

UBoxComponent::UBoxComponent()
{
    BoundingBox = new FAABB(-BoxExtent, BoxExtent);
    bOwnsBoundingBox = true;
    SetShapeColor(GetDefaultWireColor());
}

void UBoxComponent::SetBoxExtent(const FVector& InExtent)
{
    BoxExtent = InExtent;

    if (bOwnsBoundingBox && BoundingBox)
    {
        FAABB* AABB = static_cast<FAABB*>(BoundingBox);
        AABB->Min = FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
        AABB->Max = FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
        MarkAsDirty();
    }
}

FColor UBoxComponent::GetDefaultWireColor() const
{
    // Green 계열
    return FColor(0, 255, 0, 255);
}

void UBoxComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FVector Extent = GetBoxExtent();
        FJsonSerializer::ReadVector(InOutHandle, "BoxExtent", Extent, GetBoxExtent());
        SetBoxExtent(Extent);
    }
    else
    {
        InOutHandle["BoxExtent"] = FJsonSerializer::VectorToJson(GetBoxExtent());
    }
}

UObject* UBoxComponent::Duplicate()
{
    UBoxComponent* Duplicated = Cast<UBoxComponent>(Super::Duplicate());
    Duplicated->SetBoxExtent(BoxExtent);
    return Duplicated;
}
UClass* UBoxComponent::GetSpecificWidgetClass() const
{
    return UBoxComponentWidget::StaticClass();
}
void UBoxComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}
