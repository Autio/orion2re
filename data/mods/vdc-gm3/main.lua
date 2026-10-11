local repair_technology = require("content.cryogen")
local repair_homeworld = require("content.homeworld")

events.on("new_game.player_setup", repair_technology)
events.on("new_game.home_system_generated", repair_homeworld)