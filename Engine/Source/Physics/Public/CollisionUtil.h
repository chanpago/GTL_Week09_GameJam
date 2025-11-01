#pragma once
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Collision/Public/BoxComponent.h"
#include "Component/Collision/Public/CapsuleComponent.h"
#include "Global/Vector.h"
#include <algorithm>  // std::max 사용

/**
 * @file CollisionUtil.h
 * @brief Narrow Phase 충돌 검사 유틸리티 함수들
 * @note 모든 충돌 검사는 월드 좌표계 기준으로 수행됩니다
 * @note Broad Phase는 옥트리에서 처리하고, 이 함수들은 정밀 검사용입니다
 */

namespace CollisionUtil
{
    /*-----------------------------------------------------------------------------
        Helper Functions (내부 사용)
    -----------------------------------------------------------------------------*/

    /**
     * @brief 세 값 중 최대값을 반환합니다
     * @param A 첫 번째 값
     * @param B 두 번째 값
     * @param C 세 번째 값
     * @return 최대값
     */
    inline float Max3(float A, float B, float C)
    {
        return std::max(std::max(A, B), C);
    }

    /*-----------------------------------------------------------------------------
        Sphere vs Sphere Collision
    -----------------------------------------------------------------------------*/

    /**
     * @brief 두 Sphere 컴포넌트가 겹치는지 정밀 검사합니다
     * @param SphereA 첫 번째 Sphere 컴포넌트
     * @param SphereB 두 번째 Sphere 컴포넌트
     * @return 겹치면 true, 아니면 false
     *
     * @details 알고리즘:
     * 1. 두 Sphere의 월드 위치(중심점)를 가져옴
     * 2. 월드 스케일을 고려하여 실제 반지름 계산
     * 3. 두 중심점 사이 거리의 제곱 계산 (sqrt 비용 절약)
     * 4. 반지름 합의 제곱과 비교
     * 5. 거리² <= (반지름A + 반지름B)² 이면 충돌
     *
     * @note 성능 최적화: 제곱 값으로 비교하여 sqrt() 연산 회피
     * @note Sphere는 균등 스케일을 가정 (X, Y, Z 중 최대값 사용)
     */
    inline bool TestOverlap(const USphereComponent* SphereA, const USphereComponent* SphereB)
    {
        // nullptr 체크
        if (!SphereA || !SphereB)
        {
            return false;
        }

        // 월드 위치 가져오기 (중심점)
        FVector CenterA = SphereA->GetWorldLocation();
        FVector CenterB = SphereB->GetWorldLocation();

        // 월드 스케일 가져오기
        FVector ScaleA = SphereA->GetWorldScale3D();
        FVector ScaleB = SphereB->GetWorldScale3D();

        // Sphere는 균등 스케일을 가정 (X, Y, Z 중 최대값 사용)
        // 이유: 비균등 스케일 시 타원체가 되므로 최대 반지름으로 보수적 판정
        float MaxScaleA = Max3(ScaleA.X, ScaleA.Y, ScaleA.Z);
        float MaxScaleB = Max3(ScaleB.X, ScaleB.Y, ScaleB.Z);

        // 스케일을 반영한 실제 반지름 계산
        float RadiusA = SphereA->GetSphereRadius() * MaxScaleA;
        float RadiusB = SphereB->GetSphereRadius() * MaxScaleB;

        // 두 중심점 사이 벡터
        FVector Delta = CenterB - CenterA;

        // 거리의 제곱 계산 (sqrt 비용 절약)
        float DistanceSquared = Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z;

        // 반지름 합의 제곱
        float RadiusSum = RadiusA + RadiusB;
        float RadiusSumSquared = RadiusSum * RadiusSum;

        // 거리² <= (반지름A + 반지름B)² 이면 충돌
        return DistanceSquared <= RadiusSumSquared;
    }

    /*-----------------------------------------------------------------------------
        Generic TestOverlap - Shape 타입에 따라 적절한 함수 호출
    -----------------------------------------------------------------------------*/

    /**
     * @brief 두 ShapeComponent의 타입을 판별하여 적절한 충돌 검사 수행
     * @param ShapeA 첫 번째 Shape 컴포넌트
     * @param ShapeB 두 번째 Shape 컴포넌트
     * @return 겹치면 true, 아니면 false
     *
     * @details
     * Cast를 통해 런타임에 타입을 판별하고 적절한 Narrow Phase 함수 호출
     * 지원하지 않는 조합은 false 반환 (향후 구현 필요)
     */
    inline bool TestOverlap(const UShapeComponent* ShapeA, const UShapeComponent* ShapeB)
    {
        if (!ShapeA || !ShapeB)
        {
            return false;
        }

        // Sphere vs Sphere
        const USphereComponent* SphereA = Cast<USphereComponent>(const_cast<UShapeComponent*>(ShapeA));
        const USphereComponent* SphereB = Cast<USphereComponent>(const_cast<UShapeComponent*>(ShapeB));

        if (SphereA && SphereB)
        {
            return TestOverlap(SphereA, SphereB);
        }

        // Box vs Box (TODO: 구현 필요)
        // const UBoxComponent* BoxA = Cast<UBoxComponent>(const_cast<UShapeComponent*>(ShapeA));
        // const UBoxComponent* BoxB = Cast<UBoxComponent>(const_cast<UShapeComponent*>(ShapeB));
        // if (BoxA && BoxB) { return TestOverlap(BoxA, BoxB); }

        // Sphere vs Box (TODO: 구현 필요)
        // ...

        // Capsule vs Capsule (TODO: 구현 필요)
        // ...

        // 지원하지 않는 조합은 false 반환
        return false;
    }

} // namespace CollisionUtil