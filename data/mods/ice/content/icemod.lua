local function is_unavailable(technologies, application_id)
    return not technologies:is_available(application_id) and
        not technologies:is_researched(application_id)
end

local function replace_application(technologies, removed_id,
        first_id, second_id, choose_first)
    technologies:replace_available(
        removed_id, first_id, second_id, choose_first)
end

return function(event)
    local player = game.players:get(event.player_id)
    if not player:has_trait("uncreative") then return end
    local technologies = player.technologies
    local shield_applications = {
        "class_i_shield",
        "class_iii_shield",
        "class_v_shield",
        "class_vii_shield",
        "class_x_shield",
    }
    for _, application_id in ipairs(shield_applications) do
        if not is_unavailable(technologies, application_id) then
            return
        end
    end

    if technologies:is_available("shield_capacitors") then
        replace_application(technologies, "shield_capacitors",
            "inertial_stabilizer", "tractor_beam",
            game.rng:integer(0, 2) == 1)
    end
    if technologies:is_available("multi_phased_shields") then
        replace_application(technologies, "multi_phased_shields",
            "stealth_field", "warp_dissipator",
            game.rng:integer(0, 2) == 1)
    end
    if technologies:is_available("hard_shields") then
        local choose_cloaking_device =
            game.rng:integer(0, 2) == 1 and
            technologies:is_available("extended_fuel_tanks")
        replace_application(technologies, "hard_shields",
            "cloaking_device", "warp_interdictor",
            choose_cloaking_device)
    end
end