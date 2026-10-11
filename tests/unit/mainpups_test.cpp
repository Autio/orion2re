#include "pch.h"

#include "doctest/doctest.h"

namespace TEST_MAINPUPS_LEADER_TECHNOLOGY {
    void Reset_();
}

DOCTEST_TEST_CASE("leader technology text uses the configured government leader") {
    TEST_MAINPUPS_LEADER_TECHNOLOGY::Reset_();
    OFFICER::progression.special_leaders.advanced_government_leader_id = 46;
    char output[80];

    MAINPUPS::Leader_Technology_String(0, 46, output, sizeof(output));
    DOCTEST_CHECK(strcmp(output, "Federation") == 0);

    MAINPUPS::Leader_Technology_String(0, 47, output, sizeof(output));
    DOCTEST_CHECK(output[0] == '\0');
}