local function is_unavailable(technologies, application_id)
  return not technologies:is_available(application_id) and
    not technologies:is_researched(application_id)
end

local function choose_application(technologies, field_id,
    first_application_id, second_application_id)
  local application_id = second_application_id
  if game.rng:integer(0, 2) == 1 then
    application_id = first_application_id
  end
  technologies:set_field_application(field_id, application_id)
end

return function(event)
  local player = game.players:get(event.player_id)
  local technologies = player.technologies

  if is_unavailable(technologies, "extended_fuel_tanks") then
    technologies:set_available("hard_shields", false)
  end

  if player:has_trait("lithovore") and
      technologies:is_available("weather_control_system") then
    technologies:set_available("weather_control_system", false)
  end

  if not player:has_trait("uncreative") then return end
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
    choose_application(technologies, "ion_fission",
      "ion_drive", "ion_pulse_cannon")
  end
  if technologies:is_available("multi_phased_shields") then
    choose_application(technologies, "multi_phased_physics",
      "phasor", "phasor_rifle")
  end
  if technologies:is_available("hard_shields") then
    choose_application(technologies, "distortion_fields",
      "cloaking_device", "stasis_field")
  end
end