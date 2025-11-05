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
    // Shift + F1로 입력 비활성화된 경우 무시
    if (!bInputEnabled)
    {
        return;
    }

    HandleMouseInput(DeltaTime);
    HandleKeyboardInput(DeltaTime);
}

void UPlayerInput::HandleKeyboardInput(float DeltaTime)
{
    auto& InputManager = UInputManager::GetInstance();

    if (InputManager.IsKeyDown(EKeyInput::W))
    {
        // UE_LOG("Move W");
        OnMoveForward.BroadCast(1.0f);  // DeltaTime 제거 (Tick에서 처리)
    }

    if (InputManager.IsKeyDown(EKeyInput::S))
    {
        // UE_LOG("Move S");
        OnMoveForward.BroadCast(-1.0f);  // DeltaTime 제거
    }

    if (InputManager.IsKeyDown(EKeyInput::D))
    {
        // UE_LOG("Move D");
        OnMoveRight.BroadCast(1.0f * DeltaTime);
    }

    if (InputManager.IsKeyDown(EKeyInput::A))
    {
        // UE_LOG("Move A");
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
