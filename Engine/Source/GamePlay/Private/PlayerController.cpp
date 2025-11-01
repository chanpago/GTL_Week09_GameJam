#include "pch.h"
#include "GamePlay/Public/PlayerController.h"
#include "GamePlay/public/PlayerInput.h"

IMPLEMENT_CLASS(APlayerController, AActor)

APlayerController::APlayerController()
{
}

APlayerController::~APlayerController()
{
}

void APlayerController::Initialize()
{
    PlayerInput = NewObject<UPlayerInput>();

    SetInput(PlayerInput);
}

void APlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (PlayerInput)
    {
        PlayerInput->Update(DeltaTime);
    }
}

void APlayerController::SetInput(UPlayerInput* PlayerInput)
{
    PlayerInput->OnMoveForward.AddDynamic(this, &APlayerController::MoveForward);
    PlayerInput->OnMoveRight.AddDynamic(this, &APlayerController::MoveRight);
    PlayerInput->OnTurn.AddDynamic(this, &APlayerController::Turn);
    PlayerInput->OnLookUp.AddDynamic(this, &APlayerController::LookUp);
}

void APlayerController::Possess(AActor* TargetActor)
{
    if (!TargetActor)
    {
        UE_LOG("Target Actor is null");
        return;
    }
    ControlledActor.Set(TargetActor);
}

void APlayerController::UnPossess()
{
    ControlledActor.Reset();
}

void APlayerController::MoveForward(float Value)
{
    if (!ControlledActor.IsValid() || Value == 0.0f || MoveSpeed == 0.0f)
    {
        return;
    }
    
    FVector Forward(1.0f, 0.0f, 0.0f);
    FVector NewLocation = ControlledActor->GetActorLocation() + (Forward * Value * MoveSpeed);
    ControlledActor->SetActorLocation(NewLocation);
}

void APlayerController::MoveRight(float Value)
{
    if (!ControlledActor.IsValid() || Value == 0.0f || MoveSpeed == 0.0f)
    {
        return;
    }
    
    FVector Forward(0.0f, 1.0f, 0.0f);
    FVector NewLocation = ControlledActor->GetActorLocation() + (Forward * Value * MoveSpeed);
    ControlledActor->SetActorLocation(NewLocation);
}

void APlayerController::Turn(float Value)
{
    if (!ControlledActor.IsValid() || Value == 0.0f)
    {
        return;
    }

    FVector Rotation = ControlledActor->GetActorRotation().ToEuler();
    Rotation.Z += Value;

    ControlledActor->SetActorRotation(FQuaternion::FromEuler(Rotation));
}

void APlayerController::LookUp(float Value)
{
    if (!ControlledActor.IsValid() || Value == 0.0f)
    {
        return;
    }

    FVector Rotation = ControlledActor->GetActorRotation().ToEuler();
    Rotation.Y += Value;

    ControlledActor->SetActorRotation(FQuaternion::FromEuler(Rotation));
}
