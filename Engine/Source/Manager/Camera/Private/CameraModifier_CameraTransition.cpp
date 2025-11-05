#include "pch.h"
#include "Manager/Camera/Public/CameraModifier_CameraTransition.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"
#include "Editor/Public/Camera.h"
#include "Component/Camera/Public/CameraComponent.h"

IMPLEMENT_CLASS(UCameraModifier_CameraTransition, UCameraModifier)

UCameraModifier_CameraTransition::UCameraModifier_CameraTransition()
	: TransitionTimer(0.0f)
	, TransitionDuration(1.0f)
	, TransitionEaseType(ECameraEaseType::EaseInOut)
	, bIsTransitioning(false)
	, TargetActor(nullptr)
	, ActorOffset(FVector::Zero())
{
	// Priority 설정 (CameraShake보다 높아야 전환이 먼저 적용됨)
	Priority = 150;
}

void UCameraModifier_CameraTransition::StartTransitionToLocation(
	const FVector& TargetLocation,
	const FRotator& TargetRotation,
	float Duration,
	ECameraEaseType EaseType,
	float TargetFOV,
	const float* BezierCP)
{
	if (!CameraOwner)
	{
		return;
	}

	// Get current camera state
	UCamera* Camera = CameraOwner->GetCamera();
	UCameraComponent* CameraComponent = CameraOwner->GetCameraComponent();

	if (Camera)
	{
		TransitionStartView.Location = Camera->GetLocation();
		TransitionStartView.Rotation = Camera->GetRotationRotator();
		TransitionStartView.FOV = Camera->GetFovY();
	}
	else if (CameraComponent)
	{
		TransitionStartView.Location = CameraComponent->GetWorldLocation();
		FVector EulerRot = CameraComponent->GetWorldRotation();
		TransitionStartView.Rotation = FRotator(EulerRot.X, EulerRot.Y, EulerRot.Z);
		TransitionStartView.FOV = CameraComponent->GetFieldOfView();
	}
	else
	{
		return;
	}

	// Set target state
	TransitionTargetView.Location = TargetLocation;
	TransitionTargetView.Rotation = TargetRotation;
	TransitionTargetView.Target = nullptr;
	TransitionTargetView.FOV = (TargetFOV > 0.0f) ? TargetFOV : TransitionStartView.FOV;

	// Set transition parameters
	TransitionDuration = Duration;
	TransitionTimer = 0.0f;
	TransitionEaseType = EaseType;
	bIsTransitioning = true;
	TargetActor = nullptr;
	ActorOffset = FVector::Zero();

	// Copy Bezier control points if provided
	if (BezierCP != nullptr)
	{
		this->BezierCP[0] = BezierCP[0];
		this->BezierCP[1] = BezierCP[1];
		this->BezierCP[2] = BezierCP[2];
		this->BezierCP[3] = BezierCP[3];
	}

	// Enable modifier
	EnableModifierInstant();
}

void UCameraModifier_CameraTransition::StartTransitionToActor(
	AActor* InTargetActor,
	float Duration,
	ECameraEaseType EaseType,
	const FVector& Offset,
	const float* InBezierCP)
{
	if (!CameraOwner || !InTargetActor)
	{
		return;
	}

	// Get current camera state
	UCamera* Camera = CameraOwner->GetCamera();
	UCameraComponent* CameraComponent = CameraOwner->GetCameraComponent();

	if (Camera)
	{
		TransitionStartView.Location = Camera->GetLocation();
		TransitionStartView.Rotation = Camera->GetRotationRotator();
		TransitionStartView.FOV = Camera->GetFovY();
	}
	else if (CameraComponent)
	{
		TransitionStartView.Location = CameraComponent->GetWorldLocation();
		FVector EulerRot = CameraComponent->GetWorldRotation();
		TransitionStartView.Rotation = FRotator(EulerRot.X, EulerRot.Y, EulerRot.Z);
		TransitionStartView.FOV = CameraComponent->GetFieldOfView();
	}
	else
	{
		return;
	}

	// Set target state (will be updated each frame to follow actor)
	TransitionTargetView.Target = InTargetActor;
	TransitionTargetView.Location = InTargetActor->GetActorLocation() + Offset;

	// Calculate look-at rotation
	FVector Direction = (InTargetActor->GetActorLocation() - TransitionTargetView.Location).GetNormalized();
	float Yaw = atan2f(Direction.Y, Direction.X) * (180.0f / 3.14159265359f);
	float Pitch = asinf(-Direction.Z) * (180.0f / 3.14159265359f);
	TransitionTargetView.Rotation = FRotator(Pitch, Yaw, 0.0f);
	TransitionTargetView.FOV = TransitionStartView.FOV;

	// Set transition parameters
	TransitionDuration = Duration;
	TransitionTimer = 0.0f;
	TransitionEaseType = EaseType;
	bIsTransitioning = true;
	TargetActor = InTargetActor;
	ActorOffset = Offset;

	// Copy Bezier control points if provided
	if (InBezierCP != nullptr)
	{
		this->BezierCP[0] = InBezierCP[0];
		this->BezierCP[1] = InBezierCP[1];
		this->BezierCP[2] = InBezierCP[2];
		this->BezierCP[3] = InBezierCP[3];
	}

	// Enable modifier
	EnableModifierInstant();
}

void UCameraModifier_CameraTransition::StopTransition()
{
	bIsTransitioning = false;
	TransitionTimer = 0.0f;
	TargetActor = nullptr;

	// Disable modifier
	DisableModifierInstant();
}

float UCameraModifier_CameraTransition::GetTransitionProgress() const
{
	if (!bIsTransitioning || TransitionDuration <= 0.0f)
	{
		return 0.0f;
	}

	float RawAlpha = TransitionTimer / TransitionDuration;
	return Clamp(RawAlpha, 0.0f, 1.0f);
}

void UCameraModifier_CameraTransition::UpdateModifier(float DeltaTime)
{
	// Base class alpha update (for fade in/out)
	UCameraModifier::UpdateModifier(DeltaTime);

	if (!bIsTransitioning)
	{
		return;
	}

	// Update timer
	TransitionTimer += DeltaTime;

	// Calculate alpha (0 to 1)
	float RawAlpha = TransitionTimer / TransitionDuration;
	RawAlpha = Clamp(RawAlpha, 0.0f, 1.0f);

	// Update target if following an actor
	if (TargetActor != nullptr)
	{
		TransitionTargetView.Location = TargetActor->GetActorLocation() + ActorOffset;

		// Calculate look-at rotation
		FVector Direction = (TargetActor->GetActorLocation() - TransitionTargetView.Location).GetNormalized();
		float Yaw = atan2f(Direction.Y, Direction.X) * (180.0f / 3.14159265359f);
		float Pitch = asinf(-Direction.Z) * (180.0f / 3.14159265359f);
		TransitionTargetView.Rotation = FRotator(Pitch, Yaw, 0.0f);
	}

	// Check if transition is complete
	if (TransitionTimer >= TransitionDuration)
	{
		StopTransition();
	}
}

bool UCameraModifier_CameraTransition::ModifyCamera(float DeltaTime, FVector& CameraLocation, FRotator& CameraRotation)
{
	if (!bIsTransitioning)
	{
		return false;
	}

	// Calculate alpha (0 to 1)
	float RawAlpha = TransitionTimer / TransitionDuration;
	RawAlpha = Clamp(RawAlpha, 0.0f, 1.0f);

	// Apply easing function (with Bezier control points if using Bezier easing)
	float Alpha = ApplyEasing(RawAlpha, TransitionEaseType, BezierCP);

	// Interpolate location
	FVector StartLoc = TransitionStartView.Location;
	FVector TargetLoc = TransitionTargetView.Location;
	FVector NewLocation = Lerp(StartLoc, TargetLoc, Alpha);

	// Interpolate rotation using SLERP for smooth rotation
	FQuaternion StartQuat = TransitionStartView.Rotation.Quaternion();
	FQuaternion TargetQuat = TransitionTargetView.Rotation.Quaternion();
	FQuaternion NewQuat = FQuaternion::Slerp(StartQuat, TargetQuat, Alpha);
	FVector EulerAngles = NewQuat.ToEuler();
	FRotator NewRotation = FRotator(EulerAngles.Y, EulerAngles.Z, EulerAngles.X);

	// Apply to output parameters
	CameraLocation = NewLocation;
	CameraRotation = NewRotation;

	// Interpolate FOV and apply directly to camera
	float StartFOV = TransitionStartView.FOV;
	float TargetFOV = TransitionTargetView.FOV;
	float NewFOV = Lerp(StartFOV, TargetFOV, Alpha);

	if (CameraOwner)
	{
		UCamera* Camera = CameraOwner->GetCamera();
		UCameraComponent* CameraComponent = CameraOwner->GetCameraComponent();

		if (Camera)
		{
			Camera->SetFovY(NewFOV);
		}
		else if (CameraComponent)
		{
			CameraComponent->SetFieldOfView(NewFOV);
		}
	}

	return false;  // false = allow other modifiers to apply
}
