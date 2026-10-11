#include "pch.h"

#include "doctest/doctest.h"
#include "game/colbldg.h"
#include "lua/lua_game_ship_identity.h"

namespace TEST_COLBLDG {
    void Reset_();
    void Set_Buildable_(int16_t building_id, bool value);
}

DOCTEST_TEST_CASE("build-list format defaults preserve classic ordering and colors") {
    TEST_COLBLDG::Reset_();

    DOCTEST_CHECK(COLBLDG::build_list_format[COLBLDG::BUILD_LIST_PRODUCT_TRADE_GOODS].order == -2);
    DOCTEST_CHECK(COLBLDG::build_list_format[COLBLDG::BUILD_LIST_PRODUCT_HOUSING].order == -1);

    const COLBLDG::s_build_list_color& normal =
        COLBLDG::build_list_colors[COLBLDG::BUILD_LIST_COLOR_DEFAULT][COLBLDG::BUILD_LIST_HIGHLIGHT_NORMAL];
    const COLBLDG::s_build_list_color& queued =
        COLBLDG::build_list_colors[COLBLDG::BUILD_LIST_COLOR_DEFAULT][COLBLDG::BUILD_LIST_HIGHLIGHT_QUEUED];
    DOCTEST_CHECK(normal.base == 0xCB);
    DOCTEST_CHECK(normal.dim == 0xCA);
    DOCTEST_CHECK(normal.dark == 0xB7);
    DOCTEST_CHECK(queued.base == 0xBB);
    DOCTEST_CHECK(queued.dim == 0xBA);
    DOCTEST_CHECK(queued.dark == 0xB8);
}

DOCTEST_TEST_CASE("build-list ordering includes Housing and Trade Goods") {
    TEST_COLBLDG::Reset_();

    static char alpha[] = "Alpha";
    static char zeta[] = "Zeta";
    TECHDATA::buildings[BUILDING_ARMOR_BARRACKS].name = alpha;
    TECHDATA::buildings[BUILDING_AUTOMATED_FACTORY].name = zeta;
    TEST_COLBLDG::Set_Buildable_(BUILDING_ARMOR_BARRACKS, true);
    TEST_COLBLDG::Set_Buildable_(BUILDING_AUTOMATED_FACTORY, true);

    COLBLDG::build_list_format[COLBLDG::BUILD_LIST_PRODUCT_HOUSING].order = -4;
    COLBLDG::build_list_format[COLBLDG::BUILD_LIST_PRODUCT_TRADE_GOODS].order = 4;

    COLBLDG::Calculate_Building_Array();

    DOCTEST_REQUIRE(COLBLDG::n_building_indexes == 4);
    DOCTEST_CHECK(COLBLDG::building_indexes[0] == COLONY_PRODUCTION_HOUSING);
    DOCTEST_CHECK(COLBLDG::building_indexes[1] == BUILDING_ARMOR_BARRACKS);
    DOCTEST_CHECK(COLBLDG::building_indexes[2] == BUILDING_AUTOMATED_FACTORY);
    DOCTEST_CHECK(COLBLDG::building_indexes[3] == COLONY_PRODUCTION_TRADE_GOODS);
}

DOCTEST_TEST_CASE("disabled Housing is omitted from the build list") {
    TEST_COLBLDG::Reset_();
    COLCALC::disable_housing = 1;

    COLBLDG::Calculate_Building_Array();

    DOCTEST_REQUIRE(COLBLDG::n_building_indexes == 1);
    DOCTEST_CHECK(COLBLDG::building_indexes[0] ==
                  COLONY_PRODUCTION_TRADE_GOODS);
}

DOCTEST_TEST_CASE("enemy outpost is not a colony base target") {
    TEST_COLBLDG::Reset_();

    for (int16_t i = 0; i < 5; ++i) {
        MOX::star[0].planet_index[i] = i;
        MOX::planet[i].planet_type = PLANET_TYPE_PLANET;
    }

    MOX::planet[0].colony_index = 0;
    MOX::colony[0].owner = 0;

    MOX::planet[1].colony_index = -1;

    MOX::planet[2].colony_index = 1;
    MOX::colony[1].owner = 0;
    MOX::colony[1].outpost_flag = 1;

    MOX::planet[3].colony_index = 2;
    MOX::colony[2].owner = 1;
    MOX::colony[2].outpost_flag = 1;

    MOX::planet[4].colony_index = -1;
    MOX::planet[4].planet_type = PLANET_TYPE_ASTEROID;

    DOCTEST_CHECK(COLBLDG::Star_N_Uncolonized_Worlds(0) == 2);
}

DOCTEST_TEST_CASE("queuing the last legal Colony Base removes it from the military list") {
    TEST_COLBLDG::Reset_();

    MOX::colony[0].owner = 0;
    MOX::colony[0].planet = 0;
    MOX::planet[0].colony_index = 0;
    MOX::planet[0].planet_type = PLANET_TYPE_PLANET;
    MOX::planet[0].star_index = 0;
    MOX::planet[1].colony_index = -1;
    MOX::planet[1].planet_type = PLANET_TYPE_PLANET;
    MOX::planet[1].star_index = 0;
    MOX::star[0].planet_index[0] = 0;
    MOX::star[0].planet_index[1] = 1;
    TEST_COLBLDG::Set_Buildable_(BUILDING_COLONY_BASE, true);

    COLBLDG::Calculate_Military_Array();
    DOCTEST_REQUIRE(COLBLDG::n_military_indexes == 2);
    DOCTEST_CHECK(COLBLDG::military_indexes[0] == BUILDING_COLONY_BASE);

    COLBLDG::current_item[0] = BUILDING_COLONY_BASE;
    COLBLDG::Calculate_Military_Array();

    DOCTEST_CHECK(COLBLDG::n_military_indexes == 0);
}

DOCTEST_TEST_CASE("pending refit design owns an independent transient copy") {
    s_ship_design source = {};
    strcpy(source.name, "Refit");
    source.size = SHIP_SIZE_LARGE;
    source.cost = 321;

    COLBLDG::Set_Pending_Refit_Design(7, &source);
    source.cost = 999;

    s_ship_design* pending = COLBLDG::Pending_Refit_Design();
    DOCTEST_REQUIRE(pending != nullptr);
    DOCTEST_CHECK(strcmp(pending->name, "Refit") == 0);
    DOCTEST_CHECK(pending->size == SHIP_SIZE_LARGE);
    DOCTEST_CHECK(pending->cost == 321);
    DOCTEST_CHECK(COLBLDG::refitting_ship_n == 7);

    COLBLDG::Clear_Pending_Refit_Design();
    DOCTEST_CHECK(COLBLDG::Pending_Refit_Design() == nullptr);
}

DOCTEST_TEST_CASE("ship allocation appends without a pre-seeded owner") {
    TEST_COLBLDG::Reset_();
    MOX::colony[0].planet = 0;
    MOX::planet[0].star_index = 0;
    MOX::NUM_SHIPS = 1;
    MOX::ship[0].owner = 0;
    MOX::ship[0].status = 0;
    s_ship_design design = {};
    design.ship_type = SHIP_TYPE_OUTPOST;

    const int16_t ship_idx =
        COLBLDG::Create_Ship_From_Design(1, &design, 0);

    DOCTEST_REQUIRE(ship_idx == 1);
    DOCTEST_CHECK(MOX::NUM_SHIPS == 2);
    DOCTEST_CHECK(MOX::ship[ship_idx].owner == 1);
    DOCTEST_CHECK(MOX::ship[ship_idx].d.ship_type == SHIP_TYPE_OUTPOST);
    DOCTEST_CHECK(orion2re::lua::game::Ship_Generation(ship_idx) == 1);
}

DOCTEST_TEST_CASE("ship allocation reuses only an owner-matched destroyed slot") {
    TEST_COLBLDG::Reset_();
    MOX::colony[0].planet = 0;
    MOX::planet[0].star_index = 0;
    MOX::NUM_SHIPS = 2;
    MOX::ship[0].owner = 0;
    MOX::ship[0].status = 5;
    MOX::ship[1].owner = 1;
    MOX::ship[1].status = 5;
    s_ship_design design = {};

    const int16_t ship_idx =
        COLBLDG::Create_Ship_From_Design(1, &design, 0);

    DOCTEST_REQUIRE(ship_idx == 1);
    DOCTEST_CHECK(MOX::NUM_SHIPS == 2);
    DOCTEST_CHECK(MOX::ship[0].status == 5);
    DOCTEST_CHECK(orion2re::lua::game::Ship_Generation(ship_idx) == 1);
}

DOCTEST_TEST_CASE("ship allocation fails when capacity has no reusable slot") {
    TEST_COLBLDG::Reset_();
    MOX::colony[0].planet = 0;
    MOX::planet[0].star_index = 0;
    MOX::NUM_SHIPS = MAX_SHIPS;
    for (int16_t ship_idx = 0; ship_idx < MAX_SHIPS; ++ship_idx) {
        MOX::ship[ship_idx].owner = 0;
        MOX::ship[ship_idx].status = 0;
    }
    s_ship_design design = {};

    DOCTEST_CHECK(COLBLDG::Create_Ship_From_Design(1, &design, 0) == -1);
    DOCTEST_CHECK(MOX::NUM_SHIPS == MAX_SHIPS);
}

DOCTEST_TEST_CASE("moving a Repeat pair forward preserves unrelated queue entries") {
    TEST_COLBLDG::Reset_();
    const int16_t initial[7] = {
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_RESEARCH_LAB,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_SPACEPORT,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    memcpy(COLBLDG::current_item, initial, sizeof(initial));

    COLBLDG::Insert_Before_Queue_Slot(1, 0);

    const int16_t expected[7] = {
        BUILDING_RESEARCH_LAB,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_SPACEPORT,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    DOCTEST_CHECK(memcmp(COLBLDG::current_item, expected, sizeof(expected)) == 0);
    DOCTEST_CHECK(COLBLDG::active_prod[0] == -1);
}

DOCTEST_TEST_CASE("moving before a Repeat marker preserves its product pair") {
    TEST_COLBLDG::Reset_();
    const int16_t initial[7] = {
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_RESEARCH_LAB,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_SPACEPORT,
        BUILDING_MARINE_BARRACKS,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    memcpy(COLBLDG::current_item, initial, sizeof(initial));

    COLBLDG::Insert_Before_Queue_Slot(4, 2);

    const int16_t expected[7] = {
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_MARINE_BARRACKS,
        BUILDING_RESEARCH_LAB,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_SPACEPORT,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    DOCTEST_CHECK(memcmp(COLBLDG::current_item, expected, sizeof(expected)) == 0);
    DOCTEST_CHECK(COLBLDG::active_prod[0] == -1);
}

DOCTEST_TEST_CASE("moving a Repeat pair backward preserves unrelated queue entries") {
    TEST_COLBLDG::Reset_();
    const int16_t initial[7] = {
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_RESEARCH_LAB,
        BUILDING_SPACEPORT,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_MARINE_BARRACKS,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    memcpy(COLBLDG::current_item, initial, sizeof(initial));

    COLBLDG::Insert_Before_Queue_Slot(3, 0);

    const int16_t expected[7] = {
        BUILDING_SPACEPORT,
        COLONY_PRODUCTION_REPEAT,
        BUILDING_AUTOMATED_FACTORY,
        BUILDING_RESEARCH_LAB,
        BUILDING_MARINE_BARRACKS,
        COLONY_PRODUCTION_NONE,
        COLONY_PRODUCTION_NONE,
    };
    DOCTEST_CHECK(memcmp(COLBLDG::current_item, expected, sizeof(expected)) == 0);
    DOCTEST_CHECK(COLBLDG::active_prod[0] == -1);
}