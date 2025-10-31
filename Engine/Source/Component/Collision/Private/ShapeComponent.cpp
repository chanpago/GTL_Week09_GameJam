#include "pch.h"
#include "Component/Collision/Public/ShapeComponent.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_ABSTRACT_CLASS(UShapeComponent, UPrimitiveComponent)

UShapeComponent::UShapeComponent()
{
}

void UShapeComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    if (bInIsLoading)
    {
        // 1) 색상 로드: 배열 [R,G,B,A] 또는 "#RRGGBB"/"#RRGGBBAA"
        FColor LoadedColor = GetShapeColor(); // 기본값: 현재 보유 색상
        FJsonSerializer::ReadColor(InOutHandle, "ShapeColor", LoadedColor, FColor(255, 255, 255, 51));
        SetShapeColor(LoadedColor);

        // 2) 플래그 로드: 문자열 "true"/"false" 패턴 유지
        FString DrawOnlyString;
        FJsonSerializer::ReadString(InOutHandle, "bDrawOnlyIfSelected", DrawOnlyString, "false");
        SetDrawOnlyIfSelected(DrawOnlyString == "true");
    }
    else
    {
        // 1) 색상 저장: [R,G,B,A]
        InOutHandle["ShapeColor"] = FJsonSerializer::ColorToJson(GetShapeColor());

        // 2) 플래그 저장: 문자열 "true"/"false"
        InOutHandle["bDrawOnlyIfSelected"] = ShouldDrawOnlyIfSelected() ? "true" : "false";
    }
}

UObject* UShapeComponent::Duplicate()
{
    UShapeComponent* ShapeComponent = Cast<UShapeComponent>(Super::Duplicate());
    ShapeComponent->SetShapeColor(ShapeColor);
    ShapeComponent->SetDrawOnlyIfSelected(bDrawOnlyIfSelected);
    return ShapeComponent;
}

void UShapeComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}