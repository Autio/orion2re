return {
    generated = function(event)
        if event.planet_id == 0 then
            local planet = game.planets:get(event.planet_id)
            if planet.size ~= defines.planet_size.huge or
                    planet.climate ~= defines.climate.gaia or
                    planet.special ~= defines.planet_special.gold_deposits then
                error("planet event did not observe VDC home-system state")
            end
            planet.minerals = defines.minerals.rich
            planet.gravity = defines.gravity.normal
        end
    end,
}