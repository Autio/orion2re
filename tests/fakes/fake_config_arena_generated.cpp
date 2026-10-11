#include "pch.h"

#include "config/config_runtime.h"

namespace TEST_CONFIG_ARENA {
    char* target;
    struct s_pair {
        uint8_t foo;
        uint8_t bar;
    };
    std::vector<s_pair> dynamic = {{3, 4}};
    uint32_t fail_default_leaf = UINT32_MAX;
} // namespace TEST_CONFIG_ARENA

namespace config {
    namespace {
        const s_type_descriptor string_type = {
            CONFIG_VALUE_STRING,
            false,
            false,
            0,
            0,
            CONFIG_SCALAR_PRIMITIVE,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            0,
            false,
            false,
            0,
            false,
            true,
            0,
            16,
            CONFIG_VALUE_INT16,
        };
        const s_type_descriptor uint8_type = {
            CONFIG_VALUE_UINT8,
            false,
            false,
            0,
            0,
            CONFIG_SCALAR_PRIMITIVE,
        };
    } // namespace

    const size_t variable_count = 2;
    const uint32_t saved_variables[] = {1};
    const s_config_snapshot_descriptor saved_config_snapshot = {saved_variables, 1, UINT32_C(1) << CONFIG_GROUP_GAMEPLAY};
    const s_config_snapshot_descriptor synchronized_config_snapshot = saved_config_snapshot;
    const s_schema_node_descriptor schema_nodes[] = {
        {
            "label",
            CONFIG_NODE_SCALAR,
            CONFIG_NO_NODE,
            0,
            0,
            0,
            0,
            false,
            0,
            CONFIG_NO_NODE,
            CONFIG_NO_NODE,
            &string_type,
            0,
            0,
            0,
        },
        {
            "dynamic",
            CONFIG_NODE_VECTOR,
            CONFIG_NO_NODE,
            0,
            1,
            1,
            1,
            false,
            0,
            2,
            CONFIG_NO_NODE,
            nullptr,
            0,
            1,
            2,
            false,
            false,
            0,
            3,
            0,
        },
        {
            "",
            CONFIG_NODE_RECORD,
            1,
            0,
            1,
            1,
            1,
            false,
            0,
            3,
            CONFIG_NO_NODE,
            nullptr,
            0,
            0,
            0,
            false,
            false,
            0,
            1,
            0,
        },
        {
            "foo",
            CONFIG_NODE_SCALAR,
            2,
            0,
            1,
            1,
            1,
            false,
            0,
            CONFIG_NO_NODE,
            4,
            &uint8_type,
            0,
            0,
            0,
            false,
            false,
            0,
            0,
            0,
        },
        {
            "bar",
            CONFIG_NODE_SCALAR,
            2,
            0,
            1,
            1,
            1,
            false,
            0,
            CONFIG_NO_NODE,
            CONFIG_NO_NODE,
            &uint8_type,
            0,
            0,
            0,
            false,
            false,
            1,
            1,
            1,
        },
    };
    const size_t schema_node_count = 5;
    const s_runtime_variable_descriptor runtime_variables[] = {
        {"label", 0, CONFIG_GROUP_INTERFACE, false, false, 0, 1, true},
        {"dynamic", 2, CONFIG_GROUP_GAMEPLAY, true, true, 1, 2, false},
    };
    const s_array_index_alias_descriptor* const array_index_aliases = nullptr;
    const size_t array_index_alias_count = 0;
    const s_config_group_descriptor config_groups[] = {
        {"interface", false, false},
        {"gameplay", true, true},
        {"technology", true, true},
    };

    t_config_group Get_Variable_Group(t_variable_id variable_id) {
        if (variable_id == 0) return CONFIG_GROUP_INTERFACE;
        return variable_id == 1 ? CONFIG_GROUP_GAMEPLAY : CONFIG_GROUP_COUNT;
    }

    const s_config_group_descriptor* Get_Config_Group(t_config_group group) {
        return group >= 0 && group < CONFIG_GROUP_COUNT ? &config_groups[group] : nullptr;
    }

    namespace generated {
        bool Get_Default_Value(t_variable_id variable_id, s_typed_value* value) {
            if (value == nullptr || variable_id >= variable_count) {
                return false;
            }
            *value = {};
            if (variable_id == 1) {
                value->type = CONFIG_VALUE_AGGREGATE;
                value->aggregate_values.resize(2);
                return Get_Default_Leaf_Value(1, 0, &value->aggregate_values[0]) &&
                    Get_Default_Leaf_Value(1, 1, &value->aggregate_values[1]);
            }
            value->type = CONFIG_VALUE_STRING;
            value->string_value = "default";
            return true;
        }

        bool Get_Default_Leaf_Value(t_variable_id variable_id, uint32_t leaf_index, s_typed_value* value) {
            if (value == nullptr || variable_id >= variable_count) return false;
            if (variable_id == 1 && leaf_index == TEST_CONFIG_ARENA::fail_default_leaf) return false;
            if (variable_id == 0) return leaf_index == 0 && Get_Default_Value(0, value);
            *value = {};
            value->type = CONFIG_VALUE_UINT8;
            value->integer_value = leaf_index == 0 ? 3 : leaf_index == 1 ? 4
                                                                         : 0;
            return true;
        }

        bool Resolve_Variable(t_variable_id variable_id, uint32_t leaf_index, s_resolved_variable* variable) {
            if (variable == nullptr || variable_id >= variable_count) {
                return false;
            }
            *variable = {};
            if (variable_id == 1) {
                const size_t element = leaf_index / 2;
                if (element >= TEST_CONFIG_ARENA::dynamic.size()) return false;
                variable->type = &uint8_type;
                variable->targets[0] = {
                    leaf_index % 2 == 0 ? static_cast<void*>(&TEST_CONFIG_ARENA::dynamic[element].foo) : static_cast<void*>(&TEST_CONFIG_ARENA::dynamic[element].bar),
                    sizeof(uint8_t),
                    s_variable_target::CONFIG_TARGET_VALUE,
                };
                variable->target_count = 1;
                return true;
            }
            if (leaf_index != 0) return false;
            variable->type = &string_type;
            variable->targets[0] = {
                &TEST_CONFIG_ARENA::target,
                sizeof(TEST_CONFIG_ARENA::target),
                s_variable_target::CONFIG_TARGET_NULLABLE_ARENA_STRING_POINTER,
            };
            variable->target_count = 1;
            return true;
        }

        bool Resize_Vector(t_variable_id variable_id, size_t element_count) {
            if (variable_id != 1) return false;
            TEST_CONFIG_ARENA::dynamic.resize(element_count);
            return true;
        }

        size_t Get_Vector_Size(t_variable_id variable_id) {
            return variable_id == 1 ? TEST_CONFIG_ARENA::dynamic.size() : 0;
        }
    } // namespace generated
} // namespace config