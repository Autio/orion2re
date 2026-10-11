#include "pch.h"

#include "doctest/doctest.h"
#include "game/bill.h"

namespace TEST_BILL_HISTORY {
    void Reset_();
}

DOCTEST_TEST_CASE("population history includes freighted population") {
    TEST_BILL_HISTORY::Reset_();
    MOX::player[0].total_pop = 12;
    MOX::player[0].settlers_freighted = 3;

    BILL::Record_History();

    DOCTEST_CHECK(MOX::player[0].population_history[7] == 15);
    DOCTEST_CHECK(MOX::bill_savegame[1] == 8);
}