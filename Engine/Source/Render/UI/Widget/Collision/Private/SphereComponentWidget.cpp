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
    // 모든 입력 필드를 검은색으로 설정
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // 체크 표시 흰색
    ImGui::Separator();

    bool GenerateOverlap = SphereComponent->bGenerateOverlapEvents;
    if (ImGui::Checkbox("오버랩 이벤트 생성", &GenerateOverlap))
    {
        SphereComponent->bGenerateOverlapEvents = GenerateOverlap;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 오버랩 이벤트를 생성합니다.");
    }
    bool GenerateHit = SphereComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("히트 이벤트 생성", &GenerateHit))
    {
        SphereComponent->bGenerateHitEvents = GenerateHit;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 히트 이벤트를 생성합니다.\n"
            "\"오브젝트 통과 막기\"도 체크해야 히트 이벤트가 작동합니다.");
    }
    bool BlockComponent = SphereComponent->bBlockComponent;
    if (ImGui::Checkbox("오브젝트 통과 막기", &BlockComponent))
    {
        SphereComponent->bBlockComponent = BlockComponent;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 오브젝트 통과를 막습니다.(아직 미구현)\n"
            "체크해야 히트 이벤트가 작동합니다.");
    }

    float Radius = SphereComponent->GetSphereRadius();
    if (ImGui::DragFloat("스피어 반경", &Radius, 0.1f, 0.0f, 100000.0f))
    {
        SphereComponent->SetSphereRadius(Radius);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("구의 반지름(Radius)입니다.");
    }

    FColor ShapeColor = SphereComponent->GetShapeColor();
    float Color01[4] = { ShapeColor.R / 255.f, ShapeColor.G / 255.f, ShapeColor.B / 255.f, ShapeColor.A / 255.f };
    if (ImGui::ColorEdit4("셰이프 색", Color01, ImGuiColorEditFlags_NoInputs))
    {
        FColor NewColor(
            static_cast<uint8>(Color01[0] * 255.f + 0.5f),
            static_cast<uint8>(Color01[1] * 255.f + 0.5f),
            static_cast<uint8>(Color01[2] * 255.f + 0.5f),
            static_cast<uint8>(Color01[3] * 255.f + 0.5f));
        SphereComponent->SetShapeColor(NewColor);
    }

    bool DrawOnlyIfSelected = SphereComponent->ShouldDrawOnlyIfSelected();
    if (ImGui::Checkbox("선택될 때만 그리기", &DrawOnlyIfSelected))
    {
        SphereComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("충돌 컴포넌트가 직접 선택될 때만 그립니다.");
    }
    // 스타일 복원
    ImGui::PopStyleColor(6);
}