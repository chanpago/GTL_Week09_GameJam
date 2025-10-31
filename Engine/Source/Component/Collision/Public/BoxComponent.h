#pragma once
#include "Component/Collision/Public/ShapeComponent.h"
#include "Global/Vector.h"
#include "Physics/Public/AABB.h"

UCLASS()
class UBoxComponent : public UShapeComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UBoxComponent, UShapeComponent)

public:
    UBoxComponent();

    const FVector& GetBoxExtent() const { return BoxExtent; }
    void SetBoxExtent(const FVector& InExtent);

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    virtual UObject* Duplicate() override;

protected:
    void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    FVector BoxExtent = FVector(50.f, 50.f, 50.f);
};