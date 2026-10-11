local retained = {}

services.register("galaxy_generator", 1, function()
  retained.system = game.systems:get(0)
  retained.planet = game.planets:get(0)
  retained.ship = game.ships:get(0)
  retained.leader = game.leaders:get(0)
  retained.nebula = game.nebulas:create(10, 20, 1)
  if candidate_should_fail then
    error("requested candidate failure")
  end
end)

events.on("game.initialized", function()
  if retained.system.valid or retained.planet.valid or
      retained.ship.valid or retained.leader.valid or
      retained.nebula.valid then
    error("candidate handle remained valid")
  end
  if pcall(function() return retained.system.x end) or
      pcall(function() return retained.planet.size end) or
      pcall(function() return retained.ship.x end) or
      pcall(function() return retained.leader.name end) or
      pcall(function() return retained.nebula.x end) then
    error("expired candidate handle remained readable")
  end
  if retained.system == game.systems:get(0) or
      retained.planet == game.planets:get(0) or
      retained.ship == game.ships:get(0) or
      retained.leader == game.leaders:get(0) then
    error("candidate handle retargeted a live object")
  end
end)