#include "pch.h"

#include "doctest/doctest.h"
#include "game/diplomac.h"

namespace TEST_DIPLOMAC {
    void Reset_();
}

namespace {
    void Check_Proposal_(int16_t proposal_type) {
        int16_t response = 0;
        int16_t demand_type = 0;
        int16_t demand_tech = 0;
        int16_t demand_gold = 0;
        int16_t demand_system = 0;
        int16_t demand_tribute = 0;
        diplomac::Check_Treaty_Proposal(
            0, 1, proposal_type, &response, &demand_type, &demand_tech,
            &demand_gold, &demand_system, &demand_tribute);
    }
}

DOCTEST_TEST_CASE("Meklar and Bulrathi starting relations are symmetric") {
    TEST_DIPLOMAC::Reset_();
    MOX::player[0].race = STOCK_RACE_BULRATHI;
    MOX::player[1].race = STOCK_RACE_MEKLAR;

    diplomac::Init_Diplomatic_Relations();

    DOCTEST_CHECK(MOX::player[0].relations[1] == -12);
    DOCTEST_CHECK(MOX::player[1].relations[0] == -12);
    DOCTEST_CHECK(MOX::player[0].base_relations[1] == -12);
    DOCTEST_CHECK(MOX::player[1].base_relations[0] == -12);
}

DOCTEST_TEST_CASE("configured starting relations initialize current and base values") {
    TEST_DIPLOMAC::Reset_();
    MOX::player[0].race = STOCK_RACE_HUMAN;
    MOX::player[1].race = STOCK_RACE_TRILARIAN;
    const int16_t original =
        diplomac::starting_relations[STOCK_RACE_HUMAN][STOCK_RACE_TRILARIAN];
    diplomac::starting_relations[STOCK_RACE_HUMAN][STOCK_RACE_TRILARIAN] = -37;

    diplomac::Init_Diplomatic_Relations();

    DOCTEST_CHECK(MOX::player[0].relations[1] == -37);
    DOCTEST_CHECK(MOX::player[0].base_relations[1] == -37);
    diplomac::starting_relations[STOCK_RACE_HUMAN][STOCK_RACE_TRILARIAN] = original;
}

DOCTEST_TEST_CASE("alternate diplomacy selects proposal-specific modifiers") {
    struct s_proposal_case {
        int16_t proposal_type;
        int16_t alternate_modifier_type;
    };
    const s_proposal_case cases[] = {
        {3, 3},
        {7, 1},
        {8, 1},
    };

    for (uint8_t enabled = 0; enabled <= 1; ++enabled) {
        for (const s_proposal_case& proposal : cases) {
            TEST_DIPLOMAC::Reset_();
            diplomac::alternate_diplomacy = enabled;
            MOX::player[0].time_since_last_attack[1] = 10;

            Check_Proposal_(proposal.proposal_type);

            const int16_t selected =
                enabled != 0 ? proposal.alternate_modifier_type : 0;
            DOCTEST_CHECK(MOX::player[0].treaty_modifier[1] ==
                          (selected == 0 ? -31 : -10));
            DOCTEST_CHECK(MOX::player[0].trade_modifier[1] ==
                          (selected == 1 ? -31 : -10));
            DOCTEST_CHECK(MOX::player[0].tech_exchange_modifier[1] == -10);
            DOCTEST_CHECK(MOX::player[0].peace_modifier[1] ==
                          (selected == 3 ? -61 : -10));
        }
    }

    diplomac::alternate_diplomacy = 0;
}