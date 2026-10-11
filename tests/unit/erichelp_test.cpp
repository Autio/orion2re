#include "pch.h"

#include "doctest/doctest.h"
#include "game/erichelp.h"
#include "game/fields.h"

namespace {

s_help_box* active_help_list;
int16_t active_help_box_count;
bool help_list_deactivated;

}

namespace fields {

void __cdecl Deactivate_Help_List() {
    help_list_deactivated = true;
}

void __cdecl Set_Help_List(s_help_box* help_data, int16_t count) {
    active_help_list = help_data;
    active_help_box_count = count;
}

}

DOCTEST_TEST_CASE("new-game technology levels share one help entry") {
    active_help_list = nullptr;
    active_help_box_count = 0;
    help_list_deactivated = false;

    ERICHELP::Set_New_Game_Screen_Help_List();

    DOCTEST_CHECK(help_list_deactivated);
    DOCTEST_CHECK(active_help_list == ERICHELP::new_game_screen_help_list);
    DOCTEST_CHECK(active_help_box_count == 11);
    DOCTEST_CHECK(active_help_list[0].help_id == 550);
}