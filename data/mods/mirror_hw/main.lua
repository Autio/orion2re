services.register("galaxy_generator", 1, function()
  local players = game.players:all()
  if #players == 0 then
    return
  end

  local source_player = players[game.rng:integer(0, #players)]
  local source_home = source_player.home_planet
  local source_system = source_home.system
  local source_planets = source_system.planets:all()

  for _, player in ipairs(players) do
    if player ~= source_player then
      local destination_home = player.home_planet
      local destination_system = destination_home.system
      for _, planet in ipairs(destination_system.planets:all()) do
        if planet ~= destination_home then
          planet:delete()
        end
      end

      destination_home:set_orbit(destination_system, source_home.orbit)
      for _, planet in ipairs(source_planets) do
        if planet ~= source_home then
          game.planets:copy(planet, destination_system, planet.orbit)
        end
      end
    end
  end
end)