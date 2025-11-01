#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"
#include "Global/Matrix.h"

struct FCapsuleVolume : public IBoundingVolume
{
    FVector Center;          // 월드 중심
    FVector Axis;            // 월드 축(정규화)
    float CapsuleHalfHeight; // 원기둥 절반 높이(축 방향)
    float CapsuleRadius;     // 반지름
    FMatrix ScaleRotation;   // 회전/스케일(축 방향 포함)

    FCapsuleVolume()
        : Center(FVector::Zero()), Axis(FVector::UnitZ()), CapsuleHalfHeight(50.f), CapsuleRadius(20.f),
        ScaleRotation(FMatrix::Identity())
    {
    }

    bool RaycastHit() const override { return false; }
    EBoundingVolumeType GetType() const override { return EBoundingVolumeType::Capsule; }
};