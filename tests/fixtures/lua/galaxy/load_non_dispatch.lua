services.register("galaxy_generator", 1, function()
  storage.generator_calls = (storage.generator_calls or 0) + 1
end)

events.on("game.loaded", function()
  if (storage.generator_calls or 0) ~= 0 then
    error("galaxy generator ran while loading")
  end
end)