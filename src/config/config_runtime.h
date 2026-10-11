#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "config/generated/config_groups.h"
#include "config/config_values.h"
#include "config/config_bitmap.h"

namespace arena {
    struct s_arena;
}

namespace config {

    enum e_value_type {
        CONFIG_VALUE_BOOL,
        CONFIG_VALUE_INT8,
        CONFIG_VALUE_UINT8,
        CONFIG_VALUE_INT16,
        CONFIG_VALUE_UINT16,
        CONFIG_VALUE_INT32,
        CONFIG_VALUE_UINT32,
        CONFIG_VALUE_STRING,
        CONFIG_VALUE_BITMAP,
        CONFIG_VALUE_RATIO,
        CONFIG_VALUE_AGGREGATE,
    };

    struct s_config_group_descriptor {
        const char* id;
        bool save;
        bool synchronize;
    };

    enum e_scalar_kind {
        CONFIG_SCALAR_PRIMITIVE,
        CONFIG_SCALAR_ENUM,
        CONFIG_SCALAR_MASK,
        CONFIG_SCALAR_BITMAP,
        CONFIG_SCALAR_RATIO,
    };

    enum e_schema_node_kind {
        CONFIG_NODE_SCALAR,
        CONFIG_NODE_RECORD,
        CONFIG_NODE_ARRAY,
        CONFIG_NODE_MAP,
        CONFIG_NODE_VECTOR,
        CONFIG_NODE_NAMESPACE,
    };

    using t_variable_id = uint32_t;

    struct s_typed_value {
        e_value_type type = CONFIG_VALUE_BOOL;
        bool bool_value = false;
        int64_t integer_value = 0;
        std::string string_value;
        s_config_bitmap bitmap_value;
        int64_t ratio_numerator = 0;
        int64_t ratio_denominator = 1;
        s_config_values<s_typed_value> aggregate_values;
    };

    struct s_symbol_descriptor {
        const char* name;
        int64_t value;
    };

    struct s_bitmap_symbol_descriptor {
        const char* name;
        const uint8_t* value;
    };

    struct s_type_descriptor {
        e_value_type value_type;
        bool has_min;
        bool has_max;
        int64_t min;
        int64_t max;
        e_scalar_kind scalar_kind;
        const s_symbol_descriptor* values;
        size_t value_count;
        const s_symbol_descriptor* flags;
        size_t flag_count;
        const s_bitmap_symbol_descriptor* bitmap_values;
        size_t bitmap_value_count;
        const s_bitmap_symbol_descriptor* bitmap_flags;
        size_t bitmap_flag_count;
        const uint8_t* bitmap_known_bits;
        size_t bitmap_bit_count;
        bool allow_numeric;
        bool reject_unknown_numeric;
        uint64_t known_bits;
        bool has_min_length;
        bool has_max_length;
        size_t min_length;
        size_t max_length;
        e_value_type ratio_component_type;
    };

    struct s_ratio_value {
        int64_t numerator;
        int64_t denominator;
    };

    struct s_variable_target {
        void* address;
        size_t size;
        enum e_storage {
            CONFIG_TARGET_VALUE,
            CONFIG_TARGET_STRING_BUFFER,
            CONFIG_TARGET_ARENA_STRING_POINTER,
            CONFIG_TARGET_NULLABLE_ARENA_STRING_POINTER,
        } storage;
    };

    struct s_variable_descriptor {
        const char* name;
        const s_type_descriptor* type;
    };

    constexpr size_t CONFIG_MAX_VARIABLE_TARGETS = 2;

    struct s_resolved_variable {
        const s_type_descriptor* type;
        s_variable_target targets[CONFIG_MAX_VARIABLE_TARGETS];
        size_t target_count;
    };

    struct s_schema_node_descriptor {
        const char* id;
        e_schema_node_kind kind;
        uint32_t parent;
        int32_t external_index;
        t_variable_id first_variable;
        t_variable_id last_variable;
        t_variable_id variable_id;
        bool has_terminator;
        int64_t terminator;
        uint32_t first_child;
        uint32_t next_sibling;
        const s_type_descriptor* type;
        int32_t array_index_base;
        uint32_t array_element_count;
        uint32_t array_element_variable_count;
        bool implicit_terminator;
        bool unique_values;
        uint32_t first_leaf;
        uint32_t last_leaf;
        uint32_t leaf_index;
    };

    struct s_variable_location {
        const s_schema_node_descriptor* node;
        t_variable_id variable_offset;
    };

    struct s_array_index_alias_descriptor {
        uint32_t array_node;
        const char* name;
        int32_t external_index;
    };

    struct s_runtime_variable_descriptor;

    struct s_runtime_schema {
        const s_schema_node_descriptor* nodes;
        size_t node_count;
        size_t variable_count;
        const s_runtime_variable_descriptor* variables;
        const s_array_index_alias_descriptor* array_index_aliases;
        size_t array_index_alias_count;
    };

    struct s_runtime_variable_descriptor {
        const char* name;
        uint32_t node;
        t_config_group group;
        bool aggregate;
        bool vector;
        uint32_t default_element_count;
        uint32_t element_leaf_count;
        bool has_arena_strings;
        bool native_defaults = false;
    };

    struct s_variable_accessor {
        t_variable_id variable_id = UINT32_MAX;
        uint32_t leaf_index = 0;
    };

    struct s_non_default_aggregate {
        t_variable_id variable_id = UINT32_MAX;
        uint32_t leaf_count = 0;
        std::vector<uint64_t> leaves;
    };

    struct s_config_state {
        s_config_values<s_typed_value> values;
        std::vector<uint64_t> non_default_values;
        std::vector<t_variable_id> non_default_variable_ids;
        s_config_values<uint32_t> non_default_variable_positions;
        s_config_values<uint32_t> non_default_aggregate_positions;
        std::vector<s_non_default_aggregate> non_default_aggregates;
        bool tracks_non_default_values = false;
        mutable bool validated = false;
    };

    using t_state_validator = bool (*)(const s_config_state* state);
    using t_commit_listener = void (*)();

    struct s_config_snapshot_descriptor {
        const uint32_t* variables;
        uint32_t variable_count;
        uint32_t group_mask;
    };

    struct s_config_write;
    struct s_config_write_set;
    struct s_cfg_diagnostic;

    extern const size_t variable_count;
    extern const s_schema_node_descriptor schema_nodes[];
    extern const size_t schema_node_count;
    extern const s_runtime_variable_descriptor runtime_variables[];
    extern const s_array_index_alias_descriptor* const array_index_aliases;
    extern const size_t array_index_alias_count;
    extern const s_config_group_descriptor config_groups[];
    extern const s_config_snapshot_descriptor synchronized_config_snapshot;
    extern const s_config_snapshot_descriptor saved_config_snapshot;

    constexpr uint32_t CONFIG_NO_NODE = UINT32_MAX;
    constexpr t_variable_id CONFIG_NO_VARIABLE = UINT32_MAX;

    const s_runtime_schema* Get_Runtime_Schema();
    bool Get_Variable_Location(const s_runtime_schema* schema, t_variable_id variable_id, s_variable_location* location);
    bool Get_Array_Element_Offset(const s_runtime_schema* schema, t_variable_id variable_id, uint32_t leaf_index, uint32_t array_node, uint32_t* element_offset);
    const s_schema_node_descriptor* Get_Variable_Leaf_Node(const s_runtime_schema* schema,
        t_variable_id variable_id,
        uint32_t leaf_index);
    const s_schema_node_descriptor* Get_Variable_Node(const s_runtime_schema* schema,
        t_variable_id variable_id);
    const s_type_descriptor* Get_Variable_Type(t_variable_id variable_id);
    const s_type_descriptor* Get_Variable_Leaf_Type(t_variable_id variable_id,
        uint32_t leaf_index);
    t_config_group Get_Variable_Group(t_variable_id variable_id);
    const s_config_group_descriptor* Get_Config_Group(t_config_group group);
    bool Find_Variable(const char* name, size_t name_length, t_variable_id* variable_id);
    bool Find_Variable_Accessor(const char* name, size_t name_length, s_variable_accessor* accessor);
    bool Get_Variable_Name(const s_runtime_schema* schema, t_variable_id variable_id, std::string* name);
    bool Get_Variable_Name(t_variable_id variable_id, std::string* name);
    bool Get_Variable_Leaf_Name(const s_runtime_schema* schema,
        t_variable_id variable_id,
        uint32_t leaf_index,
        std::string* name);
    bool Get_Variable_Leaf_Name(t_variable_id variable_id, uint32_t leaf_index, std::string* name);
    bool Resolve_Write_Path(s_config_write* write, s_cfg_diagnostic* diagnostic);

    bool Initialize_Defaults(s_config_state* state);
    bool Initialize_Tracked_Defaults(s_config_state* state);
    const s_config_state* Get_Default_State();
    bool Snapshot_Contains_Variable(const s_config_snapshot_descriptor* descriptor, t_variable_id id);
    bool Read_Current_State(s_config_state* state);
    bool Track_State_Non_Default_Values(s_config_state* state);
    bool Read_State_Value(const s_config_state* state, t_variable_id variable_id, s_typed_value* value);
    bool Read_State_Leaf_Value(const s_config_state* state, t_variable_id variable_id, uint32_t leaf_index, s_typed_value* value);
    bool Set_State_Value(s_config_state* state,
        t_variable_id variable_id,
        const s_typed_value& value);
    bool Set_State_Leaf_Value(s_config_state* state, t_variable_id variable_id, uint32_t leaf_index, const s_typed_value& value);
    bool Copy_State_Value(s_config_state* target,
        const s_config_state* source,
        t_variable_id variable_id);
    bool Overlay_Config_State(const s_config_snapshot_descriptor* descriptor,
        const s_config_state* source,
        s_config_state* target);
    bool State_Tracks_Non_Default_Values(const s_config_state* state);
    bool State_Value_Is_Non_Default(const s_config_state* state,
        t_variable_id variable_id);
    bool State_Leaf_Is_Non_Default(const s_config_state* state,
        t_variable_id variable_id,
        uint32_t leaf_index);
    const s_non_default_aggregate* Get_State_Non_Default_Aggregate(
        const s_config_state* state,
        t_variable_id variable_id);
    bool Config_Values_Equal(const s_typed_value& left,
        const s_typed_value& right);
    bool Apply_Write_Set(const s_config_write_set* write_set, s_config_state* state, s_cfg_diagnostic* diagnostic);
    void Register_State_Validator(t_state_validator validator);
    void Register_Commit_Listener(t_commit_listener listener);
    bool Validate_State(const s_config_state* state);
    bool Commit_Config_Transition(uint32_t group_mask,
        const s_config_state* prior,
        const s_config_state* current);
    bool Commit_Config_Groups(uint32_t group_mask, const s_config_state* state);
    bool Commit_State(const s_config_state* state);
    bool Commit_Initial_State(const s_config_state* state);
    bool Commit_Config_Group(t_config_group group, const s_config_state* state);
    arena::s_arena* Acquire_String_Arena(t_config_group group);
    bool Is_Active_String(const char* value);
    void Shutdown_Runtime();

    namespace generated {

        bool Get_Default_Value(t_variable_id variable_id, s_typed_value* value);
        bool Get_Default_Leaf_Value(t_variable_id variable_id, uint32_t leaf_index, s_typed_value* value);
        bool Resolve_Variable(t_variable_id variable_id, uint32_t leaf_index, s_resolved_variable* variable);
        void Set_Value(t_variable_id variable_id, const s_typed_value* value, arena::s_arena* string_arena);
        bool Set_Leaf_Value(t_variable_id variable_id,
            uint32_t leaf_index,
            const s_typed_value* value,
            arena::s_arena* string_arena);
        bool Get_Current_Value(t_variable_id variable_id, s_typed_value* value);
        bool Get_Current_Leaf_Value(t_variable_id variable_id, uint32_t leaf_index, s_typed_value* value);
        bool Resize_Vector(t_variable_id variable_id, size_t element_count);
        size_t Get_Vector_Size(t_variable_id variable_id);

    } // namespace generated

} // namespace config