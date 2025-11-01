#pragma once
#include "Render/UI/Widget/Public/Widget.h"
class USphereComponent;

UCLASS()
class USphereComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(USphereComponentWidget, UWidget)
public:
    void Initialize() override {}
    void Update() override;
    void RenderWidget() override;
private:
    USphereComponent* SphereComponent = nullptr;
};