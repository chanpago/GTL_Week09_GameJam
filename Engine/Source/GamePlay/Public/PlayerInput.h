#pragma once
#include "Global/DelegateMacros.h"


DECLARE_DYNAMIC_DELEGATE(FOnMoveForward, float);
DECLARE_DYNAMIC_DELEGATE(FOnMoveRight, float);
DECLARE_DYNAMIC_DELEGATE(FOnTurn, float);
DECLARE_DYNAMIC_DELEGATE(FOnLookUp, float);

class UPlayerInput : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(UPlayerInput, UObject)
public:
    UPlayerInput();
    ~UPlayerInput();
    
    void Update(float DeltaTime);

public:
    FOnMoveForward OnMoveForward;
    FOnMoveRight OnMoveRight;
    FOnTurn OnTurn;
    FOnLookUp OnLookUp;

private:
    void HandleKeyboardInput(float DeltaTime);
    void HandleMouseInput(float DeltaTime);
    
};
