#include "pch.h"

#include "doctest/doctest.h"
#include "game/officer.h"
#include "game/report.h"

namespace TEST_REPORT {
    extern int16_t colonizable_planet_count;
    extern int16_t confirmation_result;
    extern bool accept_officer_offer;
    extern s_ship_data* cleared_auto_colonization_ship;

    void Reset_();
    void Add_Colony_Ship_();
    void Add_Colony_Base_();
}

DOCTEST_TEST_CASE("native occupation reports follow the gameplay setting") {
    TEST_REPORT::Reset_();
    MOX::NUM_COLONIES = 1;
    MOX::colony[0].owner = 0;
    MOX::colony[0].occupation_policy = 4;
    MOX::colony[0].n_pops = 1;
    POP::Set_Race(&MOX::colony[0].pop[0], POP::RACE_NATIVE);
    POP::Set_Assigned(&MOX::colony[0].pop[0], true);

    COLONY::allow_native_annihilation = 0;
    DOCTEST_CHECK(REPORT::Need_To_Set_Occupation_Policy(0, 0) == 0);

    COLONY::allow_native_annihilation = 1;
    DOCTEST_CHECK(REPORT::Need_To_Set_Occupation_Policy(0, 0) == 1);
}

DOCTEST_TEST_CASE("outpost prompt remains enabled by default") {
    TEST_REPORT::Reset_();

    DOCTEST_CHECK(REPORT::Colonization(0, 0) == 1);
}

DOCTEST_TEST_CASE("leader offers preserve classic decline behavior by default") {
    TEST_REPORT::Reset_();
    MOX::player[0].officer_for_hire = 3;
    MOX::player[0].stardate_last_offered_hero = 35005;
    MOX::leaders[3].player_index = 0;

    DOCTEST_CHECK(REPORT::Has_Officer_For_Hire(0, 0) == 1);
    DOCTEST_CHECK(MOX::player[0].officer_for_hire == 3);
    DOCTEST_CHECK(REPORT::Has_Officer_For_Hire(0, 1) == 1);
    DOCTEST_CHECK(MOX::leaders[3].status == 4);
    DOCTEST_CHECK(MOX::leaders[3].player_index == 0);
    DOCTEST_CHECK(MOX::player[0].stardate_last_offered_hero == 35005);
    DOCTEST_CHECK(MOX::player[0].officer_for_hire == -1);
}

DOCTEST_TEST_CASE("declining a leader offer keeps it hireable without resetting the timer") {
    TEST_REPORT::Reset_();
    OFFICER::reject_declined_offers = 1;
    MOX::player[0].officer_for_hire = 3;
    MOX::player[0].stardate_last_offered_hero = 35001;
    MOX::leaders[3].player_index = 0;
    MOX::leaders[3].xp = 60;

    DOCTEST_CHECK(REPORT::Has_Officer_For_Hire(0, 1) == 1);
    DOCTEST_CHECK(MOX::leaders[3].status == 4);
    DOCTEST_CHECK(MOX::leaders[3].player_index == 0);
    DOCTEST_CHECK(MOX::leaders[3].xp == 60);
    DOCTEST_CHECK(MOX::leaders[3].eta == 0);
    DOCTEST_CHECK(MOX::player[0].stardate_last_offered_hero == 35001);
    DOCTEST_CHECK(MOX::player[0].officer_for_hire == -1);
}

DOCTEST_TEST_CASE("accepting a leader into the pool records its offer date") {
    TEST_REPORT::Reset_();
    OFFICER::reject_declined_offers = 1;
    TEST_REPORT::accept_officer_offer = true;
    MOX::stardate = 35006;
    MOX::player[0].officer_for_hire = 3;
    MOX::player[0].stardate_last_offered_hero = 35001;
    MOX::leaders[3].player_index = 0;

    DOCTEST_CHECK(REPORT::Has_Officer_For_Hire(0, 1) == 1);
    DOCTEST_CHECK(MOX::leaders[3].status == 0);
    DOCTEST_CHECK(MOX::leaders[3].player_index == 0);
    DOCTEST_CHECK(MOX::leaders[3].xp == 0);
    DOCTEST_CHECK(MOX::player[0].stardate_last_offered_hero == 35005);
    DOCTEST_CHECK(MOX::player[0].officer_for_hire == -1);
}

DOCTEST_TEST_CASE("outpost prompt can be skipped") {
    TEST_REPORT::Reset_();
    REPORT::no_outpost_prompt = 1;

    DOCTEST_CHECK(REPORT::Colonization(0, 0) == 0);
}

DOCTEST_TEST_CASE("initial research prompt can be suppressed") {
    TEST_REPORT::Reset_();
    MOX::stardate = 35001;
    MOX::player[0].current_research_field = TECH_FIELD_STARTING_TECH;
    MOX::player[0].tech_fields[TECH_FIELD_STARTING_TECH] = 3;

    REPORT::initial_tech_prompt = 1;
    DOCTEST_CHECK(REPORT::Set_Initial_Tech(0, 0) == 2);

    REPORT::initial_tech_prompt = 0;
    DOCTEST_CHECK(REPORT::Set_Initial_Tech(0, 0) == 0);

    REPORT::initial_tech_prompt = 1;
}

DOCTEST_TEST_CASE("skipping outpost prompt retains valid colony prompt") {
    TEST_REPORT::Reset_();
    TEST_REPORT::Add_Colony_Ship_();
    TEST_REPORT::colonizable_planet_count = 1;
    REPORT::no_outpost_prompt = 1;

    DOCTEST_CHECK(REPORT::Colonization(0, 0) == 1);
}

DOCTEST_TEST_CASE("skipping outpost prompt suppresses unusable colony prompt") {
    TEST_REPORT::Reset_();
    TEST_REPORT::Add_Colony_Ship_();
    REPORT::no_outpost_prompt = 1;

    DOCTEST_CHECK(REPORT::Colonization(0, 0) == 0);
}

DOCTEST_TEST_CASE("filtered colony prompt retains its ship for assignment cleanup") {
    TEST_REPORT::Reset_();
    TEST_REPORT::Add_Colony_Ship_();
    TEST_REPORT::Add_Colony_Base_();
    TEST_REPORT::confirmation_result = 1;
    REPORT::no_outpost_prompt = 1;

    DOCTEST_CHECK(REPORT::Colonization(0, 1) == 1);
    DOCTEST_CHECK(TEST_REPORT::cleared_auto_colonization_ship == &MOX::ship[1]);
}