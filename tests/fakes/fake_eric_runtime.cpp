#include "pch.h"

namespace test_eric_runtime {
    s_player players[2];
    s_colony colonies[1];
    s_planet_data planets[1];
    s_ship_data ships[MAX_SHIPS];
    s_leader_data leaders[MAX_LEADERS];
    int16_t last_help_string;
    char help_string[1];
    int32_t fade_levels[101];
    int16_t fade_level_count;

    void Reset_() {
        memset(players, 0, sizeof(players));
        memset(colonies, 0, sizeof(colonies));
        memset(planets, 0, sizeof(planets));
        memset(ships, 0, sizeof(ships));
        memset(leaders, 0, sizeof(leaders));

        players[0].traits[TRAIT_TELEPATHIC] = 1;
        colonies[0].owner = 1;
        colonies[0].planet = 0;
        planets[0].star_index = 3;

        ships[0].owner = 0;
        ships[0].status = 0;
        ships[0].location = 3;
        ships[0].d.size = SHIP_SIZE_LARGE;

        leaders[0].player_index = 1;
        leaders[0].status = 1;
        leaders[0].eta = 0;
        leaders[0].type = LEADER_TYPE_COLONY;
        leaders[0].location = 3;

        last_help_string = -1;
    }

    void Reset_Fade_() {
        ERIC::fade_percent = 100;
        ERIC::fade_speed = {25, 25};
        fade_level_count = 0;
    }

    int16_t Fade_Level_Count_() {
        return fade_level_count;
    }

    int32_t Fade_Level_(int16_t index) {
        return fade_levels[index];
    }

    s_leader_data* Defender_Leader_() {
        return &leaders[0];
    }

    int16_t Last_Help_String_() {
        return last_help_string;
    }
}

namespace MOX {
    s_player* player = test_eric_runtime::players;
    s_colony* colony = test_eric_runtime::colonies;
    s_planet_data* planet = test_eric_runtime::planets;
    s_ship_data* ship = test_eric_runtime::ships;
    s_leader_data* leaders = test_eric_runtime::leaders;
}

namespace OFFICER {
    bool __cdecl Officer_Has_General_Skill(int16_t, int16_t leader_idx, int16_t skill_idx) {
        uint32_t skill_mask = 0;
        if (skill_idx == 0) {
            skill_mask = LEADER_GENERAL_SKILL_ASSASSIN;
        } else if (skill_idx == 48) {
            skill_mask = LEADER_GENERAL_SKILL_TELEPATH;
        } else if (skill_idx == 49) {
            skill_mask = LEADER_GENERAL_SKILL_TELEPATH2;
        }
        return (MOX::leaders[leader_idx].general_skills & skill_mask) != 0;
    }
}

namespace ESTRINGS {
    char* __cdecl E_Strings(int16_t string_id) {
        test_eric_runtime::last_help_string = string_id;
        return test_eric_runtime::help_string;
    }
}

namespace GENDRAW {
    void __cdecl Help(const char*) {
    }
}

namespace palstore {
    void __cdecl Darken_Palette(int32_t fade_level) {
        test_eric_runtime::fade_levels[test_eric_runtime::fade_level_count++] =
            fade_level;
    }
}

namespace video {
    void __cdecl Set_Refresh_Full() {
    }

    void __cdecl Submit_Palette() {
    }

    void __cdecl Set_Refresh_Stencil() {
    }
}

namespace RUSS {
    void __cdecl Mox_Update() {
    }
}