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
    if (ImGui::Checkbox("오버랩 이벤트 생성", &GenerateOverlap))
    {
        CapsuleComponent->bGenerateOverlapEvents = GenerateOverlap;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 오버랩 이벤트를 생성합니다.");
    }
    

    bool GenerateHit = CapsuleComponent->bGenerateHitEvents;
    if (ImGui::Checkbox("히트 이벤트 생성", &GenerateHit))
    {
        CapsuleComponent->bGenerateHitEvents = GenerateHit;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 충돌 히트 이벤트를 생성합니다.\n"
                            "\"오브젝트 통과 막기\"도 체크해야 히트 이벤트가 작동합니다.");
    }
    

    bool BlockComponent = CapsuleComponent->bBlockComponent;
    if (ImGui::Checkbox("오브젝트 통과 막기", &BlockComponent))
    {
        CapsuleComponent->bBlockComponent = BlockComponent;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 시, 오브젝트 통과를 막습니다.(아직 미구현)\n"
            "체크해야 히트 이벤트가 작동합니다.");
    }

    float HalfHeight = CapsuleComponent->GetCapsuleHalfHeight();
    float Radius = CapsuleComponent->GetCapsuleRadius();

    if (ImGui::DragFloat("캡슐 절반 높이", &HalfHeight, 0.1f, Radius, 100000.0f))
    {
        // Radius보다 작아지지 않도록 클램핑
        if (HalfHeight < Radius)
        {
            HalfHeight = Radius;
        }
        CapsuleComponent->SetCapsuleHalfHeight(HalfHeight);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("캡슐의 중앙에서 상단 또는 하단 반구 중심까지의 해당하는 거리입니다.\n",
                            "캡슐 반경보다 작을 수 없습니다.");
    }
    if (ImGui::DragFloat("캡슐 반경", &Radius, 0.1f, 0.0f, HalfHeight))
    {
        // HalfHeight보다 커지지 않도록 클램핑
        if (Radius > HalfHeight)
        {
            Radius = HalfHeight;
        }
        CapsuleComponent->SetCapsuleRadius(Radius);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("캡슐 반구와 중앙 실린더(원기둥)의 반경(반지름)입니다.\n"
                            "캡슐 절반 높이보다 클 수 없습니다.");
    }

    FColor ShapeColor = CapsuleComponent->GetShapeColor();
    float Color01[4] = { ShapeColor.R / 255.f, ShapeColor.G / 255.f, ShapeColor.B / 255.f, ShapeColor.A / 255.f };
    if (ImGui::ColorEdit4("셰이프 색", Color01, ImGuiColorEditFlags_NoInputs))
    {
        FColor NewColor(
            static_cast<uint8>(Color01[0] * 255.f + 0.5f),
            static_cast<uint8>(Color01[1] * 255.f + 0.5f),
            static_cast<uint8>(Color01[2] * 255.f + 0.5f),
            static_cast<uint8>(Color01[3] * 255.f + 0.5f));
        CapsuleComponent->SetShapeColor(NewColor);
    }

    bool DrawOnlyIfSelected = CapsuleComponent->ShouldDrawOnlyIfSelected();
    if (ImGui::Checkbox("선택될 때만 그리기", &DrawOnlyIfSelected))
    {
        CapsuleComponent->SetDrawOnlyIfSelected(DrawOnlyIfSelected);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("충돌 컴포넌트가 직접 선택될 때만 그립니다.");
    }

}