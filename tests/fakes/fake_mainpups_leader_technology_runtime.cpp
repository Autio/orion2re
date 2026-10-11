#include "pch.h"

namespace {
    s_leader_data leaders[MAX_LEADERS];
    s_player players[MAX_PLAYERS];
    char federation[] = "Federation";
    char empty[] = "";
    char string_format[] = "%s";
}

namespace TEST_MAINPUPS_LEADER_TECHNOLOGY {
    void Reset_() {
        memset(leaders, 0, sizeof(leaders));
        memset(players, 0, sizeof(players));
        memset(MOX::government_labels, 0, sizeof(MOX::government_labels));
        memset(TECHDATA::technology_applications, 0,
               sizeof(TECHDATA::technology_applications));
        MOX::player[0].traits[TRAIT_CURRENT_GOVERNMENT] = GOVERNMENT_DEMOCRACY;
        MOX::government_labels[GOVERNMENT_FEDERATION] = federation;
    }
}

namespace MOX {
    s_leader_data* leaders = ::leaders;
    s_player* player = players;
    char* government_labels[GOVERNMENT_COUNT];
}

namespace TECHDATA {
    s_tech_app_data technology_applications[TECH_APP_COUNT];
}

namespace ESTRINGS {
    char* __cdecl E_Strings(int16_t string_id) {
        if (string_id == 207) {
            return string_format;
        }
        return empty;
    }
}