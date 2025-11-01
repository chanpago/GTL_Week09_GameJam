#pragma once
#include "Global/WeakObjectPtr.h"

class UPlayerInput;
class AActor;

class APlayerController : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(APlayerController, AActor)
public:
    APlayerController();
    ~APlayerController();

    void Initialize();
    
    void Tick(float DeltaTime) override;

    void SetInput(UPlayerInput* PlayerInput);

    void Possess(AActor* TargetActor);
    void UnPossess();

private:
    // Value값의 부호를 바꾸면 반대방향 이동
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

private:
    TWeakObjectPtr<AActor> ControlledActor;
    UPlayerInput* PlayerInput = nullptr;

    float MoveSpeed = 1.0f;
};
