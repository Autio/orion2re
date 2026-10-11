#include "pch.h"

#include "config/config_runtime.h"

namespace aidesign {
    namespace {

        bool State_Integer_(const config::s_config_state* state, const char* path, int64_t* value) {
            config::t_variable_id variable_id = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable(path, strlen(path), &variable_id) ||
                variable_id >= state->values.size()) {
                return false;
            }
            const config::s_typed_value& state_value = state->values[variable_id];
            *value = state_value.type == config::CONFIG_VALUE_BOOL ? (state_value.bool_value ? 1 : 0) : state_value.integer_value;
            return true;
        }

        bool Validate_Priority_List_(const config::s_config_state* state, const char* path, int count) {
            char item_path[96];
            char kind_path[96];
            for (int index = 1; index <= count; ++index) {
                snprintf(item_path, sizeof(item_path), "ai.ship_design.%s.%d.item", path, index);
                snprintf(kind_path, sizeof(kind_path), "ai.ship_design.%s.%d.kind", path, index);
                int64_t item = 0;
                int64_t kind = 0;
                if (!State_Integer_(state, item_path, &item) ||
                    !State_Integer_(state, kind_path, &kind)) {
                    return false;
                }
                if ((kind == AI_SPECIAL_ITEM_SPECIAL && item >= SPECIAL_COUNT) ||
                    (kind == AI_SPECIAL_ITEM_WEAPON && item >= WEAPON_COUNT)) {
                    return false;
                }
            }
            return true;
        }

        bool Validate_Ship_Design_Config_(const config::s_config_state* state) {
            config::t_variable_id variable_id = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable("ai.ship_design.special_priority.c1_bio_weapon.1.item",
                    strlen("ai.ship_design.special_priority.c1_bio_weapon.1.item"),
                    &variable_id)) {
                return true;
            }
            return Validate_Priority_List_(state, "special_priority.c1_bio_weapon", 5) &&
                Validate_Priority_List_(state, "special_priority.c1_capture", 5) &&
                Validate_Priority_List_(state, "special_priority.c1_cloaking", 3) &&
                Validate_Priority_List_(state, "special_priority.c1_beam_defense", 4) &&
                Validate_Priority_List_(state, "special_priority.c1_missile_defense", 8) &&
                Validate_Priority_List_(state, "special_priority.c1_armor", 3) &&
                Validate_Priority_List_(state, "special_priority.c1_shield", 3) &&
                Validate_Priority_List_(state, "special_priority.c2_beam_offense", 7) &&
                Validate_Priority_List_(state, "special_priority.c3_missile_offense", 2) &&
                Validate_Priority_List_(state, "special_priority.c5_special_weapon", 5) &&
                Validate_Priority_List_(state, "c6_special_device", 12) &&
                Validate_Priority_List_(state, "defense_priority.c4_defense_human_non_beams", 19) &&
                Validate_Priority_List_(state, "defense_priority.c4_defense_human_beams", 19);
        }

        const bool ship_design_config_validator_registered = [] {
            config::Register_State_Validator(Validate_Ship_Design_Config_);
            return true;
        }();

    } // namespace
} // namespace aidesign