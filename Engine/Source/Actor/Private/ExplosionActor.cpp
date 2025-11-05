#include "pch.h"
#include "Actor/Public/ExplosionActor.h"
#include "Component/Public/BillBoardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Texture.h"
#include "Level/Public/World.h"
#include "Manager/Time/Public/TimeManager.h"

IMPLEMENT_CLASS(AExplosionActor, AActor)

AExplosionActor::AExplosionActor()
{
    bCanEverTick = true;

    // 빌보드 컴포넌트 생성
    ExplosionBillboard = CreateDefaultSubobject<UBillBoardComponent>();
    if (ExplosionBillboard)
    {
        SetRootComponent(ExplosionBillboard);
        ExplosionBillboard->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
        ExplosionBillboard->SetRelativeRotation(FQuaternion::Identity());
        ExplosionBillboard->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
    }
}

AExplosionActor::~AExplosionActor()
{
}

void AExplosionActor::BeginPlay()
{
    Super::BeginPlay();

    if (ExplosionBillboard)
    {
        // Boom.png 텍스처 로드
        UTexture* BoomTexture = UAssetManager::GetInstance().LoadTexture("Data/Texture/Boom.png");
        if (BoomTexture)
        {
            ExplosionBillboard->SetSprite(BoomTexture);

            // SubUV 애니메이션 설정 (6x6 그리드, 36fps)
            ExplosionBillboard->SetSubUVAnimation(
                true,   // 활성화
                6,      // 6열
                6,      // 6행
                36.0f   // 초당 36프레임 (36프레임 = 1초)
            );

            // 상대 시간 모드 활성화 (독립적인 애니메이션)
            ExplosionBillboard->SetUseRelativeTime(true);
            ExplosionBillboard->SetSpawnTime(UTimeManager::GetInstance().GetGameTime());

            // 폭발 크기 적용
            ExplosionBillboard->SetRelativeScale3D(FVector(ExplosionScale, ExplosionScale, ExplosionScale));
        }

        RegisterComponent(ExplosionBillboard);
    }

    // 타이머 초기화
    Lifetime = 0.0f;
}

void AExplosionActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 생존 시간 증가
    Lifetime += DeltaTime;

    // 최대 생존 시간 초과 시 삭제
    if (Lifetime >= MaxLifetime)
    {
        GWorld->DestroyActor(this);
    }
}