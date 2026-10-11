#include "pch.h"

#include "config/config_runtime.h"

namespace spy {
    namespace {
        bool Validate_Count_Bonus_(const config::s_config_state* state) {
            config::t_variable_id plus_two_id = config::CONFIG_NO_VARIABLE;
            config::t_variable_id plus_one_id = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable("spy.count_bonus.plus_two",
                    strlen("spy.count_bonus.plus_two"),
                    &plus_two_id) ||
                !config::Find_Variable("spy.count_bonus.plus_one",
                    strlen("spy.count_bonus.plus_one"),
                    &plus_one_id)) {
                return true;
            }
            return plus_two_id < state->values.size() && plus_one_id < state->values.size() &&
                state->values[plus_two_id].integer_value < state->values[plus_one_id].integer_value;
        }

        const bool count_bonus_config_validator_registered = [] {
            config::Register_State_Validator(Validate_Count_Bonus_);
            return true;
        }();
    } // namespace
} // namespace spy