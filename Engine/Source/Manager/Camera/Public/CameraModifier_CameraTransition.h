#pragma once
#include "Manager/Camera/Public/CameraModifier.h"
#include "Global/Function.h"

// Forward declarations
class AActor;

// Include for FViewTarget definition (needed for member variables)
#ifndef FVIEWTARGET_DEFINED
#define FVIEWTARGET_DEFINED
struct FViewTarget
{
	AActor* Target = nullptr;
	FVector Location = FVector::Zero();
	FRotator Rotation = FRotator(0.0f, 0.0f, 0.0f);
	float FOV = 90.0f;
};
#endif

/**
 * @brief Camera Transition Modifier
 * 카메라를 특정 위치/회전으로 부드럽게 전환
 * Bezier 곡선을 사용한 고급 Easing 지원
 */
class UCameraModifier_CameraTransition : public UCameraModifier
{
	DECLARE_CLASS(UCameraModifier_CameraTransition, UCameraModifier)

public:
	UCameraModifier_CameraTransition();
	virtual ~UCameraModifier_CameraTransition() = default;

	/**
	 * @brief 특정 위치/회전으로 카메라 전환 시작
	 * @param TargetLocation 목표 위치
	 * @param TargetRotation 목표 회전
	 * @param Duration 전환 시간 (초)
	 * @param EaseType Easing 타입
	 * @param TargetFOV 목표 FOV (-1이면 변경하지 않음)
	 * @param BezierCP Bezier 컨트롤 포인트 (EaseType이 Bezier일 때만 사용)
	 */
	void StartTransitionToLocation(
		const FVector& TargetLocation,
		const FRotator& TargetRotation,
		float Duration = 1.0f,
		ECameraEaseType EaseType = ECameraEaseType::EaseInOut,
		float TargetFOV = -1.0f,
		const float* BezierCP = nullptr
	);

	/**
	 * @brief 특정 액터로 카메라 전환 시작
	 * @param TargetActor 목표 액터
	 * @param Duration 전환 시간 (초)
	 * @param EaseType Easing 타입
	 * @param Offset 액터 위치로부터의 오프셋
	 * @param BezierCP Bezier 컨트롤 포인트
	 */
	void StartTransitionToActor(
		AActor* TargetActor,
		float Duration = 1.0f,
		ECameraEaseType EaseType = ECameraEaseType::EaseInOut,
		const FVector& Offset = FVector::Zero(),
		const float* BezierCP = nullptr
	);

	/**
	 * @brief 카메라 전환 즉시 중단
	 */
	void StopTransition();

	/**
	 * @brief 카메라가 전환 중인지 확인
	 */
	bool IsCameraTransitioning() const { return bIsTransitioning; }

	/**
	 * @brief 전환 진행도 반환 (0.0 ~ 1.0)
	 */
	float GetTransitionProgress() const;

	// UCameraModifier override
	virtual bool ModifyCamera(float DeltaTime, FVector& CameraLocation, FRotator& CameraRotation) override;
	virtual void UpdateModifier(float DeltaTime) override;

public:
	// ========== Bezier Curve Settings ==========

	/** Bezier curve control points (default: easeOutQuad) */
	float BezierCP[4] = { 0.250f, 0.460f, 0.450f, 0.940f };

	/**
	 * @brief Bezier 컨트롤 포인트 설정
	 */
	void SetBezierControlPoints(const float CP[4])
	{
		BezierCP[0] = CP[0];
		BezierCP[1] = CP[1];
		BezierCP[2] = CP[2];
		BezierCP[3] = CP[3];
	}

	/**
	 * @brief Bezier 컨트롤 포인트 반환
	 */
	const float* GetBezierControlPoints() const { return BezierCP; }

private:
	/** 전환 시작 뷰 정보 */
	FViewTarget TransitionStartView;

	/** 전환 목표 뷰 정보 */
	FViewTarget TransitionTargetView;

	/** 전환 진행 시간 */
	float TransitionTimer = 0.0f;

	/** 전환 지속 시간 */
	float TransitionDuration = 1.0f;

	/** Easing 타입 */
	ECameraEaseType TransitionEaseType = ECameraEaseType::EaseInOut;

	/** 전환 중인지 여부 */
	bool bIsTransitioning = false;

	/** 목표 액터 (액터 추적 모드일 때) */
	AActor* TargetActor = nullptr;

	/** 액터로부터의 오프셋 */
	FVector ActorOffset = FVector::Zero();
};
