#pragma once
#include "Render/UI/Widget/Public/Widget.h"
class UCapsuleComponent;

UCLASS()
class UCapsuleComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(UCapsuleComponentWidget, UWidget)
public:
    void Initialize() override {}
    void Update() override;
    void RenderWidget() override;
private:
    UCapsuleComponent* CapsuleComponent = nullptr;
};