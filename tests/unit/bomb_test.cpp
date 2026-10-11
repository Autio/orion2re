#include "pch.h"

#include "doctest/doctest.h"

namespace TEST_TOWN_ZERO {
    void Reset_();
}

DOCTEST_TEST_CASE("zero-population colony defenses are valid bomb targets") {
    TEST_TOWN_ZERO::Reset_();

    s_colony* colony = &MOX::colony[0];
    colony->buildings[BUILDING_MISSILE_BASE] = 1;
    colony->buildings[BUILDING_ROBOTIC_FACTORY] = 1;

    s_combat_bomb_result result = {};
    result.attacker_idx = 0;

    DOCTEST_REQUIRE(BOMB::Resolve_Bomb_Hit(colony, &result, 1) == 1);
    DOCTEST_CHECK(colony->buildings[BUILDING_MISSILE_BASE] == 0);
    DOCTEST_CHECK(result.buildings_destroyed[BUILDING_MISSILE_BASE] == 1);
    DOCTEST_CHECK(result.colony_wiped_out == 0);
}

DOCTEST_TEST_CASE("bombing the final building destroys a zero-population colony") {
    TEST_TOWN_ZERO::Reset_();

    s_colony* colony = &MOX::colony[0];
    colony->buildings[BUILDING_ROBOTIC_FACTORY] = 1;

    s_combat_bomb_result result = {};
    result.attacker_idx = 0;

    DOCTEST_REQUIRE(BOMB::Resolve_Bomb_Hit(colony, &result, 1) == 1);
    DOCTEST_CHECK(result.damage_occurred == 1);
    DOCTEST_CHECK(result.buildings_destroyed[BUILDING_ROBOTIC_FACTORY] == 1);
    DOCTEST_CHECK(result.colony_wiped_out == 1);
    DOCTEST_CHECK(MOX::player[0].colony_war_losses[0] == 10);
}

DOCTEST_TEST_CASE("Capitol bombardment follows the configured vulnerability") {
    TEST_TOWN_ZERO::Reset_();

    s_colony* colony = &MOX::colony[0];
    colony->buildings[BUILDING_CAPITOL] = 1;
    s_combat_bomb_result result = {};
    result.attacker_idx = 0;

    BOMB::capitol_can_be_bombed = false;
    DOCTEST_CHECK(BOMB::Resolve_Bomb_Hit(colony, &result, 1) == 0);
    DOCTEST_CHECK(colony->buildings[BUILDING_CAPITOL] == 1);

    BOMB::capitol_can_be_bombed = true;
    DOCTEST_REQUIRE(BOMB::Resolve_Bomb_Hit(colony, &result, 1) == 1);
    DOCTEST_CHECK(colony->buildings[BUILDING_CAPITOL] == 0);
    DOCTEST_CHECK(result.buildings_destroyed[BUILDING_CAPITOL] == 1);
    DOCTEST_CHECK(result.colony_wiped_out == 1);

    BOMB::capitol_can_be_bombed = false;
}