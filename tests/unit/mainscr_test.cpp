#include "pch.h"

#include <string>
#include <vector>

#include "config/config_loader.h"
#include "doctest/doctest.h"
#include "game/mainscr.h"

namespace {
    static_assert(std::size(MAINSCR::mouse_list_relocation) == 2);

    std::vector<std::string> registered_hotkeys;
    int reload_count;
    int full_reload_count;
    bool reload_result;
    bool observed_network_game;
    std::string user_box_message;
} // namespace

namespace MOX {
    uint8_t draw_main_screen_to_back;
    int8_t game_type;
} // namespace MOX

namespace fields {
    int16_t fields_count;

    int16_t __cdecl Add_Multi_Hot_Key_Field(const char* hotkey) {
        registered_hotkeys.emplace_back(hotkey);
        return fields_count++;
    }
} // namespace fields

namespace config {
    bool Reload_All_Active_Config_Groups(s_cfg_diagnostic* diagnostic) {
        full_reload_count++;
        if (!reload_result) {
            diagnostic->message = "reload failed";
        }
        return reload_result;
    }

    bool Reload_Active_Config_Groups(bool network_game,
        s_cfg_diagnostic* diagnostic,
        t_config_group_merger,
        t_config_group_reload_policy) {
        reload_count++;
        observed_network_game = network_game;
        if (!reload_result) {
            diagnostic->message = "reload failed";
        }
        return reload_result;
    }
} // namespace config

namespace HAROLD {
    int16_t __cdecl User_Box(uint8_t*, const char* message, int16_t) {
        user_box_message = message;
        return 0;
    }
} // namespace HAROLD

DOCTEST_TEST_CASE("main-screen income optionally includes nonzero tax rates") {
    const bool saved_setting = MAINSCR::main_screen_tax_rate;
    const std::string saved_template = MAINSCR::main_screen_income_template;
    snprintf(MAINSCR::main_screen_income_template,
        sizeof(MAINSCR::main_screen_income_template),
        "%s",
        "$SIGN$$INCOME$ $UNIT$ ($TAX_RATE$%)");
    char output[128];

    MAINSCR::main_screen_tax_rate = false;
    MAINSCR::Format_Main_Screen_Income(output, sizeof(output), 1200, "1,200", "BC", 20);
    DOCTEST_CHECK(std::string(output) == "+1,200 BC");

    MAINSCR::main_screen_tax_rate = true;
    MAINSCR::Format_Main_Screen_Income(output, sizeof(output), 1200, "1,200", "BC", 0);
    DOCTEST_CHECK(std::string(output) == "+1,200 BC");
    for (uint8_t rate = 10; rate <= 50; rate += 10) {
        MAINSCR::Format_Main_Screen_Income(output, sizeof(output), 1200, "1,200", "BC", rate);
        DOCTEST_CHECK(std::string(output) == "+1,200 BC (" + std::to_string(rate) + "%)");
    }
    MAINSCR::Format_Main_Screen_Income(output, sizeof(output), -1200, "-1.200", "MC", 50);
    DOCTEST_CHECK(std::string(output) == "-1.200 MC (50%)");
    MAINSCR::Format_Main_Screen_Income(output, sizeof(output), 0, "0", "BC", 10);
    DOCTEST_CHECK(std::string(output) == "+0 BC (10%)");

    snprintf(MAINSCR::main_screen_income_template,
        sizeof(MAINSCR::main_screen_income_template),
        "%s",
        "$TAX_RATE$%: $SIGN$$INCOME$ $UNIT$");
    MAINSCR::Format_Main_Screen_Income(output, sizeof(output), 1200, "1,200", "BC", 20);
    DOCTEST_CHECK(std::string(output) == "20%: +1,200 BC");
    char short_output[5];
    MAINSCR::Format_Main_Screen_Income(short_output, sizeof(short_output), 1200, "1,200", "BC", 20);
    DOCTEST_CHECK(std::string(short_output) == "20%:");

    MAINSCR::main_screen_tax_rate = saved_setting;
    snprintf(MAINSCR::main_screen_income_template,
        sizeof(MAINSCR::main_screen_income_template),
        "%s",
        saved_template.c_str());
}

DOCTEST_TEST_CASE("main-screen relocation mode uses the restored destination cursor") {
    const s_mouse_list& relocation = MAINSCR::mouse_list_relocation[0];
    DOCTEST_CHECK(relocation.icon == 2);
    DOCTEST_CHECK(relocation.x1 == 0);
    DOCTEST_CHECK(relocation.y1 == 0);
    DOCTEST_CHECK(relocation.x2 == 639);
    DOCTEST_CHECK(relocation.y2 == 479);

    const s_mouse_list& destination = MAINSCR::mouse_list_relocation[1];
    DOCTEST_CHECK(destination.icon == 18);
    DOCTEST_CHECK(destination.x1 == 22);
    DOCTEST_CHECK(destination.y1 == 22);
    DOCTEST_CHECK(destination.x2 == 526);
    DOCTEST_CHECK(destination.y2 == 420);
}

DOCTEST_TEST_CASE("main-screen RI registration reloads without consuming R") {
    registered_hotkeys.clear();
    reload_count = 0;
    full_reload_count = 0;
    reload_result = true;
    observed_network_game = false;
    user_box_message.clear();
    fields::fields_count = 100;
    MOX::game_type = 0;
    MOX::draw_main_screen_to_back = 0;

    MAINSCR::Add_Cheats();

    DOCTEST_CHECK(registered_hotkeys == std::vector<std::string>{
                                            "EINSTEIN",
                                            "MOOLA",
                                            "MENLO",
                                            "ISEEALL",
                                            "RE",
                                            "RI",
                                            "X",
                                            "ALE",
                                            "ALLAI",
                                            "SCOR",
                                        });
    DOCTEST_CHECK_FALSE(MAINSCR::Check_Config_Reload_Hot_Key(106));
    DOCTEST_CHECK(reload_count == 0);
    DOCTEST_CHECK(MAINSCR::Check_Config_Reload_Hot_Key(105));
    DOCTEST_CHECK(reload_count == 1);
    DOCTEST_CHECK(full_reload_count == 0);
    DOCTEST_CHECK_FALSE(observed_network_game);
    DOCTEST_CHECK(MOX::draw_main_screen_to_back == 1);

    MOX::draw_main_screen_to_back = 0;
    DOCTEST_CHECK(MAINSCR::Check_Config_Reload_Hot_Key(104));
    DOCTEST_CHECK(full_reload_count == 1);
    DOCTEST_CHECK(MOX::draw_main_screen_to_back == 1);
}

DOCTEST_TEST_CASE("main-screen RI reports reload failure once") {
    registered_hotkeys.clear();
    reload_count = 0;
    full_reload_count = 0;
    reload_result = false;
    user_box_message.clear();
    fields::fields_count = 200;
    MOX::game_type = 3;
    MOX::draw_main_screen_to_back = 0;
    MAINSCR::Add_Cheats();

    DOCTEST_CHECK(registered_hotkeys == std::vector<std::string>{
                                            "EINSTEIN",
                                            "MOOLA",
                                            "MENLO",
                                            "ISEEALL",
                                            "RE",
                                            "RI",
                                            "ALE",
                                            "ALLAI",
                                            "SCOR",
                                        });
    DOCTEST_CHECK(MAINSCR::Check_Config_Reload_Hot_Key(205));
    DOCTEST_CHECK(reload_count == 1);
    DOCTEST_CHECK(observed_network_game);
    DOCTEST_CHECK(user_box_message == "reload failed");
    DOCTEST_CHECK(MOX::draw_main_screen_to_back == 0);

    reload_result = true;
    user_box_message.clear();
    DOCTEST_CHECK(MAINSCR::Check_Config_Reload_Hot_Key(204));
    DOCTEST_CHECK(full_reload_count == 0);
    DOCTEST_CHECK(user_box_message ==
        "You may not reload config in a multiplayer game.");
}