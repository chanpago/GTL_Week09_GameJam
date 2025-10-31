#include "pch.h"
#include "Component/Public/PointLightComponent.h"

#include "Render/UI/Widget/Public/PointLightComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UPointLightComponent, ULightComponent)

void UPointLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "DistanceFalloffExponent", DistanceFalloffExponent);
		SetDistanceFalloffExponent(DistanceFalloffExponent); // clamping을 위해 Setter 사용
		FJsonSerializer::ReadFloat(InOutHandle, "AttenuationRadius", AttenuationRadius);
	}
	else
	{
		InOutHandle["DistanceFalloffExponent"] = DistanceFalloffExponent;
		InOutHandle["AttenuationRadius"] = AttenuationRadius;
	}
}

UObject* UPointLightComponent::Duplicate()
{
	UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Super::Duplicate());
	PointLightComponent->DistanceFalloffExponent = DistanceFalloffExponent;
	PointLightComponent->AttenuationRadius = AttenuationRadius;

	return PointLightComponent;
}

UClass* UPointLightComponent::GetSpecificWidgetClass() const
{
    return UPointLightComponentWidget::StaticClass();
}

void UPointLightComponent::EnsureVisualizationIcon()
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
	Icon->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/S_LightPoint.png"));
	Icon->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));
	Icon->SetScreenSizeScaled(true);

	VisualizationIcon = Icon;
	UpdateVisualizationIconTint();
}
FPointLightInfo UPointLightComponent::GetPointlightInfo() const
{
	FPointLightInfo Info;
	Info.Color = FVector4(LightColor, 1.0f);
	Info.Position = GetWorldLocation();
	Info.Intensity = Intensity;
	Info.Range = AttenuationRadius;
	Info.DistanceFalloffExponent = DistanceFalloffExponent;

	// Shadow parameters
	Info.CastShadow = GetCastShadows() ? 1u : 0u;
	Info.ShadowModeIndex = static_cast<uint32>(GetShadowModeIndex());
	Info.ShadowBias = GetShadowBias();
	Info.ShadowSlopeBias = GetShadowSlopeBias();
	Info.ShadowSharpen = GetShadowSharpen();
	Info.Resolution = GetShadowResolutionScale();

	return Info;
}
