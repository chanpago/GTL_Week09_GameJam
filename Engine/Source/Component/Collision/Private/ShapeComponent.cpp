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
    // TODO(SDM)
    // 현재 프로젝트의 FJsonSerializer에 Vector4 직렬화 헬퍼가 없으므로
    // ShapeColor, bDrawOnlyIfSelected는 직렬화에서 제외합니다. 필요 시 확장하세요.
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