#include "pch.h"

#include "game/coldraw.h"
#include "doctest/doctest.h"

namespace TEST_COLDRAW {
    void Reset_();
    int Short_Anim_Calls_();
    int Draw_Calls_();
}

DOCTEST_TEST_CASE("food shortage animations are shown with no production or imports") {
    TEST_COLDRAW::Reset_();

    MOX::colony[0].production[ECON_FOOD] = 0;
    MOX::colony[0].imports[ECON_FOOD] = 0;
    MOX::colony[0].maintenance[ECON_FOOD] = 3;

    COLDRAW::Draw_Colony_Prod_Both(0, ECON_FOOD, 0, 20, 300, 30, 0);

    DOCTEST_CHECK(TEST_COLDRAW::Short_Anim_Calls_() == 3);
    DOCTEST_CHECK(TEST_COLDRAW::Draw_Calls_() == 3);
}