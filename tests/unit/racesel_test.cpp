#include "pch.h"

#include <cstring>

#include "doctest/doctest.h"
#include "game/racesel.h"

namespace TEST_RACESEL_DIGEST {
    void Reset_();
    const char* Captured_Paragraph_();
}

DOCTEST_TEST_CASE("race selection digest retains every configured trait") {
    RACESEL::s_stock_race configured_race = {
        .government = GOVERNMENT_UNIFICATION,
        .growth = 1,
        .farming = 1,
        .industry = 1,
        .science = 1,
        .money = 1,
        .defense = 1,
        .attack = 1,
        .ground = 1,
        .spying = 1,
        .lowg_world = 1,
        .highg_world = 1,
        .aquatic = 1,
        .subterranean = 1,
        .large_hw = 1,
        .hw_richness = -1,
        .arti_world = 1,
        .cybernetic = 1,
        .lithovore = 1,
        .repulsive = 1,
        .charismatic = 1,
        .uncreative = 1,
        .creative = 1,
        .tolerant = 1,
        .fantastic_traders = 1,
        .telepathic = 1,
        .lucky = 1,
        .omniscient = 1,
        .stealthy_ships = 1,
        .trans_dimensional = 1,
        .warlord = 1,
    };

    TEST_RACESEL_DIGEST::Reset_();
    memset(RACESEL::race_flag, 0, sizeof(RACESEL::race_flag));
    RACESEL::custom_flag = 0;
    RACESEL::displayed_race = STOCK_RACE_ALKARI;
    RACESEL::old_race = STOCK_RACE_ALKARI;
    RACESEL::race_specials[STOCK_RACE_ALKARI] =
        reinterpret_cast<uint8_t*>(&configured_race);

    RACESEL::Draw_Race_Selection_Screen();

    const char* paragraph = TEST_RACESEL_DIGEST::Captured_Paragraph_();
    DOCTEST_CHECK(std::strlen(paragraph) > 200);
    DOCTEST_CHECK(std::strstr(paragraph, "Poor Home World") != nullptr);
    DOCTEST_CHECK(
        std::strstr(
            paragraph,
            "Stealthy Ships, Trans-Dimensional, Warlord, and Unification"
        ) != nullptr
    );
}