#include "pch.h"

#include "config/config_runtime.h"

namespace INITSHIP {
    namespace {
        bool State_Integer_(const config::s_config_state* state,
                            const char* path, int64_t* value) {
            config::t_variable_id variable = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable(path, strlen(path), &variable) ||
                variable >= state->values.size()) {
                return false;
            }
            *value = state->values[variable].integer_value;
            return true;
        }

        bool Validate_FTL_Speeds_(const config::s_config_state* state) {
            int64_t td_bonus = 0;
            if (!State_Integer_(state, "movement.td_ftl_bonus", &td_bonus)) {
                return false;
            }

            char path[64];
            for (int drive = DRIVE_NO_DRIVE; drive < DRIVE_COUNT; ++drive) {
                int64_t warp_speed = 0;
                snprintf(path, sizeof(path), "design.drive.%d.warp_speed", drive);
                if (!State_Integer_(state, path, &warp_speed) ||
                    warp_speed + td_bonus > UINT8_MAX) {
                    return false;
                }
            }
            return true;
        }

        const bool ftl_speed_validator_registered = [] {
            config::Register_State_Validator(Validate_FTL_Speeds_);
            return true;
        }();
    } // namespace
} // namespace INITSHIP