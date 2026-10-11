#include "pch.h"

#include <doctest/doctest.h>

#include "game/tech.h"

TEST_CASE("technology selection ignores activation without a pointer selection") {
    s_player player = {};
    player.current_research_field = 11;
    player.current_research_application = 22;
    s_list_item_research entries[8] = {};

    s_list_item_research* selected = TECH::Get_Selected_Entry(entries);
    REQUIRE(selected == nullptr);
    CHECK_FALSE(TECH::Commit_Selected_Entry(&player, selected));
    CHECK(player.current_research_field == 11);
    CHECK(player.current_research_application == 22);
}

TEST_CASE("technology selection commits the selected application") {
    s_player player = {};
    s_list_item_research entries[8] = {};
    entries[3].current_app_index = 1;
    entries[3].tech_field_id = 17;
    entries[3].selected_app_index = 2;
    entries[3].app_ids[2] = 31;

    s_list_item_research* selected = TECH::Get_Selected_Entry(entries);
    REQUIRE(selected == &entries[3]);
    REQUIRE(TECH::Commit_Selected_Entry(&player, selected));
    CHECK(player.current_research_field == 17);
    CHECK(player.current_research_application == 31);
}

TEST_CASE("technology selection rejects an invalid application index") {
    s_player player = {};
    player.current_research_field = 11;
    player.current_research_application = 22;
    s_list_item_research entry = {};
    entry.tech_field_id = 17;
    entry.selected_app_index = -1;

    CHECK_FALSE(TECH::Commit_Selected_Entry(&player, &entry));
    CHECK(player.current_research_field == 11);
    CHECK(player.current_research_application == 22);
}