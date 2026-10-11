local function rejects_capacity(operation)
  local ok, message = pcall(operation)
  if ok or not string.find(message, "capacity exceeded", 1, true) then
    error("capacity operation did not fail as expected")
  end
end

services.register("galaxy_generator", 1, function()
  rejects_capacity(function()
    game.systems:copy(game.systems:get(0))
  end)
  rejects_capacity(function()
    game.planets:copy(game.planets:get(0), game.systems:get(1), 1)
  end)
  rejects_capacity(function()
    game.ships:copy(game.ships:get(0), game.systems:get(0))
  end)
  rejects_capacity(function()
    game.nebulas:create(1, 2, 3)
  end)
end)