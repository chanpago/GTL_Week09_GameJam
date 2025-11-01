setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector
local ReturnTable = {}

-- Script-owned velocity that drives Tick movement; adjust per actor
ReturnTable.Velocity = FVector(10.0, 0.0, 0.0)

function ReturnTable:BeginPlay()
    -- Use self.this instead of local obj for better hot-reload support
    if not self.this then
        Print("[BeginPlay] Lua script missing actor reference.")
        return
    end

    local actor = self.this

    -- Set initial position
    actor.ActorLocation = FVector(10.0, 10.0, 10.0)

    local uuid = actor.GetUUID and actor:GetUUID() or "Unknown"
    local display_name = self.Name or (actor.GetName and actor:GetName() or "Actor")
    Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))

    -- Print location (if PrintLocation is implemented)
    if actor.PrintLocation then
        actor:PrintLocation()
    end
end

function ReturnTable:Tick(dt)
    -- Always use self.this instead of cached obj
    -- This is safer for hot-reloads
    if not self.this then
        Print("[ERROR] Tick: actor reference is nil!")
        return
    end

    local actor = self.this

    -- Validate actor has GetUUID (ensures it's a valid actor)
    if not actor.GetUUID then
        Print("[ERROR] Tick: actor is invalid!")
        return
    end

    -- Get velocity
    local velocity = self.Velocity or FVector(10.0, 0.0, 0.0)

    -- Get current location
    local loc = actor.ActorLocation

    -- Update location
    actor.ActorLocation = loc + velocity * dt

    -- Print location (if implemented)
    if actor.PrintLocation then
        actor:PrintLocation()
    else
        Print(string.format("[Tick] Location: (%.2f, %.2f, %.2f)", loc.X, loc.Y, loc.Z))
    end
end

function ReturnTable:EndPlay()
    if not self.this then
        return
    end

    local actor = self.this
    local uuid = actor.GetUUID and actor:GetUUID() or "Unknown"
    Print(string.format("[EndPlay] %s", uuid))

    if actor.PrintLocation then
        actor:PrintLocation()
    end
end

return ReturnTable
