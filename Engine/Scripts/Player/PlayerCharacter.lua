-- PlayerCharacter.lua: 플레이어 캐릭터 + 무기 시스템 통합
-- PlayerCharacter에 붙여서 사용
--
-- [카메라 트랜지션 테스트 키]
-- Q키: 플레이어 앞쪽 상단에서 내려다보는 테스트 카메라로 전환
-- R키: 원래 카메라 위치로 복귀
--
-- [카메라 트랜지션 사용 예제]
-- 1. 특정 위치로 카메라 전환:
--    self:TransitionCameraToLocation(targetPos, targetRot, 2.0, 3)
--
-- 2. 특정 액터 따라가기:
--    self:TransitionCameraToActor(targetActor, 2.0, FVector(-300, 0, 100), 3)
--
-- 3. 카메라 쉐이크:
--    (기존과 동일) OnHit에서 사용 중

setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector

-- MissilePool 로드
local MissilePoolModule = require("Scripts/MissilePool")

return function()
    local PlayerActor = nil
    local MissilePool = nil
    local ReturnTable = {}

    -- 데미지 쿨타임 시스템
    local lastDamageTime = 0    -- 마지막 데미지 받은 시간
    local damageCooldown = 1.0  -- 1초 쿨타임

    -- 카메라 트랜지션 테스트용 변수
    local isQKeyPressed = false

    -- ==================================================
    -- 무기 설정
    -- ==================================================
    ReturnTable.FireCooldown = 0.5          -- 발사 쿨다운 (초)
    ReturnTable.CurrentCooldown = 0.0       -- 현재 쿨다운
    ReturnTable.MissileMeshPath = "Data/Missile.obj"  -- 미사일 메쉬
    ReturnTable.MissileScale = FVector(50.0, 200.0, 200.0)  -- 미사일 크기
    ReturnTable.WeaponEnabled = false       -- 무기 활성화 상태 (Start Game 전까지 false)

    -- 미사일 레벨 시스템 (나중에 아이템으로 업그레이드)
    ReturnTable.MissileLevel = 1            -- 현재 미사일 레벨 (1~3)
    ReturnTable.MissilesPerShot = 3         -- 레벨 1부터 2발 발사

    -- 레벨별 미사일 발사 위치 (루트 컴포넌트로부터의 상대 오프셋)
    ReturnTable.MissileSpawnOffsets = {
        -- 레벨 1: 좌우 2발
        [1] = {
            FVector(-40.0, -20.0, 12.0),  -- 왼쪽
            FVector(-40.0, 20.0, 12.0)    -- 오른쪽
        },
        -- 레벨 2: 좌우 + 중앙 3발 (나중에 확장 가능)
        [2] = {
            FVector(-2.5, -3.0, 1.3),
            FVector(-2.5, 0.0, 1.3),
            FVector(-2.5, 3.0, 1.3)
        },
        -- 레벨 3: 4발 (나중에 확장 가능)
        [3] = {
            FVector(-2.5, -3.0, 1.3),
            FVector(-2.5, -1.0, 1.3),
            FVector(-2.5, 1.0, 1.3),
            FVector(-2.5, 3.0, 1.3)
        }
    }

    -- 데미지 쿨타임 체크 (플레이어가 데미지 받을 수 있는지)
    function ReturnTable:CanDealDamage()
        local timeManager = GetTimeManager()
        if not timeManager then
            return false
        end

        local currentTime = timeManager:GetGameTime()
        local timeSinceLastDamage = currentTime - lastDamageTime

        return timeSinceLastDamage >= damageCooldown
    end

    -- 데미지 타이머 갱신
    function ReturnTable:UpdateDamageTime()
        local timeManager = GetTimeManager()
        if timeManager then
            lastDamageTime = timeManager:GetGameTime()
        end
    end

    -- ==================================================
    -- BeginPlay: 초기화
    -- ==================================================
    function ReturnTable:BeginPlay()
        PlayerActor = self.this
        Print("[PlayerCharacter] READY: PlayerCharacter is Ready!")
        
        if not PlayerActor then
            Print("[PlayerCharacter] ERROR: PlayerActor is nil!")
            return
        end

        -- PIE 재시작을 위해 풀 초기화
        MissilePoolModule.Reset()

        -- MissilePool 싱글톤 가져오기 (새로 생성)
        MissilePool = MissilePoolModule.GetInstance()
        if not MissilePool then
            Print("[PlayerCharacter] ERROR: Failed to get MissilePool instance!")
            return
        end

        local ActorName = PlayerActor:GetName()
        Print("[PlayerCharacter] Initialized for: " .. ActorName)
        Print("[PlayerCharacter] Weapon is DISABLED. Waiting for game to start...")
        Print("[PlayerCharacter] Using Object Pooling for missiles")
    end

    -- ==================================================
    -- EnableWeapon: 무기 활성화 (Start Game 버튼으로 호출됨)
    -- ==================================================
    function ReturnTable:EnableWeapon()
        self.WeaponEnabled = true
    end

    -- ==================================================
    -- DisableWeapon: 무기 비활성화
    -- ==================================================
    function ReturnTable:DisableWeapon()
        self.WeaponEnabled = false
    end

    -- ==================================================
    -- Tick: 입력 체크 및 쿨다운 관리
    -- ==================================================
    function ReturnTable:Tick(DeltaTime)
        if not PlayerActor then
            return
        end

        local InputMgr = GetInputManager()
        if not InputMgr then
            return
        end

        -- ==========================================
        -- 카메라 트랜지션 테스트 (무기 활성화와 무관)
        -- ==========================================
        -- Q키: 특정 위치로 카메라 이동 (디버그용)
        if InputMgr:IsKeyDown("Q") and not isQKeyPressed then
            isQKeyPressed = true
            self:TestCameraTransitionToFixedPosition()
            Print("[CameraTest] Q - Moving to fixed test position!")
        elseif not InputMgr:IsKeyDown("Q") and isQKeyPressed then
            isQKeyPressed = false
        end

        -- R키: 원래 카메라로 복귀
        if InputMgr:IsKeyDown("R") then
            self:TestCameraTransitionReturn()
            Print("[CameraTest] R - Returning to follow camera!")
        end

        -- ==========================================
        -- 무기 시스템 (WeaponEnabled일 때만)
        -- ==========================================
        -- 무기가 비활성화되어 있으면 발사 불가
        if not self.WeaponEnabled then
            return
        end

        -- 쿨다운 감소
        if self.CurrentCooldown > 0.0 then
            self.CurrentCooldown = self.CurrentCooldown - DeltaTime
        end

        -- 마우스 왼쪽 버튼 입력 체크 (꾹 누르면 계속 발사)
        if InputMgr:IsKeyDown("MouseLeft") and self.CurrentCooldown <= 0.0 then
            self:FireMissile()
            self.CurrentCooldown = self.FireCooldown
        end
    end

    -- ==================================================
    -- FireMissile: 미사일 발사 (오브젝트 풀 사용)
    -- ==================================================
    function ReturnTable:FireMissile()
        if not MissilePool then
            return
        end

        -- 플레이어 위치와 회전
        local PlayerLocation = PlayerActor.ActorLocation
        local PlayerRotation = PlayerActor.ActorRotation

        -- 현재 레벨에 맞는 오프셋 배열 가져오기
        local spawnOffsets = self.MissileSpawnOffsets[self.MissileLevel]
        if not spawnOffsets then
            return
        end

        -- 각 오프셋 위치에서 미사일 발사
        for i, localOffset in ipairs(spawnOffsets) do
            -- 로컬 오프셋을 월드 좌표로 변환
            local worldOffset = PlayerRotation:RotateVector(localOffset)
            local spawnLocation = PlayerLocation + worldOffset

            -- 풀에서 미사일 가져오기 (없으면 자동으로 생성)
            local MissileActor = MissilePool:GetMissile(spawnLocation, PlayerRotation, self.MissileScale)
        end
    end

    -- ==================================================
    -- UpgradeMissile: 미사일 레벨 업그레이드 (아이템으로 호출)
    -- ==================================================
    function ReturnTable:UpgradeMissile()
        if self.MissileLevel < 3 then
            self.MissileLevel = self.MissileLevel + 1
        end
    end

    -- ==================================================
    -- 카메라 트랜지션 헬퍼 함수
    -- ==================================================
    -- 특정 위치로 카메라 부드럽게 전환
    function ReturnTable:TransitionCameraToLocation(targetPos, targetRot, duration, easeType)
        local gameMode = GetGameMode()
        if gameMode then
            local playerController = gameMode:GetPlayerController()
            if playerController then
                local camMgr = playerController:GetPlayerCameraManager()
                if camMgr then
                    -- easeType: 0=Linear, 1=EaseIn, 2=EaseOut, 3=EaseInOut, 4=Bezier
                    local actualEaseType = easeType or 3  -- 기본값: EaseInOut
                    local actualDuration = duration or 2.0
                    local targetFOV = -1  -- -1 = 현재 FOV 유지

                    camMgr:StartTransitionToLocation(
                        targetPos,
                        targetRot,
                        actualDuration,
                        actualEaseType,
                        targetFOV
                    )
                end
            end
        end
    end

    -- 특정 액터를 따라가도록 카메라 전환
    function ReturnTable:TransitionCameraToActor(targetActor, duration, offset, easeType)
        local gameMode = GetGameMode()
        if gameMode then
            local playerController = gameMode:GetPlayerController()
            if playerController then
                local camMgr = playerController:GetPlayerCameraManager()
                if camMgr then
                    local actualEaseType = easeType or 3  -- 기본값: EaseInOut
                    local actualDuration = duration or 2.0
                    local actualOffset = offset or FVector(-300.0, 0.0, 100.0)

                    camMgr:StartTransitionToActor(
                        targetActor,
                        actualDuration,
                        actualEaseType,
                        actualOffset
                    )
                end
            end
        end
    end

    -- ==================================================
    -- 카메라 트랜지션 테스트 함수 (Q키, R키)
    -- ==================================================
    -- Q키: 플레이어 기준 상대 위치로 카메라 이동 (디버그용)
    function ReturnTable:TestCameraTransitionToFixedPosition()
        if not PlayerActor then
            return
        end

        local gameMode = GetGameMode()
        if not gameMode then
            return
        end

        local playerController = gameMode:GetPlayerController()
        if not playerController then
            return
        end

        local camMgr = playerController:GetPlayerCameraManager()
        if not camMgr then
            return
        end

        -- 플레이어 위치와 회전
        local playerPos = PlayerActor.ActorLocation
        local playerRot = PlayerActor.ActorRotation

        -- 플레이어 기준 상대 오프셋 (-500, 0, 500)
        local localOffset = FVector(-500.0, 0.0, 500.0)
        local worldOffset = playerRot:RotateVector(localOffset)
        local targetPos = playerPos + worldOffset

        -- 플레이어를 바라보는 회전 계산
        local lookDir = (playerPos - targetPos):GetNormalized()
        local yaw = math.atan(lookDir.Y, lookDir.X) * (180.0 / 3.14159265359)
        local pitch = math.asin(-lookDir.Z) * (180.0 / 3.14159265359)
        local targetRot = FRotator(pitch, yaw, 0.0)

        -- 2초 동안 EaseInOut으로 이동
        camMgr:StartTransitionToLocation(
            targetPos,
            targetRot,
            2.0,  -- 2초 duration
            3,    -- EaseInOut
            -1    -- FOV 유지
        )
    end

    -- R키 또는 Q키 뗐을 때: 원래 플레이어를 따라가는 카메라로 복귀
    function ReturnTable:TestCameraTransitionReturn()
        if not PlayerActor then
            return
        end

        local gameMode = GetGameMode()
        if not gameMode then
            return
        end

        local playerController = gameMode:GetPlayerController()
        if not playerController then
            return
        end

        local camMgr = playerController:GetPlayerCameraManager()
        if not camMgr then
            return
        end

        -- 트랜지션 중지 - 원래 ViewTarget(PlayerActor)를 따라가도록 복귀
        camMgr:StopCameraTransition()
    end

    -- ==================================================
    -- Collision Events
    -- ==================================================
    function ReturnTable:OnBeginOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult)
        Print("[Lua/Player] Begin overlap")
    end

    function ReturnTable:OnEndOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex)
        Print("[Lua/Player] End overlap")
    end

    function ReturnTable:OnHit(OverlappedComp, OtherActor, OtherComp, NormalImpulse, OutHit)
        Print("[Lua/Player] Hit")

        -- 쿨타임 체크 (1초에 한 번만 데미지 받음)
        if not self:CanDealDamage() then
            return
        end

        local uiManager = GetGameUIManager()
        if uiManager then
            local hudWidget = uiManager:GetHUDWidget()
            if hudWidget then
                -- HUD의 TakeDamage 함수 호출 (10 데미지)
                hudWidget:TakeDamage(15)
                --Print("[Lua/Enemy] Dealt 10 damage to player. Current health: " .. hudWidget:GetHe
                -- 카메라 쉐이크 효과 (강도: 2.0, 지속시간: 0.5초)
                local gameMode = GetGameMode()
                if gameMode then
                    local playerController = gameMode:GetPlayerController()
                    if playerController then
                        local camMgr = playerController:GetPlayerCameraManager()
                        if camMgr then
                            camMgr:StartCameraShake(5.0, 1.0)
                        end
                    end
                end
            -- 데미지 시간 갱신
                self:UpdateDamageTime()
            end
        end
    end

    -- ==================================================
    -- EndPlay
    -- ==================================================
    function ReturnTable:EndPlay()
    end

    return ReturnTable
end