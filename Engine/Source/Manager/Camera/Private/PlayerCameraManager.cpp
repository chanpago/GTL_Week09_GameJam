#include "pch.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"
#include "Manager/Camera/Public/CameraModifier.h"
#include "Manager/Camera/Public/CameraModifier_CameraShake.h"
#include "Manager/Camera/Public/CameraModifier_CameraTransition.h"
#include "Editor/Public/Camera.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Pawn/Public/Pawn.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Component/Collision/Public/ShapeComponent.h"
#include "Player/Public/EnemyCharacter.h"
#include "Global/Quaternion.h"

IMPLEMENT_CLASS(APlayerCameraManager, AActor)

// Static 변수 정의
float APlayerCameraManager::StaticShakeBezierCP[4] = { 0.250f, 0.460f, 0.450f, 0.940f };
bool APlayerCameraManager::StaticUseBezierDecay = true;
bool APlayerCameraManager::bStaticValuesInitialized = false;

APlayerCameraManager::APlayerCameraManager()
	: Camera(nullptr)
	, FadeColorLinear(0, 0, 0, 1)
	, FadeAmount(0.0f)
	, FadeAlpha(0, 0)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bFadeOut(true)
	, bHoldFade(false)
	, bIsLetterBoxActive(false)
	, LetterBoxHeight(0.1f)
	, LetterBoxTargetAlpha(0.0f)
	, LetterBoxCurrentAlpha(0.0f)
	, LetterBoxBlendSpeed(2.0f)
	, bSpringArmEnabled(true)
	, SpringArmOffset(-300.0f, 0.0f, 100.0f)
	, SpringArmLength(300.0f)
	, SpringArmInterpSpeed(10.0f)
	, bEnableCollisionTest(true)
	, CurrentViewType(ECameraViewType::ThirdPerson)
	, PreviousViewType(ECameraViewType::ThirdPerson)
	, ViewTransitionDuration(1.0f)
	, ViewTransitionTimeRemaining(0.0f)
	, bIsTransitioning(false)
{
	// ✅ Tick 활성화 - 카메라 modifier 업데이트를 위해 필수!
	bCanEverTick = true;
}

APlayerCameraManager::~APlayerCameraManager()
{
	// Clean up modifiers
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			delete Modifier;
		}
	}
	ModifierList.clear();
}

void APlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	// Static 값이 초기화되어 있으면 그걸 사용 (에디터→PIE 전달)
	if (bStaticValuesInitialized)
	{
		DefaultShakeBezierCP[0] = StaticShakeBezierCP[0];
		DefaultShakeBezierCP[1] = StaticShakeBezierCP[1];
		DefaultShakeBezierCP[2] = StaticShakeBezierCP[2];
		DefaultShakeBezierCP[3] = StaticShakeBezierCP[3];
		bDefaultUseBezierDecay = StaticUseBezierDecay;
	}
	else
	{
		// 처음 실행 시 현재 값을 static에 저장
		StaticShakeBezierCP[0] = DefaultShakeBezierCP[0];
		StaticShakeBezierCP[1] = DefaultShakeBezierCP[1];
		StaticShakeBezierCP[2] = DefaultShakeBezierCP[2];
		StaticShakeBezierCP[3] = DefaultShakeBezierCP[3];
		StaticUseBezierDecay = bDefaultUseBezierDecay;
		bStaticValuesInitialized = true;
	}

	// Add default modifiers if none exist
	if (ModifierList.empty())
	{
		// Camera Shake Modifier
		UCameraModifier_CameraShake* ShakeMod = new UCameraModifier_CameraShake();

		// Apply default Bezier settings from this manager
		ShakeMod->BezierCP[0] = DefaultShakeBezierCP[0];
		ShakeMod->BezierCP[1] = DefaultShakeBezierCP[1];
		ShakeMod->BezierCP[2] = DefaultShakeBezierCP[2];
		ShakeMod->BezierCP[3] = DefaultShakeBezierCP[3];
		ShakeMod->bUseBezierDecay = bDefaultUseBezierDecay;

		AddCameraModifier(ShakeMod);

		// Camera Transition Modifier (미리 생성해서 UI에서 베지어 곡선 표시 가능)
		UCameraModifier_CameraTransition* TransitionMod = new UCameraModifier_CameraTransition();
		AddCameraModifier(TransitionMod);
	}
}

void APlayerCameraManager::Initialize(UCamera* InCamera)
{
	Camera = InCamera;
	CameraComponent = nullptr;
}

void APlayerCameraManager::Initialize(UCameraComponent* InCameraComponent)
{
	CameraComponent = InCameraComponent;
	Camera = nullptr;
}

void APlayerCameraManager::Tick(float DeltaTime)
{
	if (!Camera && !CameraComponent)
	{
		return;
	}

	// Update all systems
	UpdateFade(DeltaTime);
	UpdateLetterBox(DeltaTime);
	UpdateViewTransition(DeltaTime);
	UpdateSpringArm(DeltaTime);

	// PIE mode: Apply camera modifiers to CameraComponent
	if (CameraComponent)
	{
		// Get initial camera transform
		FVector CameraLocation = CameraComponent->GetWorldLocation();
		FRotator CameraRotation = FRotator(0, 0, 0);  // TODO: Get actual rotation if needed

		// Apply all camera modifiers (including shake)
		ApplyCameraModifiers(DeltaTime, CameraLocation, CameraRotation);

		// Calculate offset from original location
		FVector ShakeOffset = CameraLocation - CameraComponent->GetWorldLocation();

		// Apply the offset to CameraComponent
		CameraComponent->SetCameraShakeOffset(ShakeOffset);
	}
	// Editor mode: Apply to UCamera
	else if (Camera)
	{
		// Get camera location and rotation
		FVector CameraLocation = Camera->GetLocation();
		FRotator CameraRotation = Camera->GetRotationRotator();

		// Apply camera modifiers (shake, etc.)
		ApplyCameraModifiers(DeltaTime, CameraLocation, CameraRotation);

		// Update camera transform
		Camera->SetLocation(CameraLocation);
		Camera->SetRotation(CameraRotation);
	}
}

void APlayerCameraManager::SetViewTarget(AActor* NewTarget)
{
	ViewTarget.Target = NewTarget;
}

// ========== Fade System ==========

void APlayerCameraManager::StartCameraFade(float Duration, FVector4 ToColor, bool bInFadeOut, bool bHoldWhenFinished)
{
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	FadeColorLinear = ToColor;
	bFadeOut = bInFadeOut;
	bHoldFade = bHoldWhenFinished;

	if (bInFadeOut)
	{
		// Fade to color: 0 -> 1
		FadeAmount = 0.0f;
	}
	else
	{
		// Fade from color: 1 -> 0
		FadeAmount = 1.0f;
	}
}

void APlayerCameraManager::StopCameraFade()
{
	FadeTimeRemaining = 0.0f;
	FadeAmount = 0.0f;
	bHoldFade = false;
}

void APlayerCameraManager::UpdateFade(float DeltaTime)
{
	if (FadeTimeRemaining > 0.0f)
	{
		FadeTimeRemaining -= DeltaTime;

		if (FadeTimeRemaining <= 0.0f)
		{
			// Fade finished
			FadeTimeRemaining = 0.0f;
			if (bFadeOut)
			{
				FadeAmount = 1.0f;
			}
			else
			{
				FadeAmount = 0.0f;
			}

			if (!bHoldFade)
			{
				FadeAmount = 0.0f;
			}
		}
		else
		{
			// Calculate fade alpha
			float FadeAlpha = 1.0f - (FadeTimeRemaining / FadeTime);

			if (bFadeOut)
			{
				// Fade to color: 0 -> 1
				FadeAmount = FadeAlpha;
			}
			else
			{
				// Fade from color: 1 -> 0
				FadeAmount = 1.0f - FadeAlpha;
			}
		}
	}
}

// ========== Letter Box System ==========

void APlayerCameraManager::StartLetterBox(float Height, float BlendTime)
{
	bIsLetterBoxActive = true;
	LetterBoxHeight = Height;
	LetterBoxTargetAlpha = 1.0f;
	LetterBoxBlendSpeed = (BlendTime > 0.0f) ? (1.0f / BlendTime) : 2.0f;
}

void APlayerCameraManager::StopLetterBox(float BlendTime)
{
	LetterBoxTargetAlpha = 0.0f;
	LetterBoxBlendSpeed = (BlendTime > 0.0f) ? (1.0f / BlendTime) : 2.0f;
}

void APlayerCameraManager::UpdateLetterBox(float DeltaTime)
{
	if (bIsLetterBoxActive || LetterBoxCurrentAlpha > 0.0f)
	{
		// Lerp toward target
		LetterBoxCurrentAlpha = Lerp(LetterBoxCurrentAlpha, LetterBoxTargetAlpha, LetterBoxBlendSpeed * DeltaTime);

		// Snap to target if very close
		if (fabs(LetterBoxCurrentAlpha - LetterBoxTargetAlpha) < 0.01f)
		{
			LetterBoxCurrentAlpha = LetterBoxTargetAlpha;

			// If blended out completely, mark as inactive
			if (LetterBoxCurrentAlpha == 0.0f)
			{
				bIsLetterBoxActive = false;
			}
		}
	}
}

// ========== Spring Arm System ==========

void APlayerCameraManager::SetSpringArmParams(FVector Offset, float ArmLength, float InterpSpeed)
{
	SpringArmOffset = Offset;
	SpringArmLength = ArmLength;
	SpringArmInterpSpeed = InterpSpeed;
}

void APlayerCameraManager::UpdateSpringArm(float DeltaTime)
{
	// Skip SpringArm updates if camera transition is active (transition controls camera)
	if (IsCameraTransitioning())
	{
		return;
	}

	if (!bSpringArmEnabled || (!Camera && !CameraComponent) || !ViewTarget.Target)
	{
		return;
	}

	// Get target location
	FVector TargetLocation = ViewTarget.Target->GetActorLocation();

	// Skip if target is at origin (not properly initialized yet)
	if (TargetLocation.Length() < 0.01f)
	{
		return;
	}

	// Calculate desired camera location
	FVector DesiredLocation = CalculateCameraLocationForView(CurrentViewType);

	// Get current camera location (Editor or PIE mode)
	FVector CurrentLocation;
	if (Camera)
	{
		CurrentLocation = Camera->GetLocation();
	}
	else if (CameraComponent)
	{
		CurrentLocation = CameraComponent->GetWorldLocation();
	}
	else
	{
		return;
	}

	// Lerp camera toward desired location
	FVector NewLocation = Lerp(CurrentLocation, DesiredLocation, SpringArmInterpSpeed * DeltaTime);

	// Spring Arm Collision Test
	if (bEnableCollisionTest)
	{
		// Get World and Level
		UWorld* World = GWorld;
		if (World && World->GetLevel())
		{
			// LineTrace from Target to Desired Camera Location
			FVector LineStart = TargetLocation;
			FVector LineEnd = DesiredLocation;
			FVector TraceDirection = LineEnd - LineStart;
			float DesiredDistance = TraceDirection.Length();

			if (DesiredDistance < 0.01f)
			{
				// Camera too close to target, skip collision test
				if (Camera)
					Camera->SetLocation(NewLocation);
				else if (CameraComponent)
					CameraComponent->SetWorldLocation(NewLocation);
				return;
			}

			TraceDirection.Normalize();

			float MinHitDistance = DesiredDistance;
			bool bHitDetected = false;

			// Test collision with ShapeComponents only (AABB-based)
			const TArray<UShapeComponent*>& ShapeComponents = World->GetLevel()->GetShapeComponents();

			for (UShapeComponent* ShapeComp : ShapeComponents)
			{
				if (!ShapeComp)
				{
					continue;
				}

				// Skip if this is the target's own collision component
				AActor* CompOwner = ShapeComp->GetOwner();
				if (CompOwner == ViewTarget.Target)
				{
					continue;
				}

				// Skip enemy collision components
				if (Cast<AEnemyCharacter>(CompOwner))
				{
					continue;
				}

				// Get component's World AABB
				FVector BoxMin, BoxMax;
				ShapeComp->GetWorldAABB(BoxMin, BoxMax);

				// Line-AABB intersection test (Slab method)
				float tMin = 0.0f;
				float tMax = DesiredDistance;
				bool bIntersects = true;

				// Test X axis
				if (fabsf(TraceDirection.X) < 0.0001f)
				{
					if (LineStart.X < BoxMin.X || LineStart.X > BoxMax.X)
						bIntersects = false;
				}
				else
				{
					float t1 = (BoxMin.X - LineStart.X) / TraceDirection.X;
					float t2 = (BoxMax.X - LineStart.X) / TraceDirection.X;
					if (t1 > t2) { float temp = t1; t1 = t2; t2 = temp; }
					tMin = max(tMin, t1);
					tMax = min(tMax, t2);
					if (tMin > tMax) bIntersects = false;
				}

				// Test Y axis
				if (bIntersects)
				{
					if (fabsf(TraceDirection.Y) < 0.0001f)
					{
						if (LineStart.Y < BoxMin.Y || LineStart.Y > BoxMax.Y)
							bIntersects = false;
					}
					else
					{
						float t1 = (BoxMin.Y - LineStart.Y) / TraceDirection.Y;
						float t2 = (BoxMax.Y - LineStart.Y) / TraceDirection.Y;
						if (t1 > t2) { float temp = t1; t1 = t2; t2 = temp; }
						tMin = max(tMin, t1);
						tMax = min(tMax, t2);
						if (tMin > tMax) bIntersects = false;
					}
				}

				// Test Z axis
				if (bIntersects)
				{
					if (fabsf(TraceDirection.Z) < 0.0001f)
					{
						if (LineStart.Z < BoxMin.Z || LineStart.Z > BoxMax.Z)
							bIntersects = false;
					}
					else
					{
						float t1 = (BoxMin.Z - LineStart.Z) / TraceDirection.Z;
						float t2 = (BoxMax.Z - LineStart.Z) / TraceDirection.Z;
						if (t1 > t2) { float temp = t1; t1 = t2; t2 = temp; }
						tMin = max(tMin, t1);
						tMax = min(tMax, t2);
						if (tMin > tMax) bIntersects = false;
					}
				}

				// Check if intersection exists
				if (bIntersects && tMin <= tMax && tMax >= 0.0f && tMin < MinHitDistance)
				{
					MinHitDistance = max(tMin, 0.0f);
					bHitDetected = true;
				}
			}

			// If collision detected, pull camera closer
			if (bHitDetected)
			{
				// Calculate safe camera position
				float SafeDistance = max(MinHitDistance - 10.0f, 0.0f);  // 10 units padding from collision
				FVector SafeLocation = LineStart + TraceDirection * SafeDistance;

				// Blend to safe location
				NewLocation = Lerp(CurrentLocation, SafeLocation, SpringArmInterpSpeed * DeltaTime * 2.0f);  // Faster pull on collision
			}
		}
	}

	// Set camera location (Editor or PIE mode)
	if (Camera)
	{
		Camera->SetLocation(NewLocation);

		// Calculate camera rotation (look at target)
		FRotator DesiredRotation = CalculateCameraRotationForView(CurrentViewType);
		FRotator CurrentRotation = Camera->GetRotationRotator();
		FRotator NewRotation = Lerp(CurrentRotation, DesiredRotation, SpringArmInterpSpeed * DeltaTime);

		Camera->SetRotation(NewRotation);
	}
	else if (CameraComponent)
	{
		// PIE mode: Directly set CameraComponent world location
		// This overrides the default spring arm behavior for collision handling
		CameraComponent->SetWorldLocation(NewLocation);

		// Rotation is handled by CameraComponent's default behavior in PIE mode
	}
}

FVector APlayerCameraManager::CalculateCameraLocationForView(ECameraViewType ViewType)
{
	if (!ViewTarget.Target)
	{
		return Camera->GetLocation();
	}

	FVector TargetLocation = ViewTarget.Target->GetActorLocation();

	switch (ViewType)
	{
	case ECameraViewType::ThirdPerson:
	{
		// Position camera behind and above the target
		FQuaternion TargetRotation = ViewTarget.Target->GetActorRotation();
		FMatrix RotMatrix = FMatrix::RotationMatrix(TargetRotation);

		// Extract Forward, Right, Up vectors from rotation matrix
		FVector ForwardVector(RotMatrix.Data[0][0], RotMatrix.Data[0][1], RotMatrix.Data[0][2]);
		FVector RightVector(RotMatrix.Data[1][0], RotMatrix.Data[1][1], RotMatrix.Data[1][2]);
		FVector UpVector(RotMatrix.Data[2][0], RotMatrix.Data[2][1], RotMatrix.Data[2][2]);

		return TargetLocation
			+ ForwardVector * SpringArmOffset.X
			+ RightVector * SpringArmOffset.Y
			+ UpVector * SpringArmOffset.Z;
	}

	case ECameraViewType::FirstPerson:
	{
		// Camera at target location (cockpit view)
		return TargetLocation + FVector(0, 0, 50.0f);  // Slightly above
	}

	case ECameraViewType::DeathCam:
	{
		// Camera further back and higher for death view
		FQuaternion TargetRotation = ViewTarget.Target->GetActorRotation();
		FMatrix RotMatrix = FMatrix::RotationMatrix(TargetRotation);
		FVector ForwardVector(RotMatrix.Data[0][0], RotMatrix.Data[0][1], RotMatrix.Data[0][2]);
		return TargetLocation + ForwardVector * -500.0f + FVector(0, 0, 200.0f);
	}

	case ECameraViewType::Cinematic:
	case ECameraViewType::FreeCam:
	default:
		// Keep current camera location
		return Camera->GetLocation();
	}
}

FRotator APlayerCameraManager::CalculateCameraRotationForView(ECameraViewType ViewType)
{
	if (!ViewTarget.Target)
	{
		return Camera->GetRotationRotator();
	}

	FVector CameraLocation = Camera->GetLocation();
	FVector TargetLocation = ViewTarget.Target->GetActorLocation();

	switch (ViewType)
	{
	case ECameraViewType::ThirdPerson:
	{
		// Look at target - convert direction to FRotator
		FVector Direction = (TargetLocation - CameraLocation).GetNormalized();

		// Calculate Yaw and Pitch from direction vector
		float Yaw = atan2f(Direction.Y, Direction.X) * (180.0f / 3.14159265359f);
		float Pitch = asinf(-Direction.Z) * (180.0f / 3.14159265359f);

		return FRotator(Pitch, Yaw, 0.0f);
	}

	case ECameraViewType::FirstPerson:
	{
		// Use target rotation (look where target is looking)
		FQuaternion TargetQuat = ViewTarget.Target->GetActorRotation();
		FVector Euler = TargetQuat.ToEuler();  // Returns (Roll, Pitch, Yaw)
		return FRotator(Euler.Y, Euler.Z, Euler.X);  // FRotator(Pitch, Yaw, Roll)
	}

	case ECameraViewType::DeathCam:
	{
		// Look at target from death cam position
		FVector Direction = (TargetLocation - CameraLocation).GetNormalized();

		float Yaw = atan2f(Direction.Y, Direction.X) * (180.0f / 3.14159265359f);
		float Pitch = asinf(-Direction.Z) * (180.0f / 3.14159265359f);

		return FRotator(Pitch, Yaw, 0.0f);
	}

	case ECameraViewType::Cinematic:
	case ECameraViewType::FreeCam:
	default:
		// Keep current camera rotation
		return Camera->GetRotationRotator();
	}
}

// ========== Camera View Type ==========

void APlayerCameraManager::TransitionToView(ECameraViewType NewView, float Duration)
{
	if (CurrentViewType == NewView)
	{
		return;
	}

	PreviousViewType = CurrentViewType;
	CurrentViewType = NewView;
	ViewTransitionDuration = Duration;
	ViewTransitionTimeRemaining = Duration;
	bIsTransitioning = true;
}

void APlayerCameraManager::UpdateViewTransition(float DeltaTime)
{
	if (!bIsTransitioning)
	{
		return;
	}

	ViewTransitionTimeRemaining -= DeltaTime;

	if (ViewTransitionTimeRemaining <= 0.0f)
	{
		ViewTransitionTimeRemaining = 0.0f;
		bIsTransitioning = false;
	}

	// TODO: Implement smooth transition between view types
	// For now, just snap to new view type
}

// ========== Camera Modifier System ==========

void APlayerCameraManager::AddCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier)
	{
		return;
	}

	Modifier->Initialize(this);
	ModifierList.push_back(Modifier);

	// Sort by priority (lower priority = applied first)
	std::sort(ModifierList.begin(), ModifierList.end(),
		[](UCameraModifier* A, UCameraModifier* B)
		{
			return A->GetPriority() < B->GetPriority();
		});
}

void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	auto it = std::find(ModifierList.begin(), ModifierList.end(), Modifier);
	if (it != ModifierList.end())
	{
		ModifierList.erase(it);
	}
}

void APlayerCameraManager::ClearAllModifiers()
{
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			delete Modifier;
		}
	}
	ModifierList.clear();
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FVector& CameraLocation, FRotator& CameraRotation)
{
	// Apply all camera modifiers (shake, etc.)
	for (size_t i = 0; i < ModifierList.size(); i++)
	{
		UCameraModifier* Modifier = ModifierList[i];
		if (Modifier && !Modifier->IsDisabled())
		{
			Modifier->UpdateModifier(DeltaTime);
			Modifier->ModifyCamera(DeltaTime, CameraLocation, CameraRotation);
		}
	}
}

// ========== Camera Shake ==========

void APlayerCameraManager::StartCameraShake(float Intensity, float Duration)
{
	// Find the Camera Shake Modifier
	UCameraModifier_CameraShake* ShakeMod = FindModifier<UCameraModifier_CameraShake>();
	if (ShakeMod)
	{
		ShakeMod->StartShake(Intensity, Duration);
	}
}

// ========== Camera Transition System ==========

void APlayerCameraManager::StartTransitionToLocation(
	const FVector& TargetLocation,
	const FRotator& TargetRotation,
	float Duration,
	ECameraEaseType EaseType,
	float TargetFOV,
	const float* BezierCP)
{
	// Get or create transition modifier
	UCameraModifier_CameraTransition* TransitionModifier = FindModifier<UCameraModifier_CameraTransition>();
	if (!TransitionModifier)
	{
		TransitionModifier = NewObject<UCameraModifier_CameraTransition>();
		TransitionModifier->Initialize(this);
		AddCameraModifier(TransitionModifier);
	}

	// Start transition through modifier
	TransitionModifier->StartTransitionToLocation(TargetLocation, TargetRotation, Duration, EaseType, TargetFOV, BezierCP);
}

void APlayerCameraManager::StartTransitionToActor(
	AActor* TargetActor,
	float Duration,
	ECameraEaseType EaseType,
	const FVector& Offset,
	const float* BezierCP)
{
	// Get or create transition modifier
	UCameraModifier_CameraTransition* TransitionModifier = FindModifier<UCameraModifier_CameraTransition>();
	if (!TransitionModifier)
	{
		TransitionModifier = NewObject<UCameraModifier_CameraTransition>();
		TransitionModifier->Initialize(this);
		AddCameraModifier(TransitionModifier);
	}

	// Start transition through modifier
	TransitionModifier->StartTransitionToActor(TargetActor, Duration, EaseType, Offset, BezierCP);
}

void APlayerCameraManager::StopCameraTransition()
{
	UCameraModifier_CameraTransition* TransitionModifier = FindModifier<UCameraModifier_CameraTransition>();
	if (TransitionModifier)
	{
		TransitionModifier->StopTransition();
	}
}

bool APlayerCameraManager::IsCameraTransitioning() const
{
	UCameraModifier_CameraTransition* TransitionModifier = const_cast<APlayerCameraManager*>(this)->FindModifier<UCameraModifier_CameraTransition>();
	if (TransitionModifier)
	{
		return TransitionModifier->IsCameraTransitioning();
	}
	return false;
}

float APlayerCameraManager::GetTransitionProgress() const
{
	UCameraModifier_CameraTransition* TransitionModifier = const_cast<APlayerCameraManager*>(this)->FindModifier<UCameraModifier_CameraTransition>();
	if (TransitionModifier)
	{
		return TransitionModifier->GetTransitionProgress();
	}
	return 0.0f;
}

void APlayerCameraManager::SetTransitionBezierControlPoints(const float CP[4])
{
	UCameraModifier_CameraTransition* TransitionModifier = FindModifier<UCameraModifier_CameraTransition>();
	if (TransitionModifier)
	{
		TransitionModifier->SetBezierControlPoints(CP);
	}
}

const float* APlayerCameraManager::GetTransitionBezierControlPoints() const
{
	UCameraModifier_CameraTransition* TransitionModifier = const_cast<APlayerCameraManager*>(this)->FindModifier<UCameraModifier_CameraTransition>();
	if (TransitionModifier)
	{
		return TransitionModifier->GetBezierControlPoints();
	}

	// Return default if no modifier exists yet
	static const float DefaultBezierCP[4] = { 0.250f, 0.460f, 0.450f, 0.940f };
	return DefaultBezierCP;
}
