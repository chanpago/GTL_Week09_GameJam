#pragma once
#include "Actor/Public/Actor.h"

class UBillBoardComponent;

/**
 * @brief 폭발 이펙트 액터 클래스
 * SubUV 애니메이션을 재생하고 자동으로 삭제됨
 */
UCLASS()
class AExplosionActor : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(AExplosionActor, AActor)

public:
    AExplosionActor();
    virtual ~AExplosionActor();

    // Actor lifecycle
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 폭발 설정
    void SetExplosionScale(float InScale) { ExplosionScale = InScale; }

private:
    UBillBoardComponent* ExplosionBillboard = nullptr;
    float Lifetime = 0.0f;  // 현재 생존 시간
    float MaxLifetime = 1.0f;  // 최대 생존 시간 (1초)
    float ExplosionScale = 1.0f;  // 폭발 크기
};