setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector

-- ✅ Factory Pattern: Return a function that creates instances
-- This ensures each Actor gets its own independent Lua environment
-- Multiple actors can safely share this script!

return function()
    -- These variables are created FRESH for each instance
    local obj = nil
    local ReturnTable = {}

    -- Script-owned properties (can be different per actor)
    ReturnTable.Velocity = FVector(0.0, 0.0, 0.0)

    function ReturnTable:BeginPlay()
        -- Store actor reference (this obj is unique to this instance!)
        obj = self.this
        if not obj then
            Print("[BeginPlay] Lua script missing actor reference.")
            return
        end

        local uuid = obj.GetUUID and obj:GetUUID() or "Unknown"
        local display_name = self.Name or (obj.GetName and obj:GetName() or "Actor")
        Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))

        if obj.PrintLocation then
            obj:PrintLocation()
        end
    end

    function ReturnTable:Tick(dt)
        -- Each instance has its own 'obj' variable!
        if not obj then
            return
        end

        -- Validate actor
        if not obj.GetUUID then
            Print("[ERROR] Tick: actor is invalid!")
            return
        end

        -- Get velocity (this is safe - part of the instance table)
        local velocity = self.Velocity or FVector(0.0, 0.0, 0.0)

        -- Update location
        local loc = obj.ActorLocation
        obj.ActorLocation = loc + velocity * dt

        if obj.PrintLocation then
            obj:PrintLocation()
        end
    end

    function ReturnTable:EndPlay()
        if not obj then
            return
        end

        local uuid = obj.GetUUID and obj:GetUUID() or "Unknown"
        Print(string.format("[EndPlay] %s", uuid))

        if obj.PrintLocation then
            obj:PrintLocation()
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

    -- Return the instance table
    return ReturnTable
end
