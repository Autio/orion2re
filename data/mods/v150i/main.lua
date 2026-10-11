local repair_technology = require("content.techtree")

require("content.deposits")
events.on("new_game.player_setup", repair_technology)