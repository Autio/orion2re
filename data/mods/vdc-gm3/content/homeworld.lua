return function(event)
    local player = game.players:get(event.player_id)
    local homeworld = game.planets:get(player.home_planet.id)
    local home_system = game.systems:get(event.system_id)

    if player:has_trait("high_gravity") then
        for _, planet in ipairs(home_system.planets:all()) do
            if planet.climate == defines.climate.swamp then
                planet.size = defines.planet_size.huge
            end
        end
        if player:has_trait("large_home_world") then
            homeworld.size = defines.planet_size.huge
        end
    end

    if player:has_trait("tolerant") and
            player:has_trait("subterranean") then
        homeworld.climate = defines.climate.gaia
    end

    if player:has_trait("lucky") then
        if player:has_trait("artifacts_home_world") then
            homeworld:set_special(defines.planet_special.orion)
        else
            homeworld:set_special(defines.planet_special.gold_deposits)
        end
    end
end