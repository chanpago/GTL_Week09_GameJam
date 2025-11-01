#include "pch.h"
#include "Render/UI/Widget/Collision/Public/SphereComponentWidget.h"
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_CLASS(USphereComponentWidget, UWidget)

void USphereComponentWidget::Update()
{
    UActorComponent* Selected = GEditor->GetEditorModule()->GetSelectedComponent();
    SphereComponent = Cast<USphereComponent>(Selected);
}

void USphereComponentWidget::RenderWidget()
{
    if (!SphereComponent) return;

    ImGui::Separator();

    bool GenerateOverlap = SphereComponent->bGenerateOverlapEvents;
    if (ImGui::Checkbox("Generate Overlap Events", &GenerateOverlap))
    {
        SphereComponent->bGenerateOverlapEvents = GenerateOverlap;
    }

    bool GenerateHit = SphereComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("Generate Hit Events", &GenerateHit))
    {
        SphereComponent->bGenerateHitEvents = GenerateHit;
    }

    bool BlockComponent = SphereComponent->bBlockComponent;
    if (ImGui::Checkbox("Block Component", &BlockComponent))
    {
        SphereComponent->bBlockComponent = BlockComponent;
    }

    float Radius = SphereComponent->GetSphereRadius();
    if (ImGui::DragFloat("Sphere Radius", &Radius, 0.1f, 0.0f, 100000.0f))
    {
        SphereComponent->SetSphereRadius(Radius);
    }

    FColor ShapeColor = SphereComponent->GetShapeColor();
    float Color01[4] = { ShapeColor.R / 255.f, ShapeColor.G / 255.f, ShapeColor.B / 255.f, ShapeColor.A / 255.f };
    if (ImGui::ColorEdit4("Shape Color", Color01, ImGuiColorEditFlags_NoInputs))
    {
        FColor NewColor(
            static_cast<uint8>(Color01[0] * 255.f + 0.5f),
            static_cast<uint8>(Color01[1] * 255.f + 0.5f),
            static_cast<uint8>(Color01[2] * 255.f + 0.5f),
            static_cast<uint8>(Color01[3] * 255.f + 0.5f));
        SphereComponent->SetShapeColor(NewColor);
    }

    bool DrawOnlyIfSelected = SphereComponent->ShouldDrawOnlyIfSelected();
    if (ImGui::Checkbox("Draw Only If Selected", &DrawOnlyIfSelected))
    {
        SphereComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }

}