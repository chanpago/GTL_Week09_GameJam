#include "pch.h"
#include "Render/UI/Widget/Collision/Public/BoxComponentWidget.h"
#include "Component/Collision/Public/BoxComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(UBoxComponentWidget, UWidget)

void UBoxComponentWidget::Update()
{
    UActorComponent* Selected = GEditor->GetEditorModule()->GetSelectedComponent();
    BoxComponent = Cast<UBoxComponent>(Selected);
}

void UBoxComponentWidget::RenderWidget()
{
    if (!BoxComponent)
    {
        return;
    }

    ImGui::Separator();

    // Overlap 옵션
    bool GenerateOverlap = BoxComponent->bGenerateOverlapEvents;
    if (ImGui::Checkbox("Generate Overlap Events", &GenerateOverlap))
    {
        BoxComponent->bGenerateOverlapEvents = GenerateOverlap;
    }

    // Hit 옵션 (새로 추가)
    bool GenerateHit = BoxComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("Generate Hit Events", &GenerateHit))
    {
        BoxComponent->bGenerateHitEvents = GenerateHit;
    }

    // Block 옵션
    bool BlockComponent = BoxComponent->bBlockComponent;
    if (ImGui::Checkbox("Block Component", &BlockComponent))
    {
        BoxComponent->bBlockComponent = BlockComponent;
    }

    // Box Extent (X/Y/Z)
    FVector Extent = BoxComponent->GetBoxExtent();
    float ExtentArr[3] = { Extent.X, Extent.Y, Extent.Z };
    if (ImGui::DragFloat3("Box Extent", ExtentArr, 0.1f, 0.0f, 100000.0f))
    {
        BoxComponent->SetBoxExtent(FVector(ExtentArr[0], ExtentArr[1], ExtentArr[2]));
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("박스의 반-크기(Extents)입니다. 단위: cm");
    }

    // 색상 (FColor <-> ImGui 0..1 float)
    FColor ShapeColor = BoxComponent->GetShapeColor();
    float Color01[4] = {
        ShapeColor.R / 255.0f,
        ShapeColor.G / 255.0f,
        ShapeColor.B / 255.0f,
        ShapeColor.A / 255.0f
    };
    if (ImGui::ColorEdit4("Shape Color", Color01, ImGuiColorEditFlags_NoInputs))
    {
        FColor NewColor(
            static_cast<uint8>(Color01[0] * 255.0f + 0.5f),
            static_cast<uint8>(Color01[1] * 255.0f + 0.5f),
            static_cast<uint8>(Color01[2] * 255.0f + 0.5f),
            static_cast<uint8>(Color01[3] * 255.0f + 0.5f)
        );
        BoxComponent->SetShapeColor(NewColor);
    }

    // Draw Only If Selected
    bool DrawOnlyIfSelected = BoxComponent->ShouldDrawOnlyIfSelected();
    if (ImGui::Checkbox("Draw Only If Selected", &DrawOnlyIfSelected))
    {
        BoxComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }

}