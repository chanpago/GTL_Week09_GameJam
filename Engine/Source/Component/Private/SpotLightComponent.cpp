#include "pch.h"
#include "Component/Public/SpotLightComponent.h"
#include "Render/UI/Widget/Public/SpotLightComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/EditorPrimitive.h"

IMPLEMENT_CLASS(USpotLightComponent, UPointLightComponent)

USpotLightComponent::USpotLightComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	// 화살표 프리미티브 설정 (빛 방향 표시용)
	LightDirectionArrow.VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
	LightDirectionArrow.NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
	LightDirectionArrow.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	LightDirectionArrow.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	LightDirectionArrow.bShouldAlwaysVisible = false;
}

void USpotLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "AngleFalloffExponent", AngleFalloffExponent);
        SetAngleFalloffExponent(AngleFalloffExponent); // clamping을 위해 Setter 사용
        FJsonSerializer::ReadFloat(InOutHandle, "AttenuationAngle", OuterConeAngleRad);
    }
    else
    {
        InOutHandle["AngleFalloffExponent"] = AngleFalloffExponent;
        InOutHandle["AttenuationAngle"] = OuterConeAngleRad;
    }
}

UObject* USpotLightComponent::Duplicate()
{
    USpotLightComponent* NewSpotLightComponent = Cast<USpotLightComponent>(Super::Duplicate());
    NewSpotLightComponent->SetAngleFalloffExponent(AngleFalloffExponent);
    NewSpotLightComponent->SetOuterAngle(OuterConeAngleRad);

    return NewSpotLightComponent;
}

UClass* USpotLightComponent::GetSpecificWidgetClass() const
{
    return USpotLightComponentWidget::StaticClass();
}

FVector USpotLightComponent::GetForwardVector() const
{
    FQuaternion Rotation = GetWorldRotationAsQuaternion();
    return Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
}

void USpotLightComponent::SetOuterAngle(float const InAttenuationAngleRad)
{
    OuterConeAngleRad = std::clamp(InAttenuationAngleRad, 0.0f, PI/2.0f - MATH_EPSILON);
    InnerConeAngleRad = std::min(InnerConeAngleRad, OuterConeAngleRad);
}

void USpotLightComponent::SetInnerAngle(float const InAttenuationAngleRad)
{
    InnerConeAngleRad = std::clamp(InAttenuationAngleRad, 0.0f, OuterConeAngleRad);
}

void USpotLightComponent::RenderLightDirectionGizmo(UCamera* InCamera, const D3D11_VIEWPORT& InViewport)
{
	if (!InCamera)
	{
	    return;
	}

	FVector LightLocation = GetWorldLocation();
	FQuaternion LightRotation = GetWorldRotationAsQuaternion();

	// Gizmo와 동일한 Screen Space Scale 계산
	const FCameraConstants& CameraConstants = InCamera->GetFViewProjConstants();
	const FMatrix& ProjMatrix = CameraConstants.Projection;
	const ECameraType CameraType = InCamera->GetCameraType();
	const float ViewportHeight = InViewport.Height;

	float Scale = 1.0f;
	const float DesiredPixelSize = 30.0f;

	if (ViewportHeight > 1.0f)
	{
		float ProjYY = abs(ProjMatrix.Data[1][1]);
		if (ProjYY > 0.0001f)
		{
			if (CameraType == ECameraType::ECT_Perspective)
			{
				const FMatrix& ViewMatrix = CameraConstants.View;
				FVector4 GizmoPos4(LightLocation.X, LightLocation.Y, LightLocation.Z, 1.0f);
				FVector4 ViewSpacePos = GizmoPos4 * ViewMatrix;
				float ProjectedDepth = std::max(abs(ViewSpacePos.Z), 1.0f);
				Scale = (DesiredPixelSize * ProjectedDepth) / (ProjYY * ViewportHeight * 0.5f);
			}
			else // Orthographic
			{
				float OrthoHeight = 2.0f / ProjYY;
				Scale = (DesiredPixelSize * OrthoHeight) / ViewportHeight;
			}
			Scale = std::max(0.01f, std::min(Scale, 100.0f));
		}
	}

	LightDirectionArrow.Location = LightLocation;
	LightDirectionArrow.Rotation = LightRotation;
	LightDirectionArrow.Scale = FVector(Scale, Scale, Scale);

	FRenderState RenderState;
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;

	URenderer::GetInstance().RenderEditorPrimitive(LightDirectionArrow, RenderState);
}

FSpotLightInfo USpotLightComponent::GetSpotLightInfo() const
{
    FSpotLightInfo Info;
    Info.Color = FVector4(LightColor, 1);
    Info.Position = GetWorldLocation();
    Info.Intensity = Intensity;
    Info.Range = AttenuationRadius;
    Info.DistanceFalloffExponent = DistanceFalloffExponent;
    Info.InnerConeAngle = InnerConeAngleRad;
    Info.OuterConeAngle = OuterConeAngleRad;
    Info.AngleFalloffExponent = AngleFalloffExponent;
    Info.Direction = GetForwardVector();

    // Shadow parameters
    Info.LightViewProjection = CachedShadowViewProjection; // Updated by ShadowMapPass
    Info.CastShadow = GetCastShadows() ? 1u : 0u;
    Info.ShadowModeIndex = static_cast<uint32>(GetShadowModeIndex());
    Info.ShadowBias = GetShadowBias();
    Info.ShadowSlopeBias = GetShadowSlopeBias();
    Info.ShadowSharpen = GetShadowSharpen();
    Info.Resolution = GetShadowResolutionScale();

    return Info;
}

void USpotLightComponent::EnsureVisualizationIcon()
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
    Icon->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/S_LightSpot.png"));
    Icon->SetRelativeScale3D(FVector(5.f,5.f,5.f));
    Icon->SetScreenSizeScaled(true);

    VisualizationIcon = Icon;
    UpdateVisualizationIconTint();
}
