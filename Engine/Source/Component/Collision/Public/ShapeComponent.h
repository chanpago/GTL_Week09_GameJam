#pragma once
#include "Component/Public/PrimitiveComponent.h"

UCLASS()
class UShapeComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)

public:
    UShapeComponent();

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    FVector4 GetShapeColor() const { return ShapeColor; }
    void SetShapeColor(const FVector4& InColor) { ShapeColor = InColor; }

    bool ShouldDrawOnlyIfSelected() const { return bDrawOnlyIfSelected; }
    void SetDrawOnlyIfSelected(bool bInDrawOnlyIfSelected) { bDrawOnlyIfSelected = bInDrawOnlyIfSelected; }

    virtual UObject* Duplicate() override;

protected:
    virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    // TODO(SDM) - FColor 만들기
    FVector4 ShapeColor = FVector4{ 1.f, 1.f, 1.f, 0.2f };
    bool bDrawOnlyIfSelected = false;
};