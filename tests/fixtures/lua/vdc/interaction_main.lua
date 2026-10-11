local repair_homeworld = require("content.homeworld")
local generated_planet = require("test.generated_rules")

events.on("new_game.home_system_generated", repair_homeworld)
events.on("planet.generated", generated_planet.generated)