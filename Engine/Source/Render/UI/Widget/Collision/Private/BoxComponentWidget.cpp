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
    // 모든 입력 필드를 검은색으로 설정
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // 체크 표시 흰색
    ImGui::Separator();

    // Overlap 옵션
    bool GenerateOverlap = BoxComponent->bGenerateOverlapEvents;
    if (ImGui::Checkbox("오버랩 이벤트 생성", &GenerateOverlap))
    {
        BoxComponent->bGenerateOverlapEvents = GenerateOverlap;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 오버랩 이벤트를 생성합니다.");
    }

    // Hit 옵션 (새로 추가)
    bool GenerateHit = BoxComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("히트 이벤트 생성", &GenerateHit))
    {
        BoxComponent->bGenerateHitEvents = GenerateHit;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 히트 이벤트를 생성합니다.\n"
                            "\"오브젝트 통과 막기\"도 체크해야 히트 이벤트가 작동합니다.");
    }
    // Block 옵션
    bool BlockComponent = BoxComponent->bBlockComponent;
    if (ImGui::Checkbox("오브젝트 통과 막기", &BlockComponent))
    {
        BoxComponent->bBlockComponent = BlockComponent;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 오브젝트 통과를 막습니다.(아직 미구현)\n"
                          "체크해야 히트 이벤트가 작동합니다.");
    }
    // Box Extent (X/Y/Z)
    FVector Extent = BoxComponent->GetBoxExtent();
    float ExtentArr[3] = { Extent.X, Extent.Y, Extent.Z };
    if (ImGui::DragFloat3("박스 크기", ExtentArr, 0.1f, 0.0f, 100000.0f))
    {
        BoxComponent->SetBoxExtent(FVector(ExtentArr[0], ExtentArr[1], ExtentArr[2]));
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("박스의 범위(Half Extent)입니다.");
    }

    // 색상 (FColor <-> ImGui 0..1 float)
    FColor ShapeColor = BoxComponent->GetShapeColor();
    float Color01[4] = {
        ShapeColor.R / 255.0f,
        ShapeColor.G / 255.0f,
        ShapeColor.B / 255.0f,
        ShapeColor.A / 255.0f
    };
    if (ImGui::ColorEdit4("셰이프 색", Color01, ImGuiColorEditFlags_NoInputs))
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
    if (ImGui::Checkbox("선택될 때만 그리기", &DrawOnlyIfSelected))
    {
        BoxComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("충돌 컴포넌트가 직접 선택될 때만 그립니다.");
    }
    // 스타일 복원
    ImGui::PopStyleColor(6);
}