#include "pch.h"

namespace {
    s_player players[MAX_PLAYERS];
}

namespace TEST_DIPLOMAC {
    void Reset_() {
        memset(players, 0, sizeof(players));
        MOX::NUM_PLAYERS = 2;
    }
}

namespace MOX {
    s_player* player = players;
    int16_t NUM_PLAYERS = 2;
}

namespace diplodef {
    int16_t next_council_meeting_turn;
    int16_t COUNCIL_FLAG;
    int16_t council_winner;
    int16_t personality_relation_modifiers[MAX_PLAYERS];
}

namespace game_random {
    uint32_t __cdecl Random(uint32_t) {
        return 1;
    }
}

namespace AIMOVE {
    int __cdecl Number_Of_Attacks_Underway(int16_t, int16_t) {
        return 0;
    }
}

namespace COLCALC {
    int __cdecl Best_Leader_Diplomat_Bonus(int) {
        return 0;
    }

    int16_t __cdecl Get_Surrender_System(int16_t, int16_t, int16_t) {
        return -1;
    }

    bool __cdecl May_Surrender_Star(int16_t, int16_t, int16_t) {
        return false;
    }
}

namespace TECHDATA {
    s_tech_app_data technology_applications[TECH_APP_COUNT];
    s_tech_field_data technology_fields[TECH_FIELD_COUNT];
}

namespace npcdiplo {
    void __cdecl Clear_Last_Proposal_Turn(int16_t, int16_t) {
    }

    void __cdecl Get_Differential_Tech_List(
        int16_t, int16_t, uint32_t, uint32_t, uint8_t*, int32_t*,
        int16_t* tech_count, int16_t) {
        *tech_count = 0;
    }
}