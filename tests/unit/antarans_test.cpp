#include "pch.h"

#include <cstring>

#include "doctest/doctest.h"
#include "game/antarans.h"

namespace {
    const antarans::s_antaran_fleet_config classic_fleet_config = {
        .growth_delay = {200, 100, 0},
        .resource_growth = {
            .frequency = 25,
            .offensive_multiplier = {100, 150, 200},
            .defensive_multiplier = {100, 150, 200},
        },
        .attack = {
            .roll_range = 200,
            .minimum_offensive_force_shift = 2,
            .impossible_human_weight_multiplier = 3,
        },
        .ship_limit = {
            .offensive = {4, 4, 3, 2, 2},
            .defensive = {0, 0, 3, 2, 7},
        },
        .ship_cost = {2, 5, 12, 30, 75},
        .retirement = {
            {100, {100, 150, 200}},
            {199, {100, 150, 200}},
        },
    };

    void Reset_() {
        antarans::fleet_config = classic_fleet_config;
        antarans::Reset_Fleet_Runtime();
        MOX::settings = {};
        MOX::settings.game_difficulty = GAME_DIFFICULTY_AVERAGE;
        MOX::settings.starting_civilization_level =
            STARTING_CIVILIZATION_LEVEL_AVERAGE;
        MOX::stardate = 35000;
        std::memset(&MOX::antarans, 0, sizeof(MOX::antarans));
    }
}

DOCTEST_TEST_CASE("Antaran delay follows configured classic civilization bands") {
    Reset_();
    antarans::fleet_config.growth_delay[antarans::ANTARAN_CIVILIZATION_PREWARP] = 105;
    antarans::fleet_config.growth_delay[antarans::ANTARAN_CIVILIZATION_AVERAGE] = 80;
    antarans::fleet_config.growth_delay[antarans::ANTARAN_CIVILIZATION_ADVANCED] = 15;

    MOX::settings.starting_civilization_level = STARTING_CIVILIZATION_LEVEL_PRE_WARP;
    DOCTEST_CHECK(antarans::Antaran_Delay() == 105);
    MOX::settings.starting_civilization_level = STARTING_CIVILIZATION_LEVEL_AVERAGE;
    DOCTEST_CHECK(antarans::Antaran_Delay() == 80);
    MOX::settings.starting_civilization_level = STARTING_CIVILIZATION_LEVEL_ADVANCED;
    DOCTEST_CHECK(antarans::Antaran_Delay() == 15);
    MOX::settings.starting_civilization_level = STARTING_CIVILIZATION_LEVEL_POST_WARP;
    DOCTEST_CHECK(antarans::Antaran_Delay() == 80);
}

DOCTEST_TEST_CASE("Antaran delay preserves unsigned word timing semantics") {
    Reset_();
    MOX::settings.starting_civilization_level =
        STARTING_CIVILIZATION_LEVEL_PRE_WARP;
    antarans::fleet_config.growth_delay[
        antarans::ANTARAN_CIVILIZATION_PREWARP] = 65535;
    antarans::fleet_config.resource_growth.frequency = 25;

    DOCTEST_CHECK(antarans::Antaran_Delay() == 65535);

    MOX::stardate = 35024;
    antarans::Increment_Antaran_Resource_Level();
    DOCTEST_CHECK(MOX::antarans.resource_offensive == 0);
    DOCTEST_CHECK(MOX::antarans.resource_defensive == 0);

    MOX::stardate = 35000 + 65535 + 25;
    antarans::Increment_Antaran_Resource_Level();
    DOCTEST_CHECK(MOX::antarans.resource_offensive == 1);
    DOCTEST_CHECK(MOX::antarans.resource_defensive == 1);

    MOX::stardate = 35101;
    int16_t resources =
        antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_RAIDER];
    int16_t ships[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    uint8_t limits[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    limits[antarans::ANTARAN_SHIP_RAIDER] = 1;

    antarans::Build_Antaran_Ships(&resources, ships, limits, 0);

    DOCTEST_CHECK(ships[antarans::ANTARAN_SHIP_RAIDER] == 1);
}

DOCTEST_TEST_CASE("Antaran resource growth uses independent configured fleet multipliers") {
    Reset_();
    MOX::settings.starting_civilization_level = STARTING_CIVILIZATION_LEVEL_ADVANCED;
    antarans::fleet_config.resource_growth.frequency = 10;
    antarans::fleet_config.resource_growth.offensive_multiplier[
        antarans::ANTARAN_DIFFICULTY_NORMAL] = 200;
    antarans::fleet_config.resource_growth.defensive_multiplier[
        antarans::ANTARAN_DIFFICULTY_NORMAL] = 100;
    MOX::stardate = 35010;

    antarans::Increment_Antaran_Resource_Level();

    DOCTEST_CHECK(MOX::antarans.resource_offensive == 2);
    DOCTEST_CHECK(MOX::antarans.resource_defensive == 1);
}

DOCTEST_TEST_CASE("Antaran retirement uses direct normal limits and independent divisors") {
    Reset_();
    antarans::fleet_config.retirement[antarans::ANTARAN_RETIREMENT_RAIDER]
        .normal_turn_limit = 80;
    antarans::fleet_config.retirement[antarans::ANTARAN_RETIREMENT_RAIDER]
        .divisor[antarans::ANTARAN_DIFFICULTY_HARD] = 250;
    antarans::fleet_config.retirement[antarans::ANTARAN_RETIREMENT_MARAUDER]
        .normal_turn_limit = 120;
    antarans::fleet_config.retirement[antarans::ANTARAN_RETIREMENT_MARAUDER]
        .divisor[antarans::ANTARAN_DIFFICULTY_IMPOSSIBLE] = 400;

    DOCTEST_CHECK(antarans::No_Small_Ships_After_N_Turns() == 80);
    DOCTEST_CHECK(antarans::No_Medium_Ships_After_N_Turns() == 120);

    MOX::settings.game_difficulty = GAME_DIFFICULTY_HARD;
    DOCTEST_CHECK(antarans::No_Small_Ships_After_N_Turns() == 50);
    MOX::settings.game_difficulty = GAME_DIFFICULTY_IMPOSSIBLE;
    DOCTEST_CHECK(antarans::No_Medium_Ships_After_N_Turns() == 50);
}

DOCTEST_TEST_CASE("Antaran force values derive Titan discounts from configured base costs") {
    Reset_();
    antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_RAIDER] = 10;
    antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_INTRUDER] = 30;
    MOX::antarans.defensive_ships[antarans::ANTARAN_SHIP_RAIDER] = 2;
    MOX::antarans.offensive_ships[antarans::ANTARAN_SHIP_INTRUDER] = 3;

    DOCTEST_CHECK(antarans::Antaran_Defensive_Force() == 20);
    DOCTEST_CHECK(antarans::Antaran_Offensive_Force() == 90);

    MOX::antarans.offensive_titan_built = 1;
    DOCTEST_CHECK(antarans::Antaran_Defensive_Force() == 18);
    DOCTEST_CHECK(antarans::Antaran_Offensive_Force() == 81);

    antarans::Reset_Fleet_Runtime();
    DOCTEST_CHECK(MOX::antarans.offensive_titan_built == 0);
    DOCTEST_CHECK(antarans::Antaran_Offensive_Force() == 90);
}

DOCTEST_TEST_CASE("Antaran exhausted offensive limits remain disabled after losses") {
    Reset_();
    antarans::fleet_config.ship_limit.offensive[antarans::ANTARAN_SHIP_RAIDER] = 1;
    int16_t resources =
        antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_RAIDER];
    int16_t ships[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    uint8_t limits[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    limits[antarans::ANTARAN_SHIP_RAIDER] = 1;

    antarans::Build_Antaran_Ships(&resources, ships, limits, 1);

    DOCTEST_CHECK(ships[antarans::ANTARAN_SHIP_RAIDER] == 1);
    DOCTEST_CHECK(
        (MOX::antarans.offensive_limit_exhausted_mask &
         (1u << antarans::ANTARAN_SHIP_RAIDER)) != 0);

    ships[antarans::ANTARAN_SHIP_RAIDER] = 0;
    resources = antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_RAIDER];
    antarans::Build_Antaran_Ships(&resources, ships, limits, 1);

    DOCTEST_CHECK(ships[antarans::ANTARAN_SHIP_RAIDER] == 0);
}

DOCTEST_TEST_CASE("Defensive Antaran construction can exhaust offensive limits") {
    Reset_();
    antarans::fleet_config.ship_limit.offensive[antarans::ANTARAN_SHIP_RAIDER] = 0;
    antarans::fleet_config.ship_limit.offensive[antarans::ANTARAN_SHIP_MARAUDER] = 0;
    antarans::fleet_config.ship_limit.offensive[antarans::ANTARAN_SHIP_INTRUDER] = 3;
    antarans::fleet_config.ship_limit.defensive[antarans::ANTARAN_SHIP_INTRUDER] = 3;

    int16_t defensive_resources =
        antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_INTRUDER];
    int16_t defensive_ships[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    defensive_ships[antarans::ANTARAN_SHIP_INTRUDER] = 2;

    antarans::Build_Antaran_Ships(
        &defensive_resources,
        defensive_ships,
        antarans::fleet_config.ship_limit.defensive,
        0);

    DOCTEST_CHECK(defensive_ships[antarans::ANTARAN_SHIP_INTRUDER] == 3);
    DOCTEST_CHECK(
        (MOX::antarans.offensive_limit_exhausted_mask &
         (1u << antarans::ANTARAN_SHIP_INTRUDER)) != 0);

    int16_t offensive_resources =
        antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_INTRUDER];
    int16_t offensive_ships[antarans::ANTARAN_SHIP_CLASS_COUNT] = {};
    offensive_ships[antarans::ANTARAN_SHIP_INTRUDER] = 2;

    antarans::Build_Antaran_Ships(
        &offensive_resources,
        offensive_ships,
        antarans::fleet_config.ship_limit.offensive,
        1);

    DOCTEST_CHECK(offensive_ships[antarans::ANTARAN_SHIP_INTRUDER] == 2);
}

DOCTEST_TEST_CASE("Antaran attack eligibility uses the configured offensive force shift") {
    Reset_();
    antarans::fleet_config.attack.roll_range = 1;
    antarans::fleet_config.ship_cost[antarans::ANTARAN_SHIP_RAIDER] = 10;
    MOX::antarans.offensive_ships[antarans::ANTARAN_SHIP_RAIDER] = 1;
    MOX::antarans.resource_defensive = 1;

    antarans::fleet_config.attack.minimum_offensive_force_shift = 0;
    DOCTEST_CHECK_FALSE(antarans::Antarans_Should_Attack());

    antarans::fleet_config.attack.minimum_offensive_force_shift = 1;
    DOCTEST_CHECK(antarans::Antarans_Should_Attack());
}