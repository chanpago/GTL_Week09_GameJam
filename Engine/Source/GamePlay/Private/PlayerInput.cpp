#include "pch.h"
#include "GamePlay/Public/PlayerInput.h"
#include "Manager/Input/Public/InputManager.h"

IMPLEMENT_CLASS(UPlayerInput, UObject)

UPlayerInput::UPlayerInput()
{
}

UPlayerInput::~UPlayerInput()
{
}

void UPlayerInput::Update(float DeltaTime)
{
    HandleMouseInput(DeltaTime);
    HandleKeyboardInput(DeltaTime);
}

void UPlayerInput::HandleKeyboardInput(float DeltaTime)
{
    auto& InputManager = UInputManager::GetInstance();
    
    if (InputManager.IsKeyDown(EKeyInput::W))
    {
        OnMoveForward.BroadCast(1.0f * DeltaTime);
    }

    if (InputManager.IsKeyDown(EKeyInput::S))
    {
        OnMoveForward.BroadCast(-1.0f * DeltaTime);
    }

    if (InputManager.IsKeyDown(EKeyInput::D))
    {
        OnMoveRight.BroadCast(1.0f * DeltaTime);
    }

    if (InputManager.IsKeyDown(EKeyInput::A))
    {
        OnMoveRight.BroadCast(-1.0f * DeltaTime);
    }
}

void UPlayerInput::HandleMouseInput(float DeltaTime)
{
    auto& InputManager = UInputManager::GetInstance();

    float MouseDeltaX = InputManager.GetMouseDelta().X;
    float MouseDeltaY = InputManager.GetMouseDelta().Y;

    if (fabsf(MouseDeltaX) > 0.01f)
    {
        OnTurn.BroadCast(MouseDeltaX * DeltaTime);
    }

    if (fabsf(MouseDeltaY) > 0.01f)
    {
        OnLookUp.BroadCast(MouseDeltaY * DeltaTime);
    }
}
