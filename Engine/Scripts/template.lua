setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector

-- ✅ FACTORY PATTERN - Safe for multiple actors sharing the same script!
-- Return a function that creates a new instance for each Actor
return function()
    -- Each instance gets its own variables (not shared!)
    local ReturnTable = {}

    -- Script-owned velocity that drives Tick movement; adjust per actor
    ReturnTable.Velocity = FVector(0.0, 0.0, 0.0)

    function ReturnTable:BeginPlay()
        if not self.this then
            Print("[BeginPlay] Lua script missing actor reference.")
            return
        end

        local uuid = self.this.GetUUID and self.this:GetUUID() or "Unknown"
        local display_name = self.Name or (self.this.GetName and self.this:GetName() or "Actor")
        Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))

        if self.this.PrintLocation then
            self.this:PrintLocation()
        end
    end

    function ReturnTable:EndPlay()
        if not self.this then
            return
        end

        local uuid = self.this.GetUUID and self.this:GetUUID() or "Unknown"
        Print(string.format("[EndPlay] %s", uuid))

        if self.this.PrintLocation then
            self.this:PrintLocation()
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
        if not self.this then
            return
        end

        local velocity = self.Velocity or FVector(0.0, 0.0, 0.0)
        local loc = self.this.ActorLocation
        self.this.ActorLocation = loc + velocity * dt

        if self.this.PrintLocation then
            self.this:PrintLocation()
        end
    end

    return ReturnTable
end
