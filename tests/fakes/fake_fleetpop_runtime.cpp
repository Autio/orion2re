#include "pch.h"

namespace {
    s_ship_data ships[3];
    s_star_data stars[2];
    s_planet_data planets[1];
    s_player players[2];
    s_colony colonies[1];
}

namespace TEST_FLEETPOP {
    void Reset_() {
        memset(ships, 0, sizeof(ships));
        memset(stars, 0xff, sizeof(stars));
        memset(planets, 0, sizeof(planets));
        memset(players, 0, sizeof(players));
        memset(colonies, 0, sizeof(colonies));
        memset(MOX::ship_node, 0xff, sizeof(MOX::ship_node));
        memset(MOX::ship_stack_start, 0xff, sizeof(MOX::ship_stack_start));

        MOX::PLAYER_NUM = 0;
        MOX::NUM_STARS = 2;
        MOX::NUM_SHIPS = 2;
        MOX::NUM_COLONIES = 0;
        MOX::fleet_box_ship_stack = 0;
        MOX::small_ship_stack_ptr = 0;
        MOX::ship_stack_count = 1;
        MOX::ship_stack_start[0] = 0;
        MOX::ship_node[0].ship_idx = 0;
        MOX::ship_node[0].next_node = -1;
        MOX::ship[0].owner = 0;
        MOX::ship[0].d.ship_type = SHIP_TYPE_OUTPOST;
        MOX::star[1].colonize_player = 0;
        MOX::star[1].planet_index[0] = 0;
        MOX::planet[0].colony_index = -1;
    }
}

namespace MOX {
    int16_t PLAYER_NUM;
    int16_t NUM_STARS;
    int16_t NUM_SHIPS;
    int16_t NUM_COLONIES;
    int16_t small_ship_stack_ptr;
    int16_t scanned_small_ship;
    int16_t galaxy_map_scanned_star;
    int16_t ship_stack_count;
    int16_t ship_icon_count;
    s_ship_icon ship_icon[SHIP_ICON_COUNT];
    s_ship_data* ship = ships;
    s_star_data* star = stars;
    s_planet_data* planet = planets;
    s_player* player = players;
    s_colony* colony = colonies;
    s_ship_node ship_node[MAX_SHIPS];
    int16_t ship_stack_start[MAX_SHIPS];
    int16_t fleet_box_ship_stack;
}

namespace HACCESS {
    int16_t __cdecl Ship_Owner(int16_t ship_idx) {
        return MOX::ship[ship_idx].owner;
    }

    int16_t __cdecl Ship_Stack_Owner(int16_t node_idx) {
        return MOX::ship[MOX::ship_node[node_idx].ship_idx].owner;
    }
}

namespace FLT2 {
    int16_t __cdecl Fltscrn_Ships_To_Be_Displayed(int16_t) {
        return 1;
    }
}

namespace HAROLD {
    int8_t __cdecl Planet_Has_Outpost(int16_t) {
        return 0;
    }

    uint8_t __cdecl Player_Has_Visited(int16_t, int16_t) {
        return 0;
    }
}

namespace special {
    int16_t __cdecl Range(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
        const int16_t x_distance = (int16_t)abs((int)x2 - (int)x1);
        const int16_t y_distance = (int16_t)abs((int)y2 - (int)y1);
        const int16_t long_distance = x_distance > y_distance ? x_distance : y_distance;
        const int16_t short_distance = x_distance > y_distance ? y_distance : x_distance;
        return (int16_t)(long_distance + short_distance / 2);
    }
}

namespace struct_ {
    int16_t __cdecl Test_Bit_Field(void* bit_field, uint16_t bit_index) {
        const uint8_t* bytes = (const uint8_t*)bit_field;
        return (int16_t)((bytes[bit_index / 8] >> (bit_index % 8)) & 1);
    }

    void __cdecl Set_Bit_Field(void* bit_field, uint16_t bit_index) {
        uint8_t* bytes = (uint8_t*)bit_field;
        bytes[bit_index / 8] |= (uint8_t)(1U << (bit_index % 8));
    }
}

namespace strings {
    void __cdecl Delete_Structure(int32_t index, void* array,
                                   uint32_t element_size_bytes,
                                   uint32_t element_count) {
        uint8_t* bytes = (uint8_t*)array;
        memmove(bytes + index * element_size_bytes,
                bytes + (index + 1) * element_size_bytes,
                (element_count - (uint32_t)index - 1) * element_size_bytes);
    }
}