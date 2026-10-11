#include "pch.h"

#include "game/boarding.h"

namespace BOARDING {
    int16_t __cdecl Captured_Marine_Count(
        int16_t target_idx, int16_t surviving_marines) {
        if (kill_excess_marines_after_capture == 0) {
            return surviving_marines;
        }

        s_combat_data* target = &MOX::combat_data[target_idx];
        int16_t hull_class = target->size_class;
        if (COMBINIT::Is_Combat_Satellite(target)) {
            hull_class = COMBINIT::Combat_Satellite_Hull_Class(target);
        }

        int32_t capacity = TECHDATA::hull_data[hull_class].marines;
        if (struct_::Test_Bit_Field(
                target->special_device_flags, SPECIAL_TROOP_PODS) &&
            !struct_::Test_Bit_Field(
                target->special_device_damage_flags, SPECIAL_TROOP_PODS)) {
            capacity *= 2;
        }

        return surviving_marines > capacity
            ? (int16_t)capacity
            : surviving_marines;
    }

    e_boarding_filter __cdecl Filter_Boarding(
        int16_t attacker_idx, int16_t target_idx) {
        const s_combat_data& attacker = MOX::combat_data[attacker_idx];
        const s_combat_data& target = MOX::combat_data[target_idx];

        if (target.stasis_source_idx != COMBAT_SHIP_NONE) {
            return e_boarding_filter::target_in_stasis;
        }

        const int16_t range =
            CMBTAI::Range_To_Ship_Squares(attacker_idx, target_idx);
        const bool normal_ok =
            range <= board_range.normal &&
            COMBAT1::Calc_Current_Speed(target_idx) == 0;
        const bool transporters_ok =
            range <= board_range.transporters &&
            COMBINIT::Does_Combat_Ship_Have_Special(
                attacker_idx, SPECIAL_TRANSPORTERS) != 0 &&
            COMBINIT::Does_Combat_Ship_Have_Special(
                target_idx, SPECIAL_HARD_SHIELDS) == 0 &&
            target.shield_arc_current[CMBTFIRE::Facing_Shield(
                CMBTDRW1::Relative_Bearing(target_idx, attacker_idx))] == 0;

        if (!normal_ok && transporters_ok &&
            boarding.damper_survival_divisor == 0 &&
            COMBINIT::Does_Combat_Ship_Have_Special(
                target_idx, SPECIAL_DAMPER_FIELD) != 0) {
            return e_boarding_filter::transporter_blocked_by_damper;
        }
        if (!normal_ok && !transporters_ok) {
            return e_boarding_filter::allowed;
        }
        if (board_in_black_hole == 0) {
            if (target.black_hole_source_idx != COMBAT_SHIP_NONE) {
                return e_boarding_filter::target_in_black_hole;
            }
            if (attacker.black_hole_source_idx != COMBAT_SHIP_NONE) {
                return e_boarding_filter::attacker_in_black_hole;
            }
        }
        if (target.special_status_flag == 4) {
            return e_boarding_filter::target_phased_out;
        }
        return e_boarding_filter::allowed;
    }
} // namespace BOARDING
