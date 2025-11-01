#pragma once
#include "Component/Collision/Public/ShapeComponent.h"
#include "Physics/Public/AABB.h"

UCLASS()
class USphereComponent : public UShapeComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(USphereComponent, UShapeComponent)

public:
    USphereComponent();

    float GetSphereRadius() const { return SphereRadius; }
    void SetSphereRadius(float InRadius);
    FColor GetDefaultWireColor() const override;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    virtual UObject* Duplicate() override;
    UClass* GetSpecificWidgetClass() const override;
protected:
    void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    float SphereRadius = 50.f;
};