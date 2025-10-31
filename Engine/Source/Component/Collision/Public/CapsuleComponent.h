#pragma once
#include "Component/Collision/Public/ShapeComponent.h"
#include "Physics/Public/AABB.h"

UCLASS()
class UCapsuleComponent : public UShapeComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UCapsuleComponent, UShapeComponent)

public:
    UCapsuleComponent();

    float GetCapsuleHalfHeight() const { return CapsuleHalfHeight; }
    float GetCapsuleRadius() const { return CapsuleRadius; }

    void SetCapsuleHalfHeight(float InHalfHeight);
    void SetCapsuleRadius(float InRadius);

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    virtual UObject* Duplicate() override;

protected:
    void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    float CapsuleHalfHeight = 88.f; 
    float CapsuleRadius = 34.f;
};