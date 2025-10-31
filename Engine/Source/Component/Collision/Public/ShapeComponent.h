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

    FColor GetShapeColor() const { return ShapeColor; }
    void SetShapeColor(const FColor& InColor) { ShapeColor = InColor; }

    bool ShouldDrawOnlyIfSelected() const { return bDrawOnlyIfSelected; }
    void SetDrawOnlyIfSelected(bool bInDrawOnlyIfSelected) { bDrawOnlyIfSelected = bInDrawOnlyIfSelected; }

    virtual UObject* Duplicate() override;

protected:
    virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    // TODO(SDM) - FColor 만들기
    FColor ShapeColor = FColor(255, 255, 255, 51);
    bool bDrawOnlyIfSelected = false;
};