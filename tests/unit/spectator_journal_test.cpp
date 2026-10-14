#include "doctest/doctest.h"
#include "spectator/journal.h"

DOCTEST_TEST_CASE("journal preserves record boundaries and escapes legacy names") {
    spectator_journal::s_turn turn;
    turn.session = "test";
    turn.stardate = 35000;
    turn.empires.push_back({0, "A\"\\\n\xE4", 4, 3, 12, 4, false});
    const auto json = spectator_journal::Serialize_Turn(turn);
    CHECK(json.find("A\\\"\\\\\\u000a\\u00e4") != std::string::npos);
    CHECK(json.find("\"stardate\":35000") != std::string::npos);
    CHECK(json.find('\n') == json.size() - 1);
    CHECK(json.back() == '\n');
}

DOCTEST_TEST_CASE("journal bounds events and reports omissions") {
    spectator_journal::s_turn turn;
    for (int i = 0; i < 300; ++i) {
        spectator_journal::Add_Event(&turn, {0, 1, "evaluation", "text", "evidence"});
    }
    CHECK(turn.events.size() == 256);
    CHECK(turn.dropped_events == 44);
    CHECK(spectator_journal::Serialize_Turn(turn).find("\"dropped_events\":44") != std::string::npos);
}

DOCTEST_TEST_CASE("journal reports inaccessible output instead of claiming success") {
    spectator_journal::s_turn turn;
    CHECK_FALSE(spectator_journal::Append_Turn("missing-journal-directory/output.jsonl", turn));
}
