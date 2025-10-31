#include "pch.h"
#include "Component/Public/AmbientLightComponent.h"

#include "Render/UI/Widget/Public/AmbientLightComponentWidget.h"

IMPLEMENT_CLASS(UAmbientLightComponent, ULightComponent)

void UAmbientLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}

UObject* UAmbientLightComponent::Duplicate()
{
	UAmbientLightComponent* AmbientLightComponent = Cast<UAmbientLightComponent>(Super::Duplicate());

	return AmbientLightComponent;
}

UClass* UAmbientLightComponent::GetSpecificWidgetClass() const
{
	return UAmbientLightComponentWidget::StaticClass();
}

void UAmbientLightComponent::EnsureVisualizationIcon()
{
	if (VisualizationIcon)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (GWorld)
	{
		EWorldType WorldType = GWorld->GetWorldType();
		if (WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
		{
			return;
		}
	}

	UEditorIconComponent* Icon = OwnerActor->AddComponent<UEditorIconComponent>();
	if (!Icon)
	{
		return;
	}
	Icon->AttachToComponent(this);
	Icon->SetIsVisualizationComponent(true);
	Icon->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/SkyLight.png"));
	Icon->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));
	Icon->SetScreenSizeScaled(true);

	VisualizationIcon = Icon;
	UpdateVisualizationIconTint();
}

FAmbientLightInfo UAmbientLightComponent::GetAmbientLightInfo() const
{
	return FAmbientLightInfo{ FVector4(LightColor, 1), Intensity };
}