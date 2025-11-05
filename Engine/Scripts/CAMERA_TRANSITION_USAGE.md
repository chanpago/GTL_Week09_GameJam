# 카메라 트랜지션 사용법

PlayerCharacter.lua에 카메라 쉐이크와 동일한 패턴으로 카메라 트랜지션 기능이 추가되었습니다.

## 사용 가능한 함수

### 1. TransitionCameraToLocation
특정 위치와 회전으로 카메라를 부드럽게 전환합니다.

```lua
function ReturnTable:TransitionCameraToLocation(targetPos, targetRot, duration, easeType)
```

**파라미터:**
- `targetPos`: FVector - 목표 카메라 위치
- `targetRot`: FRotator - 목표 카메라 회전
- `duration`: float (선택) - 전환 시간 (초), 기본값 2.0
- `easeType`: int (선택) - Easing 타입, 기본값 3 (EaseInOut)

**EaseType 값:**
- `0` = Linear (선형)
- `1` = EaseIn (천천히 시작)
- `2` = EaseOut (천천히 끝)
- `3` = EaseInOut (천천히 시작하고 끝)
- `4` = Bezier (베지어 곡선)

### 2. TransitionCameraToActor
특정 액터를 따라가도록 카메라를 전환합니다.

```lua
function ReturnTable:TransitionCameraToActor(targetActor, duration, offset, easeType)
```

**파라미터:**
- `targetActor`: AActor - 따라갈 타겟 액터
- `duration`: float (선택) - 전환 시간 (초), 기본값 2.0
- `offset`: FVector (선택) - 액터로부터의 오프셋, 기본값 FVector(-300, 0, 100)
- `easeType`: int (선택) - Easing 타입, 기본값 3 (EaseInOut)

## 사용 예제

### 예제 1: 플레이어가 특수 능력 사용 시 카메라 연출

```lua
function ReturnTable:OnSpecialAbility()
    -- 플레이어 뒤쪽 상단에서 내려다보는 각도로 전환
    local playerPos = PlayerActor.ActorLocation
    local playerRot = PlayerActor.ActorRotation

    -- 뒤로 400, 위로 200 떨어진 위치
    local backOffset = FVector(-400.0, 0.0, 200.0)
    local worldOffset = playerRot:RotateVector(backOffset)
    local cameraPos = playerPos + worldOffset

    -- 플레이어를 바라보는 회전 계산
    local lookDir = (playerPos - cameraPos):GetNormalized()
    local yaw = math.atan2(lookDir.Y, lookDir.X) * (180.0 / math.pi)
    local pitch = math.asin(-lookDir.Z) * (180.0 / math.pi)
    local cameraRot = FRotator(pitch, yaw, 0.0)

    -- 1.5초 동안 부드럽게 전환 (EaseInOut)
    self:TransitionCameraToLocation(cameraPos, cameraRot, 1.5, 3)

    Print("[Player] Special ability camera activated!")
end
```

### 예제 2: 보스 등장 시 보스에게 카메라 전환

```lua
function ReturnTable:OnBossAppear(bossActor)
    -- 보스를 2초 동안 보여주기
    local bossOffset = FVector(-500.0, 0.0, 150.0)  -- 보스 뒤쪽 상단
    self:TransitionCameraToActor(bossActor, 2.0, bossOffset, 3)

    Print("[Player] Camera focusing on boss!")
end
```

### 예제 3: 아이템 획득 시 짧은 연출

```lua
function ReturnTable:OnItemPickup()
    local playerPos = PlayerActor.ActorLocation
    local playerRot = PlayerActor.ActorRotation

    -- 플레이어 바로 앞에서 클로즈업
    local frontOffset = FVector(150.0, 0.0, 50.0)
    local worldOffset = playerRot:RotateVector(frontOffset)
    local cameraPos = playerPos + worldOffset

    -- 플레이어를 바라봄
    local lookDir = (playerPos - cameraPos):GetNormalized()
    local yaw = math.atan2(lookDir.Y, lookDir.X) * (180.0 / math.pi)
    local pitch = math.asin(-lookDir.Z) * (180.0 / math.pi)
    local cameraRot = FRotator(pitch, yaw, 0.0)

    -- 빠르게 전환 (1초)
    self:TransitionCameraToLocation(cameraPos, cameraRot, 1.0, 2)  -- EaseOut
end
```

### 예제 4: 키 입력으로 테스트

```lua
function ReturnTable:Tick(DeltaTime)
    -- 기존 코드...

    local InputMgr = GetInputManager()
    if InputMgr then
        -- T 키: 플레이어 앞으로 카메라 전환 테스트
        if InputMgr:IsKeyDown("T") then
            local playerPos = PlayerActor.ActorLocation
            local playerRot = PlayerActor.ActorRotation

            local frontOffset = FVector(300.0, 0.0, 100.0)
            local worldOffset = playerRot:RotateVector(frontOffset)
            local targetPos = playerPos + worldOffset

            local lookDir = (playerPos - targetPos):GetNormalized()
            local yaw = math.atan2(lookDir.Y, lookDir.X) * (180.0 / math.pi)
            local pitch = math.asin(-lookDir.Z) * (180.0 / math.pi)
            local targetRot = FRotator(pitch, yaw, 0.0)

            self:TransitionCameraToLocation(targetPos, targetRot, 2.0, 3)
        end
    end
end
```

## 기존 카메라 쉐이크와 함께 사용

카메라 트랜지션과 카메라 쉐이크는 동시에 사용할 수 있습니다.

```lua
function ReturnTable:OnHit(OverlappedComp, OtherActor, OtherComp, NormalImpulse, OutHit)
    -- 기존 쉐이크 (OnHit 함수에 이미 구현됨)
    local gameMode = GetGameMode()
    if gameMode then
        local playerController = gameMode:GetPlayerController()
        if playerController then
            local camMgr = playerController:GetPlayerCameraManager()
            if camMgr then
                camMgr:StartCameraShake(5.0, 1.0)  -- 쉐이크 효과
            end
        end
    end

    -- 필요시 트랜지션도 추가 가능
    -- self:TransitionCameraToLocation(...)
end
```

## 주의사항

1. **CameraManager 접근**: 모든 함수는 내부적으로 `GetGameMode() → GetPlayerController() → GetPlayerCameraManager()` 순서로 접근합니다.

2. **좌표계**:
   - FVector: (X, Y, Z) - X는 전방, Y는 우측, Z는 상단
   - FRotator: (Pitch, Yaw, Roll) - 각도는 degree 단위

3. **성능**: 카메라 트랜지션은 Modifier 시스템을 사용하므로 여러 개를 동시에 실행해도 안전합니다.

4. **전환 중단**: 새로운 트랜지션을 시작하면 이전 트랜지션은 자동으로 중단됩니다.

## 디버깅

트랜지션이 작동하지 않으면:
1. PIE 모드에서 실행 중인지 확인
2. PlayerCameraManager가 초기화되었는지 확인
3. Console 창에서 에러 메시지 확인
