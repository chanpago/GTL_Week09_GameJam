#pragma once
#include "Render/UI/Widget/Public/Widget.h"
class UBoxComponent;

UCLASS()
class UBoxComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(UBoxComponentWidget, UWidget)

public:
    UBoxComponentWidget() = default;
    ~UBoxComponentWidget() override = default;

    void Initialize() override {}
    void Update() override;
    void RenderWidget() override;

private:
    UBoxComponent* BoxComponent = nullptr;
};