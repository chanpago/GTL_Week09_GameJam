#pragma once

#include "Actor/Public/Actor.h"

class UCameraModifier;
class UCameraModifier_CameraTransition;
class UCamera;
class UCameraComponent;
class APawn;

/**
 * @brief Camera view types for different gameplay scenarios
 */
enum class ECameraViewType : uint8
{
	ThirdPerson,     // Default third-person follow camera
	FirstPerson,     // First-person view (cockpit)
	Cinematic,       // Cinematic camera for cutscenes
	DeathCam,        // Camera view when player dies
	FreeCam          // Free camera (spectator)
};

/**
 * @brief View target struct for camera interpolation
 */
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
 * @brief APlayerCameraManager - Manages the player's camera
 * Handles camera positioning, modifiers, fade effects, letter box, etc.
 */
class APlayerCameraManager : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(APlayerCameraManager, AActor)

public:

    /**
	 * @brief Initialize the camera manager (Editor mode)
	 * @param InCamera The camera to manage
	 */
	void Initialize(UCamera* InCamera);

	/**
	 * @brief Initialize the camera manager (PIE mode)
	 * @param InCameraComponent The camera component to manage
	 */
	void Initialize(UCameraComponent* InCameraComponent);

	/**
	 * @brief Update camera state
	 * @param DeltaTime Time since last frame
	 */
	void Tick(float DeltaTime);

	/**
	 * @brief Set the view target (what the camera looks at)
	 * @param NewTarget The actor to follow
	 */
	void SetViewTarget(AActor* NewTarget);

	/**
	 * @brief Get the current view target
	 */
	AActor* GetViewTarget() const { return ViewTarget.Target; }

	/**
	 * @brief Get the managed camera (Editor mode)
	 */
	UCamera* GetCamera() const { return Camera; }

	/**
	 * @brief Get the managed camera component (PIE/Game mode)
	 */
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	// ========== Fade System ==========

	/**
	 * @brief Start a fade effect
	 * @param Duration Fade duration in seconds
	 * @param ToColor Color to fade to
	 * @param bFadeOut true = fade to color, false = fade from color
	 * @param bHoldWhenFinished Hold the fade color when finished
	 */
	void StartCameraFade(float Duration, FVector4 ToColor = FVector4(0, 0, 0, 1), bool bFadeOut = true, bool bHoldWhenFinished = false);

	/**
	 * @brief Stop the current fade
	 */
	void StopCameraFade();

	/**
	 * @brief Get the current fade amount (0 = no fade, 1 = full fade)
	 */
	float GetFadeAmount() const { return FadeAmount; }

	/**
	 * @brief Get the current fade color
	 */
	FVector4 GetFadeColor() const { return FadeColorLinear; }

	// ========== Letter Box System ==========

	/**
	 * @brief Start letter box effect (cinematic black bars)
	 * @param Height Height of bars as fraction of screen (0.1 = 10%)
	 * @param BlendTime Time to blend in/out
	 */
	void StartLetterBox(float Height = 0.1f, float BlendTime = 0.5f);

	/**
	 * @brief Stop letter box effect
	 * @param BlendTime Time to blend out
	 */
	void StopLetterBox(float BlendTime = 0.5f);

	/**
	 * @brief Get current letter box alpha (0 = no bars, 1 = full bars)
	 */
	float GetLetterBoxAlpha() const { return LetterBoxCurrentAlpha; }

	/**
	 * @brief Get letter box height as fraction of screen
	 */
	float GetLetterBoxHeight() const { return LetterBoxHeight; }

	// ========== Spring Arm System ==========

	/**
	 * @brief Set spring arm parameters
	 * @param Offset Camera offset from target
	 * @param ArmLength Distance from target
	 * @param InterpSpeed How fast camera follows (higher = faster)
	 */
	void SetSpringArmParams(FVector Offset, float ArmLength, float InterpSpeed = 10.0f);

	/**
	 * @brief Enable/disable spring arm
	 */
	void SetSpringArmEnabled(bool bEnabled) { bSpringArmEnabled = bEnabled; }

	/**
	 * @brief Get spring arm enabled state
	 */
	bool IsSpringArmEnabled() const { return bSpringArmEnabled; }

	/**
	 * @brief Enable/disable spring arm collision test
	 * @param bEnabled true to enable collision test, false to disable
	 */
	void SetSpringArmCollisionEnabled(bool bEnabled) { bEnableCollisionTest = bEnabled; }

	/**
	 * @brief Get spring arm collision test enabled state
	 */
	bool IsSpringArmCollisionEnabled() const { return bEnableCollisionTest; }

	// ========== Camera View Type ==========

	/**
	 * @brief Transition to a different camera view
	 * @param NewView The view type to transition to
	 * @param Duration Transition duration in seconds
	 */
	void TransitionToView(ECameraViewType NewView, float Duration = 1.0f);

	/**
	 * @brief Get current camera view type
	 */
	ECameraViewType GetCurrentViewType() const { return CurrentViewType; }

	// ========== Camera Transition System (Modifier-Based) ==========

	/**
	 * @brief Start a smooth transition to a specific location and rotation
	 * @param TargetLocation Target camera location
	 * @param TargetRotation Target camera rotation
	 * @param Duration Transition duration in seconds
	 * @param EaseType Easing function type
	 * @param TargetFOV Target field of view (optional, -1 to keep current)
	 * @param BezierCP Bezier control points [4] (only used if EaseType is Bezier)
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
	 * @brief Start a smooth transition to follow an actor
	 * @param TargetActor Actor to follow
	 * @param Duration Transition duration in seconds
	 * @param EaseType Easing function type
	 * @param Offset Offset from actor (optional)
	 * @param BezierCP Bezier control points [4] (only used if EaseType is Bezier)
	 */
	void StartTransitionToActor(
		AActor* TargetActor,
		float Duration = 1.0f,
		ECameraEaseType EaseType = ECameraEaseType::EaseInOut,
		const FVector& Offset = FVector::Zero(),
		const float* BezierCP = nullptr
	);

	/**
	 * @brief Stop the current camera transition
	 */
	void StopCameraTransition();

	/**
	 * @brief Check if camera is currently transitioning
	 */
	bool IsCameraTransitioning() const;

	/**
	 * @brief Get the progress of current transition (0 to 1)
	 */
	float GetTransitionProgress() const;

	/**
	 * @brief Set custom Bezier control points for transitions
	 * @param CP Control points [4]: P1.x, P1.y, P2.x, P2.y
	 */
	void SetTransitionBezierControlPoints(const float CP[4]);

	/**
	 * @brief Get current Bezier control points
	 */
	const float* GetTransitionBezierControlPoints() const;

	// ========== Camera Modifier System ==========

	/**
	 * @brief Get all camera modifiers
	 * @return Reference to modifier list
	 */
	const TArray<UCameraModifier*>& GetModifierList() const { return ModifierList; }

	/**
	 * @brief Add a camera modifier
	 * @param Modifier The modifier to add
	 */
	void AddCameraModifier(UCameraModifier* Modifier);

	/**
	 * @brief Remove a camera modifier
	 * @param Modifier The modifier to remove
	 */
	void RemoveCameraModifier(UCameraModifier* Modifier);

	/**
	 * @brief Remove all camera modifiers
	 */
	void ClearAllModifiers();

	/**
	 * @brief Find a modifier by type
	 * @tparam T The modifier type to find
	 * @return The modifier if found, nullptr otherwise
	 */
	template<typename T>
	T* FindModifier()
	{
		for (UCameraModifier* Modifier : ModifierList)
		{
			if (T* TypedModifier = dynamic_cast<T*>(Modifier))
			{
				return TypedModifier;
			}
		}
		return nullptr;
	}

	// ========== Camera Shake ==========

	/**
	 * @brief Start a camera shake effect
	 * @param Intensity Shake intensity
	 * @param Duration Shake duration in seconds
	 */
	void StartCameraShake(float Intensity = 1.0f, float Duration = 0.5f);

	// ========== Default Camera Shake Settings ==========

	/**
	 * @brief Default Bezier curve control points for shake decay
	 * Editor에서 설정한 값이 PIE/Game 모드로 전달됨
	 */
	float DefaultShakeBezierCP[4] = { 0.250f, 0.460f, 0.450f, 0.940f };  // easeOutQuad

	/**
	 * @brief Use Bezier curve for shake decay by default
	 */
	bool bDefaultUseBezierDecay = true;

	/**
	 * @brief Static storage for Editor→PIE value transfer
	 * 에디터에서 수정한 값을 PIE로 전달하기 위한 static 저장소
	 */
	static float StaticShakeBezierCP[4];
	static bool StaticUseBezierDecay;
	static bool bStaticValuesInitialized;

	// ========== Lifecycle ==========

	APlayerCameraManager();
	virtual ~APlayerCameraManager();

	/**
	 * @brief Initialize default modifiers
	 */
	virtual void BeginPlay() override;

private:
	/**
	 * @brief Update camera fade
	 * @param DeltaTime Time since last frame
	 */
	void UpdateFade(float DeltaTime);

	/**
	 * @brief Update letter box
	 * @param DeltaTime Time since last frame
	 */
	void UpdateLetterBox(float DeltaTime);

	/**
	 * @brief Update spring arm camera position
	 * @param DeltaTime Time since last frame
	 */
	void UpdateSpringArm(float DeltaTime);

	/**
	 * @brief Update camera view transition
	 * @param DeltaTime Time since last frame
	 */
	void UpdateViewTransition(float DeltaTime);

	/**
	 * @brief Apply all camera modifiers
	 * @param DeltaTime Time since last frame
	 * @param CameraLocation Camera location to modify
	 * @param CameraRotation Camera rotation to modify
	 */
	void ApplyCameraModifiers(float DeltaTime, FVector& CameraLocation, FRotator& CameraRotation);

	/**
	 * @brief Calculate camera location for current view type
	 * @param ViewType The view type
	 * @return Calculated camera location
	 */
	FVector CalculateCameraLocationForView(ECameraViewType ViewType);

	/**
	 * @brief Calculate camera rotation for current view type
	 * @param ViewType The view type
	 * @return Calculated camera rotation
	 */
	FRotator CalculateCameraRotationForView(ECameraViewType ViewType);

private:
	// ========== Core Camera ==========
	UCamera* Camera = nullptr;
	UCameraComponent* CameraComponent = nullptr;  // PIE mode camera

	// ========== View Target ==========
	FViewTarget ViewTarget;
	FName CameraStyle;

	// ========== Fade System ==========
	FVector4 FadeColorLinear = FVector4(0, 0, 0, 1);
	float FadeAmount = 0.0f;
	FVector2 FadeAlpha = FVector2(0, 0);
	float FadeTime = 0.0f;
	float FadeTimeRemaining = 0.0f;
	bool bFadeOut = true;
	bool bHoldFade = false;

	// ========== Letter Box ==========
	bool bIsLetterBoxActive = false;
	float LetterBoxHeight = 0.1f;
	float LetterBoxTargetAlpha = 0.0f;
	float LetterBoxCurrentAlpha = 0.0f;
	float LetterBoxBlendSpeed = 2.0f;

	// ========== Spring Arm ==========
	bool bSpringArmEnabled = true;
	FVector SpringArmOffset = FVector(-300.0f, 0.0f, 100.0f);
	float SpringArmLength = 300.0f;
	float SpringArmInterpSpeed = 10.0f;
	bool bEnableCollisionTest = true;

	// ========== Camera View Type ==========
	ECameraViewType CurrentViewType = ECameraViewType::ThirdPerson;
	ECameraViewType PreviousViewType = ECameraViewType::ThirdPerson;
	float ViewTransitionDuration = 1.0f;
	float ViewTransitionTimeRemaining = 0.0f;
	bool bIsTransitioning = false;

	// ========== Camera Modifiers ==========
	TArray<UCameraModifier*> ModifierList;
};
