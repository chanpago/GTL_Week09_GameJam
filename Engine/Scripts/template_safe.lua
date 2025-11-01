setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector
local ReturnTable = {}

-- Script-owned velocity that drives Tick movement; adjust per actor
ReturnTable.Velocity = FVector(0.0, 0.0, 0.0)

-- ✅ IMPORTANT: Do NOT use 'local obj = nil' at module level!
-- Multiple actors sharing the same script would share the same 'obj' variable.
-- Always use 'self.this' directly in functions.

function ReturnTable:BeginPlay()
    -- Use self.this directly, not a cached local variable
    if not self.this then
        Print("[BeginPlay] Lua script missing actor reference.")
        return
    end

    local actor = self.this  -- Function-local variable is safe!
    local uuid = actor.GetUUID and actor:GetUUID() or "Unknown"
    local display_name = self.Name or (actor.GetName and actor:GetName() or "Actor")
    Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))

    if actor.PrintLocation then
        actor:PrintLocation()
    end
end

function ReturnTable:Tick(dt)
    -- Always get actor from self.this (not from a cached variable!)
    if not self.this then
        Print("[ERROR] Tick: actor reference is nil!")
        return
    end

    local actor = self.this  -- Safe: each call gets fresh reference

    -- Validate actor is still valid
    if not actor.GetUUID then
        Print("[ERROR] Tick: actor is invalid!")
        return
    end

    -- Get velocity from self (this IS safe - it's part of the table)
    local velocity = self.Velocity or FVector(0.0, 0.0, 0.0)

    -- Update location
    local loc = actor.ActorLocation
    actor.ActorLocation = loc + velocity * dt

    -- Print location
    if actor.PrintLocation then
        actor:PrintLocation()
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

function ReturnTable:OnOverlap(otherActor)
    if not otherActor then
        return
    end

    if otherActor.PrintLocation then
        otherActor:PrintLocation()
    else
        local loc = otherActor.ActorLocation
        if loc then
            local name = otherActor.GetName and otherActor:GetName() or "Unknown"
            Print(string.format("[Overlap] %s Location: (%.2f, %.2f, %.2f)", name, loc.X, loc.Y, loc.Z))
        end
    end
end

return ReturnTable
