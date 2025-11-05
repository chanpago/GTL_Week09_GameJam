#include "pch.h"
#include "Editor/Public/Camera.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Manager/Config/Public/ConfigManager.h"

#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"

UCamera::UCamera() :
	CameraConstants(FCameraConstants()),
	RelativeLocation(FVector(-15.0f, 0.f, 10.0f)), RelativeRotation(FRotator(0.0f, 0.0f, 0.0f)),
	FovY(90.f), Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT),
	NearZ(0.1f), FarZ(10000.0f), OrthoWidth(90.0f), CameraType(ECameraType::ECT_Perspective)
{
	CurrentMoveSpeed = UConfigManager::GetInstance().GetCameraSensitivity();
}

UCamera::~UCamera()
{
	UConfigManager::GetInstance().SetCameraSensitivity(CurrentMoveSpeed);
}

void UCamera::SetRotation(const FVector& InOtherRotation)
{
	// FVector(Roll, Pitch, Yaw) -> FRotator(Pitch, Yaw, Roll)
	RelativeRotation.Pitch = InOtherRotation.Y;
	RelativeRotation.Yaw = InOtherRotation.Z;
	RelativeRotation.Roll = InOtherRotation.X;
}

FVector UCamera::UpdateInput()
{
	FVector MovementDelta = FVector::Zero(); // 마우스의 변화량을 반환할 객체
	const UInputManager& Input = UInputManager::GetInstance();

	/**
	 * @brief 마우스 우클릭을 하고 있는 동안 카메라 제어가 가능합니다.
	 */
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		if (CameraType == ECameraType::ECT_Perspective)
		{
			/**
			 * @brief Perspective: WASDQE 키로 이동, 마우스 드래그로 회전
			 */
			FVector Direction = FVector::Zero();

			if (Input.IsKeyDown(EKeyInput::A)) { Direction += -Right * 2; }
			if (Input.IsKeyDown(EKeyInput::D)) { Direction += Right * 2; }
			if (Input.IsKeyDown(EKeyInput::W)) { Direction += Forward * 2; }
			if (Input.IsKeyDown(EKeyInput::S)) { Direction += -Forward * 2; }
			if (Input.IsKeyDown(EKeyInput::Q)) { Direction += FVector(0, 0, -2); }
			if (Input.IsKeyDown(EKeyInput::E)) { Direction += FVector(0, 0, 2); }
			if (Direction.LengthSquared() > MATH_EPSILON)
			{
				Direction.Normalize();
			}
			RelativeLocation += Direction * CurrentMoveSpeed * DT;

			// 마우스 드래그로 회전
			const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();

			// Yaw/Pitch 델타 계산
			const float YawDelta = MouseDelta.X * KeySensitivityDegPerPixel * 2;
			const float PitchDelta = -MouseDelta.Y * KeySensitivityDegPerPixel * 2;  // Y 반전 (위로 드래그 = 위를 봄)

			// FRotator에 직접 누적
			RelativeRotation.Yaw += YawDelta;
			RelativeRotation.Pitch += PitchDelta;
			RelativeRotation.Roll = 0.0f;

			// Pitch 클램핑 (UE 표준: -89.9 ~ 89.9)
			constexpr float MaxPitch = 89.9f;
			RelativeRotation.Pitch = clamp(RelativeRotation.Pitch, -MaxPitch, MaxPitch);

			MovementDelta = Direction * CurrentMoveSpeed * DT;
		}

		else if (CameraType == ECameraType::ECT_Orthographic)
		{
			// 오쏘 뷰: 마우스 드래그로만 패닝
			const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
			const float PanSpeed = 0.1f; // 패닝 속도 계수
			FVector PanDelta = Right * -MouseDelta.X * PanSpeed + Up * MouseDelta.Y * PanSpeed;
			RelativeLocation += PanDelta; // 좌우는 반대, 상하는 동일
			MovementDelta = PanDelta;
		}
	}

	return MovementDelta;
}

void UCamera::Update(const D3D11_VIEWPORT& InViewport, float DeltaTime)
{
	// Update smooth transition first (highest priority)
	if (bIsTransitioning)
	{
		UpdateTransition(DeltaTime);
	}
	// PIE Mode: Follow Target (불법증축!)
	else if (FollowTarget.IsValid())
	{
		FVector TargetLocation = FollowTarget->GetActorLocation();
		FQuaternion TargetRotation = FollowTarget->GetActorRotation();

		// 비행기의 로컬 좌표계 벡터 계산
		FMatrix RotMatrix = TargetRotation.ToRotationMatrix();
		FVector TargetForward(RotMatrix.Data[0][0], RotMatrix.Data[0][1], RotMatrix.Data[0][2]);
		FVector TargetRight(RotMatrix.Data[1][0], RotMatrix.Data[1][1], RotMatrix.Data[1][2]);
		FVector TargetUp(RotMatrix.Data[2][0], RotMatrix.Data[2][1], RotMatrix.Data[2][2]);

		// 로컬 공간 오프셋을 월드 공간으로 변환 (비행기 기준!)
		FVector LocalOffset = TargetForward * FollowOffset.X +
		                      TargetRight * FollowOffset.Y +
		                      TargetUp * FollowOffset.Z;

		RelativeLocation = TargetLocation + LocalOffset;

		// Camera Lag: Slerp로 부드럽게 회전 보간
		FQuaternion CurrentRotation = GetRotationQuat();
		float LagSpeed = 5.0f; // 회전 따라가기 속도 (높을수록 빠름)
		FQuaternion SmoothedRotation = FQuaternion::Slerp(CurrentRotation, TargetRotation, LagSpeed * DeltaTime);
		SetRotationQuat(SmoothedRotation);
	}
	// 에디터 모드: 입력이 활성화되어 있으면 입력 처리
	else if (bInputEnabled)
	{
		UpdateInput();
	}

	// Perspective 모드에서만 회전 행렬로 Forward/Up/Right 계산
	// Orthographic 모드에서는 ViewportClient::SetViewType에서 설정한 값 유지
	if (CameraType == ECameraType::ECT_Perspective)
	{
		// 비행기 게임: Roll 회전 포함한 완전한 회전 행렬 사용
		const FQuaternion RotationQuat = RelativeRotation.Quaternion();
		const FMatrix RotationMatrix = RotationQuat.ToRotationMatrix();

		// RotationMatrix에서 직접 Forward/Right/Up 추출 (Roll 포함!)
		Forward = FVector(RotationMatrix.Data[0][0], RotationMatrix.Data[0][1], RotationMatrix.Data[0][2]);
		Right = FVector(RotationMatrix.Data[1][0], RotationMatrix.Data[1][1], RotationMatrix.Data[1][2]);
		Up = FVector(RotationMatrix.Data[2][0], RotationMatrix.Data[2][1], RotationMatrix.Data[2][2]);

		Forward.Normalize();
		Right.Normalize();
		Up.Normalize();
	}

	// 종횡비 갱신
	if (InViewport.Width > 0.f && InViewport.Height > 0.f)
	{
		SetAspect(InViewport.Width / InViewport.Height);

		// Orthographic 모드
		if (CameraType == ECameraType::ECT_Orthographic)
		{
			const float UnitsPerPixel = GetOrthoUnitsPerPixel(InViewport.Width);
			const float CalculatedOrthoWidth = UnitsPerPixel * InViewport.Width / 2.0f;
			SetOrthoWidth(CalculatedOrthoWidth);
		}
	}

	switch (CameraType)
	{
	case ECameraType::ECT_Perspective:
		UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		UpdateMatrixByOrth();
		break;
	}

	// 카메라가 업데이트할 때마다 Cull한다.
	// 카메라가 업데이트하지 않으면 Culling을 갱신할 이유가 없다.
    ULevel* CurrentLevel = GWorld->GetLevel();
    if (CurrentLevel)
    {
        ViewVolumeCuller.Cull(
            CurrentLevel->GetStaticOctree(),
            CurrentLevel->GetDynamicPrimitives(),
            CameraConstants
        );
    }
}

void UCamera::UpdateMatrixByPers()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	CameraConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FMatrix P = FMatrix::Identity();
	// | f/aspect   0        0         0 |
	// |    0       f        0         0 |
	// |    0       0   zf/(zf-zn)     1 |
	// |    0       0  -zn*zf/(zf-zn)  0 |
	P.Data[0][0] = F / Aspect;
	P.Data[1][1] = F;
	P.Data[2][2] = FarZ / (FarZ - NearZ);
	P.Data[2][3] = 1.0f;
	P.Data[3][2] = (-NearZ * FarZ) / (FarZ - NearZ);
	P.Data[3][3] = 0.0f;

	CameraConstants.Projection = P;
	
	CameraConstants.ViewWorldLocation = RelativeLocation;
	CameraConstants.NearClip = NearZ;
	CameraConstants.FarClip = FarZ;
}

/**
 * @brief 언리얼 엔진 방식의 Ortho Units Per Pixel 계산
 * 뷰포트 크기를 정규화하여 OrthoWidth가 뷰포트 크기에 비례하도록 함
 * ComputeOrthoZoomFactor를 사용
 * r.Editor.AlignedOrthoZoom = 1 (기본값) 적용
 * Factor = ViewportWidth / 500
 * UnitsPerPixel = (OrthoZoom / (ViewportWidth * 15)) * Factor
 *               = (OrthoZoom / (ViewportWidth * 15)) * (ViewportWidth / 500)
 *               = OrthoZoom / 7500  (ViewportWidth가 약분됨!)
 * @param ViewportWidth 
 * @return 
 */
float UCamera::GetOrthoUnitsPerPixel(float ViewportWidth) const
{
	constexpr float CAMERA_ZOOM_DIV = 15.0f;
	constexpr float ORTHO_ZOOM_FACTOR_BASE = 500.0f;

	const float ZoomFactor = ViewportWidth / ORTHO_ZOOM_FACTOR_BASE;
	return (OrthoZoom / (ViewportWidth * CAMERA_ZOOM_DIV)) * ZoomFactor;
}

void UCamera::UpdateMatrixByOrth()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	CameraConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * OrthoZoom을 기반으로 뷰포트 크기에 비례하는 OrthoWidth 계산
	 * 뷰포트가 리사이즈되어도 물체의 화면상 크기가 일정하게 유지됨
	 */
	const float SafeAspect = max(0.1f, Aspect);

	// 언리얼 방식: OrthoWidth가 뷰포트 너비에 비례하도록 계산
	// 이렇게 하면 Projection Matrix의 1/OrthoWidth가 뷰포트 크기를 상쇄시킴
	// 최종 계산은 Update()에서 뷰포트 크기를 알 때 수행됨 (여기서는 OrthoWidth 사용)

	// OrthoWidth는 이미 외부(ViewportManager 또는 Update)에서 설정되었다고 가정
	const float OrthoHeight = OrthoWidth / SafeAspect;  // UE: AspectRatio로 Height 계산

	const float HalfOrthoWidth = OrthoWidth * 0.5f;
	const float HalfOrthoHeight = OrthoHeight * 0.5f;

	const float Left = -HalfOrthoWidth;
	const float Right = HalfOrthoWidth;
	const float Bottom = -HalfOrthoHeight;
	const float Top = HalfOrthoHeight;

	FMatrix P = FMatrix::Identity();
	P.Data[0][0] = 2.0f / (Right - Left);
	P.Data[1][1] = 2.0f / (Top - Bottom);
	P.Data[2][2] = 1.0f / (FarZ - NearZ);
	P.Data[3][0] = -(Right + Left) / (Right - Left);
	P.Data[3][1] = -(Top + Bottom) / (Top - Bottom);
	P.Data[3][2] = -NearZ / (FarZ - NearZ);
	P.Data[3][3] = 1.0f;
	CameraConstants.Projection = P;

	CameraConstants.ViewWorldLocation = RelativeLocation;
	CameraConstants.NearClip = NearZ;
	CameraConstants.FarClip = FarZ;
}

const FCameraConstants UCamera::GetFViewProjConstantsInverse() const
{
	/*
	* @brief View^(-1) = R * T
	*/
	FCameraConstants Result = {};
	//FMatrix R = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
	FMatrix R = FMatrix(Right, Up, Forward);
	FMatrix T = FMatrix::TranslationMatrix(RelativeLocation);
	Result.View = R * T;

	if (CameraType == ECameraType::ECT_Orthographic)
	{
		// UpdateMatrixByOrth와 동일한 계산 방식 사용
		const float SafeAspect = max(0.1f, Aspect);
		const float OrthoHeight = OrthoWidth / SafeAspect;  // AspectRatio로 Height 계산

		const float HalfOrthoWidth = OrthoWidth * 0.5f;
		const float HalfOrthoHeight = OrthoHeight * 0.5f;

		const float Left = -HalfOrthoWidth;
		const float Right = HalfOrthoWidth;
		const float Bottom = -HalfOrthoHeight;
		const float Top = HalfOrthoHeight;

		FMatrix P = FMatrix::Identity();
		// A^{-1} (대각)
		P.Data[0][0] = (Right - Left) * 0.5f;  // (r-l)/2
		P.Data[1][1] = (Top - Bottom) * 0.5f; // (t-b)/2
		P.Data[2][2] = (FarZ - NearZ);               // (zf-zn)
		// -b A^{-1} (마지막 행의 x,y,z)
		P.Data[3][0] = (Right + Left) * 0.5f;   // (r+l)/2
		P.Data[3][1] = (Top + Bottom) * 0.5f; // (t+b)/2
		P.Data[3][2] = NearZ;                      // zn
		P.Data[3][3] = 1.0f;
		Result.Projection = P;
	}
	else if ((CameraType == ECameraType::ECT_Perspective))
	{
		const float FovRadian = FVector::GetDegreeToRadian(FovY);
		const float F = 1.0f / std::tanf(FovRadian * 0.5f);
		FMatrix P = FMatrix::Identity();
		// | aspect/F   0      0         0 |
		// |    0      1/F     0         0 |
		// |    0       0      0   -(zf-zn)/(zn*zf) |
		// |    0       0      1        zf/(zn*zf)  |
		P.Data[0][0] = Aspect / F;
		P.Data[1][1] = 1.0f / F;
		P.Data[2][2] = 0.0f;
		P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
		P.Data[3][2] = 1.0f;
		P.Data[3][3] = FarZ / (NearZ * FarZ);
		Result.Projection = P;
	}

	Result.ViewWorldLocation = RelativeLocation;
	Result.NearClip = NearZ;
	Result.FarClip = FarZ;
	return Result;
}

FRay UCamera::ConvertToWorldRay(float NdcX, float NdcY) const
{
	/* *
	 * @brief 반환할 타입의 객체 선언
	 */
	FRay Ray = {};

	const FCameraConstants& ViewProjMatrix = GetFViewProjConstantsInverse();

	/* *
	 * @brief NDC 좌표 정보를 행렬로 변환합니다.
	 */
	const FVector4 NdcNear(NdcX, NdcY, 0.0f, 1.0f);
	const FVector4 NdcFar(NdcX, NdcY, 1.0f, 1.0f);

	/* *
	 * @brief Projection 행렬을 View 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 ViewNear = MultiplyPointWithMatrix(NdcNear, ViewProjMatrix.Projection);
	const FVector4 ViewFar = MultiplyPointWithMatrix(NdcFar, ViewProjMatrix.Projection);

	/* *
	 * @brief View 행렬을 World 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 WorldNear = MultiplyPointWithMatrix(ViewNear, ViewProjMatrix.View);
	const FVector4 WorldFar = MultiplyPointWithMatrix(ViewFar, ViewProjMatrix.View);

	/* *
	 * @brief 카메라의 월드 좌표를 추출합니다.
	 * Row-major 기준, 마지막 행 벡터는 위치 정보를 가지고 있음
	 */
	const FVector4 CameraPosition(
		ViewProjMatrix.View.Data[3][0],
		ViewProjMatrix.View.Data[3][1],
		ViewProjMatrix.View.Data[3][2],
		ViewProjMatrix.View.Data[3][3]);

	if (CameraType == ECameraType::ECT_Perspective)
	{
		FVector4 DirectionVector = WorldFar - CameraPosition;
		DirectionVector.Normalize();

		Ray.Origin = CameraPosition;
		Ray.Direction = DirectionVector;
	}
	else if (CameraType == ECameraType::ECT_Orthographic)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}

	return Ray;
}

FVector UCamera::CalculatePlaneNormal(const FVector4& Axis)
{
	return FVector(Axis.X, Axis.Y, Axis.Z).Cross(Forward);
}
FVector UCamera::CalculatePlaneNormal(const FVector& Axis)
{
	return FVector(Axis.X, Axis.Y, Axis.Z).Cross(Forward);
}

void UCamera::SetFollowTarget(AActor* InTarget, const FVector& InOffset)
{
	FollowTarget.Set(InTarget);
	FollowOffset = InOffset;
	UE_LOG("[Camera] Follow target set: %s with offset (%.1f, %.1f, %.1f)",
		InTarget ? InTarget->GetName().ToString().c_str() : "null",
		InOffset.X, InOffset.Y, InOffset.Z);
}

void UCamera::ClearFollowTarget()
{
	FollowTarget.Reset();
	FollowOffset = FVector::Zero();
	UE_LOG("[Camera] Follow target cleared");
}

bool UCamera::HasFollowTarget() const
{
	return FollowTarget.IsValid();
}

void UCamera::SmoothMoveTo(
	const FVector& TargetLocation,
	const FRotator& TargetRotation,
	float Duration,
	ECameraEaseType EaseType)
{
	if (Duration <= 0.0f)
	{
		// Instant transition
		RelativeLocation = TargetLocation;
		RelativeRotation = TargetRotation;
		bIsTransitioning = false;
		return;
	}

	// Store current state as start
	TransitionStartLocation = RelativeLocation;
	TransitionStartRotation = RelativeRotation;

	// Store target state
	TransitionTargetLocation = TargetLocation;
	TransitionTargetRotation = TargetRotation;

	// Initialize transition
	TransitionDuration = Duration;
	TransitionTimeRemaining = Duration;
	TransitionEaseType = EaseType;
	bIsTransitioning = true;
}

void UCamera::StopSmoothMove()
{
	bIsTransitioning = false;
	TransitionTimeRemaining = 0.0f;
}

void UCamera::UpdateTransition(float DeltaTime)
{
	if (!bIsTransitioning)
	{
		return;
	}

	TransitionTimeRemaining -= DeltaTime;

	// Calculate alpha (0 to 1)
	float RawAlpha = 1.0f - (TransitionTimeRemaining / TransitionDuration);
	RawAlpha = Clamp(RawAlpha, 0.0f, 1.0f);

	// Apply easing function
	float Alpha = ApplyEasing(RawAlpha, TransitionEaseType);

	// Interpolate location
	RelativeLocation = Lerp(TransitionStartLocation, TransitionTargetLocation, Alpha);

	// Interpolate rotation using SLERP for smooth rotation
	FQuaternion StartQuat = TransitionStartRotation.Quaternion();
	FQuaternion TargetQuat = TransitionTargetRotation.Quaternion();
	FQuaternion NewQuat = FQuaternion::Slerp(StartQuat, TargetQuat, Alpha);
	FVector EulerAngles = NewQuat.ToEuler();
	RelativeRotation = FRotator(EulerAngles.Y, EulerAngles.Z, EulerAngles.X);

	// Check if transition is complete
	if (TransitionTimeRemaining <= 0.0f)
	{
		bIsTransitioning = false;
		TransitionTimeRemaining = 0.0f;
		RelativeLocation = TransitionTargetLocation;
		RelativeRotation = TransitionTargetRotation;
	}
}
