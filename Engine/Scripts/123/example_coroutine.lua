setmetatable(_ENV, { __index = EngineTypes })

local FVector = EngineTypes.FVector

-- ✅ Factory Pattern - Safe for multiple actors sharing the same script!
return function()
    local ReturnTable = {}

    ReturnTable.moveComplete = false

    function ReturnTable:BeginPlay()
        if not self.this then
            Print("[BeginPlay] Lua script missing actor reference.")
            return
        end

        local uuid = self.this.GetUUID and self.this:GetUUID() or "Unknown"
        local display_name = self.Name or (self.this.GetName and self.this:GetName() or "Actor")
        Print(string.format("[BeginPlay] %s (%s)", display_name, uuid))

        -- Start the AI coroutine
        Print("[AI] Starting AI_Routine coroutine...")
        StartCoroutine(self, "AI_Routine")
    end

    -- Example: Simple patrol AI with coroutine
    function ReturnTable:AI_Routine()
        Print("[AI] Patrol start")

        for i = 1, 3 do
            -- ✅ Safety check: actor might be destroyed
            if not self.this then
                Print("[AI] Actor destroyed, stopping coroutine")
                return
            end

            Print(string.format("[AI] Moving to waypoint %d", i))

            -- Move to position
            local targetX = 10.0 * i
            self.this.ActorLocation = FVector(targetX, 0.0, 0.0)

            -- Wait for 1 second
            Print(string.format("[AI] Arrived at waypoint %d, waiting 1 second...", i))
            wait(1.0)

            -- ✅ Check again after wait (actor might be destroyed during wait)
            if not self.this then
                Print("[AI] Actor destroyed during wait, stopping coroutine")
                return
            end

            -- Play animation (simulated)
            Print(string.format("[AI] Playing patrol animation at waypoint %d", i))
            wait(0.5)
        end

        Print("[AI] Patrol complete!")
    end

    -- Example: Wait until condition
    function ReturnTable:AI_WaitUntilExample()
        Print("[AI] Waiting until health < 50...")

        wait_until(function()
            return self.health < 50
        end)

        Print("[AI] Health is low! Running away!")
    end

    function ReturnTable:Tick(dt)
        if not self.this then
            return
        end

        -- You can still use Tick for other logic
        -- Coroutines run independently
    end

    function ReturnTable:EndPlay()
        if not self.this then
            return
        end

        local uuid = self.this.GetUUID and self.this:GetUUID() or "Unknown"
        Print(string.format("[EndPlay] %s", uuid))
    end

    return ReturnTable
end
