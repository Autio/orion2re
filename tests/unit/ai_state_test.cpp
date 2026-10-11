#include "pch.h"

#include "doctest/doctest.h"

namespace TEST_AI_STATE {
    void Reset_();
}

DOCTEST_TEST_CASE("AI state retains a completed turn and releases it on replacement") {
    TEST_AI_STATE::Reset_();

    ai_state::Begin_Turn();
    const ai_state::s_snapshot* first = ai_state::Get_Active();
    DOCTEST_REQUIRE(first != nullptr);
    DOCTEST_CHECK(ai_state::Get_Completed() == nullptr);
    DOCTEST_CHECK(first->stardate == 35010);
    DOCTEST_CHECK(first->allocated_bytes < 20u * 1024u * 1024u);
    DOCTEST_CHECK(AIDATA::route_star_count == 72);
    DOCTEST_CHECK(AIDATA::route_player_count == 4);
    DOCTEST_CHECK(AIDATA::route_ship_count == 500);
    DOCTEST_CHECK(ai_state::Is_Valid(first));
    DOCTEST_CHECK(AIDATA::ai_player_info == first->ai_player_info);
    AIDATA::N_Turns_From_To(71, 70, 3) = 9;
    AIDATA::Ship_N_Turns_To(499, 71) = 6;
    DOCTEST_CHECK(first->n_turns_from_to[first->Route_Index(71, 70, 3)] == 9);
    DOCTEST_CHECK(first->ship_n_turns_to[first->Ship_Route_Index(499, 71)] == 6);

    (*AIDATA::ai_player_info)[0].colony_count = 4;
    AIDATA::n_all_ships = 7;
    ai_state::Complete_Turn();

    const ai_state::s_snapshot* completed = ai_state::Get_Completed();
    DOCTEST_REQUIRE(completed != nullptr);
    DOCTEST_CHECK(ai_state::Get_Active() == nullptr);
    DOCTEST_CHECK((*completed->ai_player_info)[0].colony_count == 4);
    DOCTEST_CHECK(completed->n_all_ships == 7);
    DOCTEST_CHECK(AIDATA::ai_player_info == completed->ai_player_info);

    MOX::stardate = 35020;
    const void* route_storage = first->n_turns_from_to;
    ai_state::Begin_Turn();
    DOCTEST_CHECK(ai_state::Get_Completed() == nullptr);
    DOCTEST_REQUIRE(ai_state::Get_Active() != nullptr);
    DOCTEST_CHECK(ai_state::Get_Active()->stardate == 35020);
    DOCTEST_CHECK(ai_state::Get_Active()->n_turns_from_to == route_storage);
    DOCTEST_CHECK(ai_state::Get_Active()->n_turns_from_to[0] == 0xFF);
    DOCTEST_CHECK(ai_state::Get_Active()->n_all_ships == 0);

    ai_state::Shutdown();
    DOCTEST_CHECK(ai_state::Get_Active() == nullptr);
    DOCTEST_CHECK(ai_state::Get_Completed() == nullptr);
    DOCTEST_CHECK(AIDATA::ai_player_info == nullptr);
    DOCTEST_CHECK(AIDATA::base_planet_values == &AIDATA::g_base_planet_values);
    DOCTEST_CHECK_FALSE(ai_state::Is_Valid(nullptr));
}

DOCTEST_TEST_CASE("AI state does not retain debugger data when the debugger is disabled") {
    TEST_AI_STATE::Reset_();
    debug_overlay::g_enabled = 0;

    ai_state::Begin_Turn();
    DOCTEST_CHECK(ai_state::Get_Active() == nullptr);
    DOCTEST_CHECK(ai_state::Prepare_Colony_State(0) == nullptr);

    ai_state::Complete_Turn();
    DOCTEST_CHECK(ai_state::Get_Active() == nullptr);
    DOCTEST_CHECK(ai_state::Get_Completed() == nullptr);
    DOCTEST_CHECK(AIDATA::ai_player_info != nullptr);
    DOCTEST_CHECK(AIDATA::route_star_count == 72);

    ai_state::Shutdown();
}