local function has_deposits(planet)
  return planet.special == defines.planet_special.gold_deposits or
    planet.special == defines.planet_special.gem_deposits
end

events.on("planet.generated", function(event)
  local planet = game.planets:get(event.planet_id)
  if not has_deposits(planet) or
      planet.minerals > defines.minerals.poor then
    return
  end

  if planet.size == defines.planet_size.huge then
    planet.minerals = defines.minerals.poor
  elseif planet.size >= defines.planet_size.small then
    planet.minerals = defines.minerals.abundant
  else
    planet.minerals = defines.minerals.rich
  end
  planet.gravity = defines.gravity.normal
end)