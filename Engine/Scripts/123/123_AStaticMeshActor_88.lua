setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector
local ReturnTable = {}
local obj = nil

-- Script-owned velocity that drives Tick movement; adjust per actor
ReturnTable.Velocity = FVector(0.0, 0.0, 0.0)

local function print_actor_location(actor)
    if not actor then
        return
    end

    local loc = actor.ActorLocation
    if not loc then
        return
    end

    local name = actor.GetName and actor:GetName() or "Unknown"
    Print(string.format("[Actor] %s Location: (%.2f, %.2f, %.2f)", name, loc.X, loc.Y, loc.Z))
end

function ReturnTable:BeginPlay()
    obj = self.this
    if not obj then
        Print("[BeginPlay] Lua script missing actor reference.")
        return
    end

    local uuid = obj.GetUUID and obj:GetUUID() or "Unknown"
    local display_name = self.Name or (obj.GetName and obj:GetName() or "Actor")
    Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))
    obj:PrintLocation()
end

function ReturnTable:EndPlay()
    if not obj then
        return
    end

    local uuid = obj.GetUUID and obj:GetUUID() or "Unknown"
    Print(string.format("[EndPlay] %s", uuid))
    obj:PrintLocation()
end

function ReturnTable:OnOverlap(otherActor)
    if otherActor and otherActor.PrintLocation then
        otherActor:PrintLocation()
    else
        print_actor_location(otherActor)
    end
end

function ReturnTable:Tick(dt)
    if not obj then
        return
    end

    local velocity = self.Velocity or FVector(0.0, 0.0, 0.0)
    local loc = obj.ActorLocation
    obj.ActorLocation = loc + velocity * dt
    obj.ActorLocation = FVector(30.0, 20.0, 0.0)
    obj:PrintLocation()
end

return ReturnTable
