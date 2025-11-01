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
    virtual FColor GetDefaultWireColor() const { return FColor(200, 200, 200, 128); }
    virtual UObject* Duplicate() override;

protected:
    virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

protected:
    FColor ShapeColor = FColor(255, 255, 255, 255);
    bool bDrawOnlyIfSelected = false;
};