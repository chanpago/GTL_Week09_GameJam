#include "pch.h"
#include "Pawn/Public/Pawn.h"
#include "Player/Public/PlayerCharacter.h"
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Collision/Public/ShapeComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Core/Public/AudioEngine.h"

IMPLEMENT_CLASS(APlayerCharacter, APawn)

APlayerCharacter::APlayerCharacter()
{
	UE_LOG("===========================================");
	UE_LOG("[PlayerCharacter] CONSTRUCTOR CALLED!");
	UE_LOG("===========================================");

	bCanEverTick = true;
	MovementSpeed = 100.0f;
	this->AddTag("Player");

	// StaticMesh 추가
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	if (!StaticMeshComponent)
	{
		UE_LOG_ERROR("ACharacter: Failed to create StaticMeshComponent");
	}
	else
	{
		SetRootComponent(StaticMeshComponent);
		// Mesh 설정 (구체로 표시)
		StaticMeshComponent->SetStaticMesh("Data/MIG_29.obj");
		StaticMeshComponent->SetRelativeScale3D(FVector(10.0f, 10.0f, 10.0f));  // 크기 조정
	}

	// 구체 충돌 컴포넌트
	CollisionComponent = CreateDefaultSubobject<USphereComponent>();
	if (!CollisionComponent)
	{
		UE_LOG_ERROR("APlayerCharacter: Failed to create CollisionComponent");
	}
	else
	{
		CollisionComponent->AttachToComponent(StaticMeshComponent);
		CollisionComponent->bGenerateHitEvents = true;
		CollisionComponent->bGenerateOverlapEvents = true;
		CollisionComponent->bBlockComponent = true;
		CollisionComponent->SetRelativeLocation(FVector(-2.0f, 0.0f, 2.2f));
		CollisionComponent->SetSphereRadius(3.0f);  // 전투기 전체를 커버하는 구체
		// 충돌 콜백 등록
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::OnBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &APlayerCharacter::OnEndOverlap);
		CollisionComponent->OnComponentHit.AddDynamic(this, &APlayerCharacter::OnHit);
		UE_LOG("[PlayerCharacter] SphereCollision created");
	}

	// ✅ Create Camera Component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>();
	if (!CameraComponent)
	{
		UE_LOG_ERROR("APlayerCharacter: Failed to create CameraComponent");
	}
	else
	{
		// Attach camera to static mesh
		CameraComponent->AttachToComponent(StaticMeshComponent);

		// Set camera behind and above the player (third-person view)
		CameraComponent->SetRelativeLocation(FVector(-30.0f, 0.0f, 10.0f));

		// Configure camera properties
		CameraComponent->SetFieldOfView(90.0f);
		CameraComponent->SetAspectRatio(16.0f / 9.0f);
		CameraComponent->SetNearClipPlane(1.0f);
		CameraComponent->SetFarClipPlane(10000.0f);

		UE_LOG("[PlayerCharacter] CameraComponent created and configured");
	}

	// ✅ Create PointLight1 (attach to StaticMeshComponent, scale down by 10)
	PointLight1 = CreateDefaultSubobject<UPointLightComponent>();
	if (PointLight1)
	{
		PointLight1->AttachToComponent(StaticMeshComponent);
		PointLight1->SetRelativeLocation(FVector(-6.0f , -0.8f , 1.5f ));
		PointLight1->SetIntensity(20.0f);
		PointLight1->SetAttenuationRadius(200.8f);
		PointLight1->SetDistanceFalloffExponent(16.0f);
		PointLight1->SetLightColor(FVector(1.0f, 0.2549f, 0.0f));  // RGB (255, 65, 0)
		PointLight1->SetVisible(true);
		PointLight1->SetLightEnabled(false);
		UE_LOG("[PlayerCharacter] PointLight1 created in Constructor");
	}

	// ✅ Create PointLight2 (attach to StaticMeshComponent, scale down by 10)
	PointLight2 = CreateDefaultSubobject<UPointLightComponent>();
	if (PointLight2)
	{
		PointLight2->AttachToComponent(StaticMeshComponent);
		PointLight2->SetRelativeLocation(FVector(-6.0f, 0.8f , 1.5f));
		PointLight2->SetIntensity(20.0f);
		PointLight2->SetAttenuationRadius(200.8f);
		PointLight2->SetDistanceFalloffExponent(16.0f);
		PointLight2->SetLightColor(FVector(1.0f, 0.2549f, 0.0f));  // RGB (255, 65, 0)
		PointLight2->SetVisible(true);
		PointLight2->SetLightEnabled(false);
		UE_LOG("[PlayerCharacter] PointLight2 created in Constructor");
	}

	// ✅ Lua 스크립트 활성화 (BeginPlay에서 PlayerWeapon 로드)
	SetUseScript(true);

	UE_LOG("[PlayerCharacter] Constructor: RootComponent=%p, MeshComponent=%p", CollisionComponent, StaticMeshComponent);
}

APlayerCharacter::~APlayerCharacter()
{
}

void APlayerCharacter::BeginPlay()
{
	UE_LOG("===========================================");
	UE_LOG("[PlayerCharacter] BEGINPLAY CALLED!");
	UE_LOG("===========================================");

	// Lua 스크립트 초기화 (Super::BeginPlay 전에!)
	ULuaScriptComponent* LuaComp = GetLuaScriptComponent();
	if (!LuaComp)
	{
		UE_LOG("[PlayerCharacter] Initializing LuaScriptComponent...");
		InitLuaScriptComponent();
		LuaComp = GetLuaScriptComponent();
	}

	if (LuaComp)
	{
		UE_LOG("[PlayerCharacter] Setting script name to PlayerCharacter.lua...");
		LuaComp->SetScriptName(FString("Scripts/Player/PlayerCharacter.lua"));
		UE_LOG("[PlayerCharacter] PlayerCharacter.lua script name set!");
	}
	else
	{
		UE_LOG_ERROR("[PlayerCharacter] Failed to get LuaScriptComponent!");
	}

	// 이제 Super::BeginPlay 호출 (이미 스크립트가 설정되어 있음)
	Super::BeginPlay();

	// 충돌 컴포넌트를 Level에 등록
	if (CollisionComponent)
	{
		RegisterComponent(CollisionComponent);
		UE_LOG("[PlayerCharacter] CollisionComponent registered to Level");
	}

	// 포인트 라이트 컴포넌트를 Level에 등록
	// InitializeComponents에서 생성되므로 여기서 등록
	if (PointLight1)
	{
		RegisterComponent(PointLight1);
		UE_LOG("[PlayerCharacter] PointLight1 registered to Level (Intensity=%.2f, Enabled=%d)",
			PointLight1->GetIntensity(), PointLight1->GetLightEnabled());
	}
	if (PointLight2)
	{
		RegisterComponent(PointLight2);
		UE_LOG("[PlayerCharacter] PointLight2 registered to Level (Intensity=%.2f, Enabled=%d)",
			PointLight2->GetIntensity(), PointLight2->GetLightEnabled());
	}

	UE_LOG("[PlayerCharacter] BeginPlay: %s at (%.1f, %.1f, %.1f)",
		GetName().ToString().c_str(),
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);

	UE_LOG("===========================================");
	UE_LOG("[PlayerCharacter] BeginPlay COMPLETE!");
	UE_LOG("===========================================");
	//RegisterComponent(CollisionComponent);
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Jet sound 쿨타임 감소
	if (JetSoundCooldown > 0.0f)
	{
		JetSoundCooldown -= DeltaTime;
	}

	// 이전 프레임 입력값 저장
	float PreviousInputValue = InputValue;

	// 현재 프레임 입력값 리셋 (MoveForward에서 업데이트됨)
	InputValue = 0.0f;

	// 가속도 기반 이동 처리 (이전 프레임 입력값 사용)
	if (PreviousInputValue != 0.0f)
	{
		// Actor의 Forward 방향 계산
		FQuaternion Rotation = GetActorRotation();
		FMatrix RotMatrix = Rotation.ToRotationMatrix();
		FVector Forward(RotMatrix.Data[0][0], RotMatrix.Data[0][1], RotMatrix.Data[0][2]);
		Forward.Normalize();

		// 목표 속도 = 입력값 * 최대 속도
		float TargetSpeed = PreviousInputValue * MaxSpeed;

		// 현재 전진 방향 속도
		float CurrentSpeed = CurrentVelocity.Dot(Forward);

		// 가속 처리
		if (CurrentSpeed < TargetSpeed)
		{
			CurrentSpeed += Acceleration * DeltaTime;
			if (CurrentSpeed > TargetSpeed)
			{
				CurrentSpeed = TargetSpeed;
			}
		}
		else if (CurrentSpeed > TargetSpeed)
		{
			CurrentSpeed -= Deceleration * DeltaTime;
			if (CurrentSpeed < TargetSpeed)
			{
				CurrentSpeed = TargetSpeed;
			}
		}

		// 속도 벡터 업데이트
		CurrentVelocity = Forward * CurrentSpeed;
	}
	else
	{
		// 입력이 없으면 감속
		float CurrentSpeed = CurrentVelocity.Length();
		if (CurrentSpeed > 0.0f)
		{
			CurrentSpeed -= Deceleration * DeltaTime;
			if (CurrentSpeed < 0.0f)
			{
				CurrentSpeed = 0.0f;
			}

			if (CurrentSpeed > 0.0f)
			{
				CurrentVelocity = CurrentVelocity.GetNormalized() * CurrentSpeed;
			}
			else
			{
				CurrentVelocity = FVector::ZeroVector();
			}
		}
	}

	// 위치 업데이트
	if (CurrentVelocity.Length() > 0.0f)
	{
		FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
		SetActorLocation(NewLocation);

		// 디버깅용 로그
		UE_LOG("Velocity: %.2f, Input: %.4f", CurrentVelocity.Length(), PreviousInputValue);
	}

	// Tick 끝에서 이동 상태에 따라 라이트 업데이트
	if (PointLight1)
	{
		PointLight1->SetLightEnabled(bIsMovingForward);
	}
	if (PointLight2)
	{
		PointLight2->SetLightEnabled(bIsMovingForward);
	}

	// 다음 프레임을 위해 리셋
	bIsMovingForward = false;

	// bWasMovingForward 업데이트: 이전 프레임에 W키가 눌렸는지 확인
	// PreviousInputValue가 0이면 W키를 떼었다는 뜻
	if (PreviousInputValue <= 0.0f)
	{
		bWasMovingForward = false;
	}
	else
	{
		bWasMovingForward = true;
	}
	// InputValue는 리셋하지 않음 - MoveForward에서만 업데이트
}

void APlayerCharacter::MoveForward(float Value)
{
	// 입력값 저장 (가속도 계산은 Tick에서 처리)
	InputValue = Value;

	// W키를 누르면 이동 상태 ON (Tick에서 라이트를 켬)
	if (Value > 0.0f)
	{
		bIsMovingForward = true;

		// W키를 처음 눌렀을 때만 소리 재생 (쿨타임이 끝났을 때만)
		if (!bWasMovingForward)
		{
			UE_LOG("[PlayerCharacter] W pressed! Cooldown: %.2f", JetSoundCooldown);

			if (JetSoundCooldown <= 0.0f)
			{
				UE_LOG("[PlayerCharacter] Playing JetSound!");
				FAudioEngine::GetInstance().PlaySFX("Data/Audio/JetSound.wav", 1.0f);
				JetSoundCooldown = JetSoundCooldownTime;  // 쿨타임 리셋
			}
			else
			{
				UE_LOG("[PlayerCharacter] Cooldown still active, not playing sound");
			}
		}

		bWasMovingForward = true;
	}
	else
	{
		bWasMovingForward = false;
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	// A/D: Roll 회전 (A = 왼쪽 기울기, D = 오른쪽 기울기)
	FQuaternion CurrentRotation = GetActorRotation();
	FVector CurrentEuler = CurrentRotation.ToEuler();

	// Roll 회전 적용 (Value already contains DeltaTime)
	float RollDelta = Value * RotationSpeed;
	CurrentEuler.X += RollDelta;  // X = Roll

	FQuaternion NewRotation = FQuaternion::FromEuler(CurrentEuler);
	SetActorRotation(NewRotation);
}

void APlayerCharacter::Turn(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	// 마우스 좌우: Yaw 회전 (왼쪽 = 마이너스, 오른쪽 = 플러스)
	FQuaternion CurrentRotation = GetActorRotation();
	FVector CurrentEuler = CurrentRotation.ToEuler();

	// Yaw 회전 적용 (Value already contains DeltaTime)
	float YawDelta = Value * MouseSensitivity;
	CurrentEuler.Z += YawDelta;  // Z = Yaw

	FQuaternion NewRotation = FQuaternion::FromEuler(CurrentEuler);
	SetActorRotation(NewRotation);
}

void APlayerCharacter::LookUp(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	// 마우스 상하: Pitch 회전 (위 = 플러스, 아래 = 마이너스)
	FQuaternion CurrentRotation = GetActorRotation();
	FVector CurrentEuler = CurrentRotation.ToEuler();

	// Pitch 회전 적용 (Value already contains DeltaTime)
	float PitchDelta = Value * MouseSensitivity;
	CurrentEuler.Y += PitchDelta;  // Y = Pitch

	FQuaternion NewRotation = FQuaternion::FromEuler(CurrentEuler);
	SetActorRotation(NewRotation);
}

void APlayerCharacter::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	// 미사일과의 충돌 무시
	FString OtherName = OtherActor->GetName().ToString();
	if (OtherName.find("AMissileActor") != std::string::npos)
	{
		return;
	}

	if (ULuaScriptComponent* LuaComp = this->GetLuaScriptComponent())
	{
		LuaComp->ActivateFunction("OnBeginOverlap", OverlappedComp, OtherActor,
			OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	}
}

void APlayerCharacter::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	// 미사일과의 충돌 무시
	FString OtherName = OtherActor->GetName().ToString();
	if (OtherName.find("AMissileActor") != std::string::npos)
	{
		return;
	}

	if (ULuaScriptComponent* LuaComp = this->GetLuaScriptComponent())
	{
		LuaComp->ActivateFunction("OnEndOverlap", OverlappedComp, OtherActor,
			OtherComp, OtherBodyIndex);
	}
}

void APlayerCharacter::OnHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& OutHit)
{
	if (!OtherActor)
	{
		return;
	}

	// 미사일과의 충돌 무시
	FString OtherName = OtherActor->GetName().ToString();
	if (OtherName.find("AMissileActor") != std::string::npos)
	{
		return;
	}

	if (ULuaScriptComponent* LuaComp = this->GetLuaScriptComponent())
	{
		LuaComp->ActivateFunction("OnHit", OverlappedComp, OtherActor,
			OtherComp, NormalImpulse, OutHit);
	}
}


