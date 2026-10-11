return function(event)
    local player = game.players:get(event.player_id)
    local homeworld = player.home_planet
    if getmetatable(event) ~= "immutable Lua event object" or
            event.system_id ~= player.home_system.id or event.extra ~= nil or
            pcall(function() event.system_id = -1 end) then
        error("invalid home-system event payload")
    end

    if player:has_trait("high_gravity") then
        if player:has_trait("large_home_world") then
            homeworld.size = defines.planet_size.huge
        else
            homeworld.size = defines.planet_size.large
        end
    end

    if player:has_trait("lucky") then
        if player:has_trait("artifacts_home_world") then
            homeworld:set_special(defines.planet_special.orion)
        else
            homeworld:set_special(defines.planet_special.gold_deposits)
        end
    end

    if player:has_trait("lucky") and
            player:has_trait("low_gravity") and not player.is_human then
        homeworld.size = defines.planet_size.small
        homeworld.climate = defines.climate.gaia
    end
end