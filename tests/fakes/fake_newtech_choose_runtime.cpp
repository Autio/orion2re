#include "pch.h"

namespace {
s_player players[MAX_PLAYERS];
uint32_t selected_weight;
}

namespace TEST_NEWTECH_CHOOSE {
void Reset_() {
    memset(players, 0, sizeof(players));
    memset(MOX::tech_categories, 0, sizeof(MOX::tech_categories));
    memset(TECHDATA::technology_applications, 0,
           sizeof(TECHDATA::technology_applications));
    memset(TECHDATA::technology_fields, 0,
           sizeof(TECHDATA::technology_fields));
    selected_weight = 0;
    MOX::NUM_PLAYERS = 1;
    MOX::settings = {};
    MOX::stardate = 0;
}

s_player* Player_() {
    return &players[0];
}

uint32_t Selected_Weight_() {
    return selected_weight;
}
}

namespace MOX {
int8_t tech_categories[51][2];
s_player* player = players;
int16_t NUM_PLAYERS;
int32_t stardate;
s_settings settings;
}

namespace TECHDATA {
s_tech_app_data technology_applications[TECH_APP_COUNT];
s_tech_field_data technology_fields[TECH_FIELD_COUNT];
}

namespace COLCALC {
bool __cdecl Player_May_Get_Tech_App(s_player*, int) {
    return true;
}
}

namespace RUSS {
int16_t __cdecl Get_Weighted_Choice_Int(uint32_t* weights, int16_t count) {
    selected_weight = count > 1 ? weights[1] : 0;
    return 1;
}
}

namespace game_random {
uint32_t __cdecl Random(uint32_t) {
    return 1;
}
}