#include "pch.h"
#include "game/combat1.h"

namespace COMBAT1 {
    uint8_t interactive_combat_stats = 0;
    uint8_t td_combat_speed_bonus = 4;
    uint8_t continuous_bhg = 0;
    uint8_t non_ics_tractor_and_bhg_mode = 0;
    uint8_t phasing_cloak_decloak_defense = 0;
    uint8_t tractors_to_stop[SHIP_SIZE_COUNT] = {1, 2, 3, 4, 5, 6};
    uint8_t recloak_next_turn = 0;
    uint8_t rephase_cloak_next_turn = 0;
    int8_t ship_initiative = -1;
    uint16_t wait_movement_cost = 0;
    uint8_t max_combat_turns = 50;
    uint8_t absorber_keep_charge = 0;
    uint8_t disable_scan = 0;
    bool center_on_next_ship = true;
} // namespace COMBAT1