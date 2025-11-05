#pragma once
#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"

/**
 * @brief Helper class for testing camera transitions in C++
 *
 * Usage:
 * 1. Create an instance in your level or game mode
 * 2. Call TestBasicTransition(), TestFollowActor(), etc.
 * 3. Observe smooth camera movements
 */
class CameraTransitionTestHelper
{
public:
	/**
	 * @brief Test basic camera transition to a fixed location
	 */
	static void TestBasicTransition(APlayerCameraManager* CameraManager)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		// Test 1: Front view with EaseInOut
		FVector targetLocation(500.0f, 0.0f, 100.0f);
		FRotator targetRotation(0.0f, 180.0f, 0.0f);

		UE_LOG("[CameraTest] Starting basic transition");
		UE_LOG("[CameraTest]   Target: (%.1f, %.1f, %.1f)",
			targetLocation.X, targetLocation.Y, targetLocation.Z);
		UE_LOG("[CameraTest]   Duration: 2.0s");
		UE_LOG("[CameraTest]   Easing: EaseInOut");

		CameraManager->StartTransitionToLocation(
			targetLocation,
			targetRotation,
			2.0f,
			ECameraEaseType::EaseInOut
		);
	}

	/**
	 * @brief Test multiple sequential transitions with different easing types
	 */
	static void TestSequentialTransitions(APlayerCameraManager* CameraManager)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		// This will start the first transition
		// You'll need to chain subsequent transitions in a tick function
		// or use a coroutine system

		struct ViewPoint
		{
			FVector Location;
			FRotator Rotation;
			ECameraEaseType EaseType;
			float Duration;
			const char* Name;
		};

		static ViewPoint views[] = {
			{ FVector(500, 0, 100),    FRotator(0, 180, 0),  ECameraEaseType::Linear,       1.5f, "Front (Linear)" },
			{ FVector(0, 0, 800),      FRotator(-80, 0, 0),  ECameraEaseType::EaseIn,       2.0f, "Top (EaseIn)" },
			{ FVector(0, 600, 100),    FRotator(0, -90, 0),  ECameraEaseType::EaseOut,      2.0f, "Side (EaseOut)" },
			{ FVector(-400, 0, 600),   FRotator(-45, 0, 0),  ECameraEaseType::SmoothStep,   1.8f, "Angle (SmoothStep)" },
			{ FVector(300, 300, 50),   FRotator(-10, -135, 0), ECameraEaseType::SmootherStep, 2.2f, "Low (SmootherStep)" }
		};

		// Start first transition
		UE_LOG("[CameraTest] Starting sequential transitions test");
		CameraManager->StartTransitionToLocation(
			views[0].Location,
			views[0].Rotation,
			views[0].Duration,
			views[0].EaseType
		);
		UE_LOG("[CameraTest] -> %s", views[0].Name);
	}

	/**
	 * @brief Test following an actor with smooth transition
	 */
	static void TestFollowActor(APlayerCameraManager* CameraManager, AActor* TargetActor)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		if (!TargetActor)
		{
			UE_LOG("[CameraTest] Error: TargetActor is null");
			return;
		}

		// Third-person follow with offset
		FVector offset(-300.0f, 0.0f, 100.0f);

		UE_LOG("[CameraTest] Starting follow actor transition");
		UE_LOG("[CameraTest]   Target: %s", TargetActor->GetName().ToString().c_str());
		UE_LOG("[CameraTest]   Offset: (%.1f, %.1f, %.1f)", offset.X, offset.Y, offset.Z);
		UE_LOG("[CameraTest]   Duration: 1.5s");

		CameraManager->StartTransitionToActor(
			TargetActor,
			1.5f,
			ECameraEaseType::EaseInOut,
			offset
		);
	}

	/**
	 * @brief Test instant transition (duration = 0)
	 */
	static void TestInstantTransition(APlayerCameraManager* CameraManager)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		FVector targetLocation(0.0f, 0.0f, 500.0f);
		FRotator targetRotation(-45.0f, 0.0f, 0.0f);

		UE_LOG("[CameraTest] Testing instant transition (no interpolation)");

		CameraManager->StartTransitionToLocation(
			targetLocation,
			targetRotation,
			0.0f,  // Instant!
			ECameraEaseType::Linear
		);
	}

	/**
	 * @brief Test all easing types in sequence
	 */
	static void TestAllEasingTypes(APlayerCameraManager* CameraManager, int currentIndex = 0)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		struct EasingTest
		{
			ECameraEaseType Type;
			const char* Name;
			FVector Location;
			float Duration;
		};

		static EasingTest tests[] = {
			{ ECameraEaseType::Linear,       "Linear",       FVector(400, 0, 100),    2.0f },
			{ ECameraEaseType::EaseIn,       "EaseIn",       FVector(-400, 0, 100),   2.0f },
			{ ECameraEaseType::EaseOut,      "EaseOut",      FVector(0, 400, 100),    2.0f },
			{ ECameraEaseType::EaseInOut,    "EaseInOut",    FVector(0, -400, 100),   2.0f },
			{ ECameraEaseType::SmoothStep,   "SmoothStep",   FVector(300, 300, 200),  2.0f },
			{ ECameraEaseType::SmootherStep, "SmootherStep", FVector(-300, -300, 200), 2.0f }
		};

		if (currentIndex >= sizeof(tests) / sizeof(EasingTest))
		{
			UE_LOG("[CameraTest] All easing types tested!");
			return;
		}

		const EasingTest& test = tests[currentIndex];
		UE_LOG("[CameraTest] Testing easing type: %s", test.Name);

		CameraManager->StartTransitionToLocation(
			test.Location,
			FRotator(-20.0f, 0.0f, 0.0f),
			test.Duration,
			test.Type
		);
	}

	/**
	 * @brief Test stopping a transition mid-way
	 */
	static void TestStopTransition(APlayerCameraManager* CameraManager)
	{
		if (!CameraManager)
		{
			UE_LOG("[CameraTest] Error: CameraManager is null");
			return;
		}

		if (CameraManager->IsCameraTransitioning())
		{
			float progress = CameraManager->GetTransitionProgress();
			UE_LOG("[CameraTest] Stopping transition at %.1f%% progress", progress * 100.0f);
			CameraManager->StopCameraTransition();
		}
		else
		{
			UE_LOG("[CameraTest] No active transition to stop");
		}
	}

};
