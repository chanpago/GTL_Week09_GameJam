#pragma once
#include "Pawn/Public/Pawn.h"

class USphereComponent;
class UCameraComponent;
class UPointLightComponent;
struct FHitResult;


/**
 * @brief APlayerCharacter is the default pawn class for player-controlled characters.
 * Inherits from APawn and adds player-specific functionality.
 */
UCLASS()
class APlayerCharacter : public APawn
{
    GENERATED_BODY()
    DECLARE_CLASS(APlayerCharacter, APawn)

public:
    APlayerCharacter();
    virtual ~APlayerCharacter();

    // UObject interface


    // Lifecycle
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Input handling (override from APawn)
    virtual void MoveForward(float Value) override;
    virtual void MoveRight(float Value) override;
    virtual void Turn(float Value) override;
    virtual void LookUp(float Value) override;

    void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void OnHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& OutHit);

    // Get camera component
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

protected:
    // Default movement speed
    float MovementSpeed = 10.0f;

    // Default rotation speed (degrees per second)
    float RotationSpeed = 50.0f;

    // Mouse sensitivity for pitch/yaw rotation
    float MouseSensitivity = 10.0f;

    // Track forward movement for engine lights
    bool bIsMovingForward = false;

    // Acceleration-based movement system
    FVector CurrentVelocity = FVector::ZeroVector();     // 현재 속도
    float InputValue = 0.0f;                             // 입력값 (W키 누른 정도)
    float Acceleration = 300.0f;                         // 가속도
    float MaxSpeed = 500.0f;                             // 최대 속도
    float Deceleration = 150.0f;                         // 감속도

    // Jet sound cooldown system
    float JetSoundCooldown = 0.0f;                       // 현재 쿨타임 타이머
    float JetSoundCooldownTime = 5.0f;                   // 쿨타임 (5초)
    bool bWasMovingForward = false;                      // 이전 프레임에 W키를 눌렀는지

    USphereComponent* CollisionComponent = nullptr;
    UStaticMeshComponent* StaticMeshComponent = nullptr;
    UCameraComponent* CameraComponent = nullptr;
    UPointLightComponent* PointLight1 = nullptr;
    UPointLightComponent* PointLight2 = nullptr;
};
