#include "pch.h"

#include "config/config_runtime.h"

namespace ERICNET {
    namespace {
        bool Validate_Crew_Experience_(const config::s_config_state* state) {
            char elite_path[48];
            char ultra_elite_path[48];
            snprintf(elite_path, sizeof(elite_path), "combat.crew.%d.experience", CREW_QUALITY_ELITE);
            snprintf(ultra_elite_path, sizeof(ultra_elite_path), "combat.crew.%d.experience", CREW_QUALITY_ULTRA_ELITE);

            config::t_variable_id elite_id = config::CONFIG_NO_VARIABLE;
            config::t_variable_id ultra_elite_id = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable(
                    elite_path, strlen(elite_path), &elite_id)) {
                return true;
            }
            if (!config::Find_Variable(
                    ultra_elite_path, strlen(ultra_elite_path), &ultra_elite_id) ||
                elite_id >= state->values.size() ||
                ultra_elite_id >= state->values.size()) {
                return false;
            }
            return state->values[ultra_elite_id].integer_value >
                state->values[elite_id].integer_value;
        }

        const bool crew_experience_validator_registered = [] {
            config::Register_State_Validator(Validate_Crew_Experience_);
            return true;
        }();
    } // namespace
} // namespace ERICNET