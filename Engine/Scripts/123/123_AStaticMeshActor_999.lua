setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector

-- ✅ FACTORY PATTERN - Safe for multiple actors sharing the same script!
-- Return a function that creates a new instance for each Actor
return function()
    -- Each instance gets its own variables (not shared!)
    local obj = nil
    local ReturnTable = {}

    -- Script-owned velocity that drives Tick movement; adjust per actor
    ReturnTable.Velocity = FVector(10.0, 0.0, 0.0)

    function ReturnTable:BeginPlay()
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

    function ReturnTable:Tick(dt)
        if not obj then
            return
        end

        local velocity = self.Velocity or FVector(0.0, 0.0, 0.0)
        local loc = obj.ActorLocation
        obj.ActorLocation = loc + velocity * dt

        if obj.PrintLocation then
            obj:PrintLocation()
        end
    end

    return ReturnTable
end
