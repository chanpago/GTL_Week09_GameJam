-- setmetatable를 통해 EngineTypes를 전역으로 설정
setmetatable(_ENV, { __index = EngineTypes })

local ReturnTable = {}
local FVector = EngineTypes.FVector

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()
    print("[LUA] BeginPlay Cube")
    print("[LUA] self.Name =", self.Name)
    
    -- this로 Actor에 접근
    local actor = self.this
    if actor then
        print("[LUA] Actor location:", actor.ActorLocation.X, actor.ActorLocation.Y, actor.ActorLocation.Z)
        
        -- 핫 리로드 테스트: 이 값을 수정하고 저장하면 리로드 시 즉시 반영됩니다!
        actor.ActorLocation = FVector.new(30.0, 30.0, 50.0)
        print("[LUA] Set new location: 100, 50, 80")
    end
end

-- Tick: 매 프레임마다 호출
function ReturnTable:Tick(deltaTime)
    -- 비어있음 - 위치는 BeginPlay에서만 설정
end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay()
    print("[LUA] EndPlay Cube")
end

return ReturnTable
