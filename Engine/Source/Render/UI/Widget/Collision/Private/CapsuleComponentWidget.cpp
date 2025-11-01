#include "pch.h"
#include "Render/UI/Widget/Collision/Public/CapsuleComponentWidget.h"
#include "Component/Collision/Public/CapsuleComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_CLASS(UCapsuleComponentWidget, UWidget)

void UCapsuleComponentWidget::Update()
{
    UActorComponent* Selected = GEditor->GetEditorModule()->GetSelectedComponent();
    CapsuleComponent = Cast<UCapsuleComponent>(Selected);
}

void UCapsuleComponentWidget::RenderWidget()
{
    if (!CapsuleComponent) return;

    ImGui::Separator();

    bool GenerateOverlap = CapsuleComponent->bGenerateOverlapEvents;
    if (ImGui::Checkbox("Generate Overlap Events", &GenerateOverlap))
    {
        CapsuleComponent->bGenerateOverlapEvents = GenerateOverlap;
    }

    bool GenerateHit = CapsuleComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("Generate Hit Events", &GenerateHit))
    {
        CapsuleComponent->bGenerateHitEvents = GenerateHit;
    }

    bool BlockComponent = CapsuleComponent->bBlockComponent;
    if (ImGui::Checkbox("Block Component", &BlockComponent))
    {
        CapsuleComponent->bBlockComponent = BlockComponent;
    }

    float HalfHeight = CapsuleComponent->GetCapsuleHalfHeight();
    float Radius = CapsuleComponent->GetCapsuleRadius();

    if (ImGui::DragFloat("Capsule Half Height", &HalfHeight, 0.1f, 0.0f, 100000.0f))
    {
        CapsuleComponent->SetCapsuleHalfHeight(HalfHeight);
    }
    if (ImGui::DragFloat("Capsule Radius", &Radius, 0.1f, 0.0f, 100000.0f))
    {
        CapsuleComponent->SetCapsuleRadius(Radius);
    }

    FColor ShapeColor = CapsuleComponent->GetShapeColor();
    float Color01[4] = { ShapeColor.R / 255.f, ShapeColor.G / 255.f, ShapeColor.B / 255.f, ShapeColor.A / 255.f };
    if (ImGui::ColorEdit4("Shape Color", Color01, ImGuiColorEditFlags_NoInputs))
    {
        FColor NewColor(
            static_cast<uint8>(Color01[0] * 255.f + 0.5f),
            static_cast<uint8>(Color01[1] * 255.f + 0.5f),
            static_cast<uint8>(Color01[2] * 255.f + 0.5f),
            static_cast<uint8>(Color01[3] * 255.f + 0.5f));
        CapsuleComponent->SetShapeColor(NewColor);
    }

    bool DrawOnlyIfSelected = CapsuleComponent->ShouldDrawOnlyIfSelected();
    if (ImGui::Checkbox("Draw Only If Selected", &DrawOnlyIfSelected))
    {
        CapsuleComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }

}