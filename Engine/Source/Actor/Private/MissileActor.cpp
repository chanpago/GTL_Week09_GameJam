#include "pch.h"
#include "Actor/Public/MissileActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "Player/Public/EnemyCharacter.h"
#include "Player/Public/PlayerCharacter.h"
#include "Actor/Public/ExplosionActor.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Core/Public/AudioEngine.h"

IMPLEMENT_CLASS(AMissileActor, AActor)

AMissileActor::AMissileActor()
{
	bCanEverTick = true;
	// 메쉬 컴포넌트 생성
	StaicMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	if (StaicMeshComponent)
	{
		SetRootComponent(StaicMeshComponent);
		StaicMeshComponent->SetStaticMesh("Data/Missile.obj");
		StaicMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		StaicMeshComponent->SetRelativeRotation(FQuaternion::FromEuler(FVector(0.0f, 0.0f, 0.0f)));
		StaicMeshComponent->SetRelativeScale3D(FVector(50.f, 200.f, 200.f));
		StaicMeshComponent->SetCanEverTick(false);
		StaicMeshComponent->bGenerateOverlapEvents = false;
	}
	// 구체 충돌 컴포넌트 생성
	MissileCollision = CreateDefaultSubobject<USphereComponent>();
	if (MissileCollision)
	{
		MissileCollision->AttachToComponent(StaicMeshComponent);
		MissileCollision->SetSphereRadius(0.03f);
		MissileCollision->SetRelativeLocation(FVector(0.45f, 0.0f, 0.0f));
		MissileCollision->bGenerateHitEvents = true;
		MissileCollision->bBlockComponent = true;  // ✅ Hit 이벤트를 위해 Blocking 활성화
		MissileCollision->SetCanEverTick(true);
		MissileCollision->OnComponentHit.AddDynamic(this, &AMissileActor::OnMissileHit);
	}
}

AMissileActor::~AMissileActor()
{
}

void AMissileActor::BeginPlay()
{
	Super::BeginPlay();

	// 플래그 초기화
	bHasHitEnemy = false;

	if (MissileCollision)
	{
		RegisterComponent(MissileCollision);
	}
}

void AMissileActor::OnMissileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Null 체크
	if (!OtherActor)
	{
		return;
	}

	// 자기 자신과의 충돌 무시
	if (OtherActor == this)
	{
		return;
	}

	// 다른 미사일과의 충돌 무시
	if (Cast<AMissileActor>(OtherActor))
	{
		return;
	}

	// 플레이어와의 충돌 무시
	if (Cast<APlayerCharacter>(OtherActor))
	{
		return;
	}

	// 적 캐릭터인 경우 데미지 처리
	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(OtherActor);
	if (EnemyChar != nullptr && !bHasHitEnemy)
	{
		// 중복 히트 방지
		bHasHitEnemy = true;

		// 폭발 위치 저장 (적 캐릭터 위치)
		FVector ExplosionLocation = EnemyChar->GetActorLocation();

		// C++에서 직접 TakeDamage 호출
		float MissileDamage = 100.0f;
		EnemyChar->TakeDamage(MissileDamage);

		// Hit Stop 발동 (0.5초)
		//UTimeManager::GetInstance().StartHitStop(0.1f);

		// 폭발음 재생
		FAudioEngine::GetInstance().PlaySFX("Data/Audio/Explosion.wav", 1.0f);

		// 폭발 이펙트 생성
		if (GWorld && GWorld->GetLevel())
		{
			AExplosionActor* Explosion = NewObject<AExplosionActor>();
			if (Explosion)
			{
				Explosion->SetActorLocation(ExplosionLocation);
				Explosion->SetExplosionScale(300.0f); // 폭발 크기 설정
				GWorld->GetLevel()->AddActorToLevel(Explosion);
				Explosion->BeginPlay();
			}
		}

		// 미사일을 화면 밖으로 이동 (Lua에서 비활성화 처리)
		SetActorLocation(FVector(0, 0, -10000));
	}
}
