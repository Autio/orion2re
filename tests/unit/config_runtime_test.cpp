#include "pch.h"

#include <cstring>

#include "config/config_decode.h"
#include "config/config_lifecycle.h"
#include "config/config_runtime.h"
#include "doctest/doctest.h"
#include "game/arena.h"

namespace TEST_CONFIG_ARENA {
    extern char* target;
    struct s_pair {
        uint8_t foo;
        uint8_t bar;
    };
    extern std::vector<s_pair> dynamic;
    extern uint32_t fail_default_leaf;
} // namespace TEST_CONFIG_ARENA

DOCTEST_TEST_SUITE("configuration string arena") {
    DOCTEST_TEST_CASE("incremental validation still rejects direct edits and malformed shapes") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&state));
        DOCTEST_REQUIRE(config::Validate_State(&state));
        DOCTEST_CHECK(state.values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(state.non_default_variable_positions.Stored_Value_Count() == 0);
        auto invalid = state;
        invalid.values[1].aggregate_values[0].integer_value = 256;
        DOCTEST_CHECK_FALSE(config::Validate_State(&invalid));
        invalid = state;
        invalid.values[1].aggregate_values.resize(3);
        DOCTEST_CHECK_FALSE(config::Validate_State(&invalid));
        invalid = state;
        invalid.values[1].aggregate_values.resize(4);
        DOCTEST_CHECK_FALSE(config::Validate_State(&invalid));
        invalid = state;
        invalid.values[0].string_value = "a string beyond the configured maximum";
        DOCTEST_CHECK_FALSE(config::Commit_Config_Transition(UINT32_MAX, &state, &invalid));
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("default states share immutable values and store only changed leaves") {
        config::Shutdown_Runtime();
        TEST_CONFIG_ARENA::dynamic = {{3, 4}};
        config::s_config_state launch;
        config::s_config_state saved;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&launch));
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&saved));
        DOCTEST_CHECK(launch.values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(saved.values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(&launch.values.Read(1) == &saved.values.Read(1));

        config::s_typed_value value = launch.values.Read(1).aggregate_values[0];
        value.integer_value = 10;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&launch, 1, 0, value));
        value.integer_value = 20;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&saved, 1, 0, value));
        DOCTEST_CHECK(launch.values.Stored_Value_Count() == 1);
        DOCTEST_CHECK(launch.values.Read(1).aggregate_values.Stored_Value_Count() == 1);
        DOCTEST_CHECK(&launch.values.Read(1).aggregate_values[1] ==
            &saved.values.Read(1).aggregate_values[1]);
        config::s_config_state copy = launch;
        value.integer_value = 30;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&copy, 1, 0, value));
        DOCTEST_CHECK(launch.values.Read(1).aggregate_values[0].integer_value == 10);

        DOCTEST_REQUIRE(config::Commit_State(&launch));
        const uint32_t gameplay = UINT32_C(1) << config::CONFIG_GROUP_GAMEPLAY;
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &launch, &saved));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 20);
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &saved, &launch));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 10);

        config::s_config_state defaults;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&defaults));
        DOCTEST_CHECK(defaults.values.Read(1).aggregate_values[0].integer_value == 3);
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&copy, 1, 0,
            defaults.values.Read(1).aggregate_values[0]));
        DOCTEST_CHECK(copy.values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(copy.non_default_variable_ids.empty());
        config::Shutdown_Runtime();
        DOCTEST_CHECK(defaults.values.Read(1).aggregate_values[0].integer_value == 3);
    }

    DOCTEST_TEST_CASE("failed growth does not retain an edit of a default aggregate") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&state));
        config::s_typed_value value = state.values.Read(1).aggregate_values[0];
        TEST_CONFIG_ARENA::fail_default_leaf = 9;
        const bool applied = config::Set_State_Leaf_Value(&state, 1, 9, value);
        TEST_CONFIG_ARENA::fail_default_leaf = UINT32_MAX;
        DOCTEST_CHECK_FALSE(applied);
        DOCTEST_CHECK(state.values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(state.non_default_variable_ids.empty());
        DOCTEST_CHECK(state.non_default_aggregates.empty());
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("dense vector growth accepts an aliased input leaf") {
        config::Shutdown_Runtime();
        TEST_CONFIG_ARENA::target = nullptr;
        TEST_CONFIG_ARENA::dynamic = {{3, 4}};
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Read_Current_State(&state));
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&state, 1, 7,
            state.values.Read(1).aggregate_values[0]));
        DOCTEST_CHECK(state.values.Read(1).aggregate_values[7].integer_value == 3);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("baseline storage copies only a thousand edits in a large aggregate") {
        config::s_config_values<config::s_typed_value> values;
        values.resize(100000);
        for (size_t index = 0; index < values.size(); ++index) {
            values[index].type = config::CONFIG_VALUE_UINT32;
            values[index].integer_value = static_cast<int64_t>(index);
        }
        values.Freeze();
        auto overlay = values;
        for (size_t index = 0; index < 1000; ++index) {
            overlay[index * 97].integer_value = -1;
        }
        auto copy = overlay;
        DOCTEST_CHECK(values.Stored_Value_Count() == 0);
        DOCTEST_CHECK(overlay.Stored_Value_Count() == 1000);
        DOCTEST_CHECK(copy.Stored_Value_Count() == 1000);
        DOCTEST_CHECK(&copy.Read(99999) == &values.Read(99999));
        copy[0].integer_value = 42;
        DOCTEST_CHECK(overlay.Read(0).integer_value == -1);
        for (size_t index = 0; index < 1000; ++index) copy.Reset(index * 97);
        DOCTEST_CHECK(copy.Stored_Value_Count() == 0);
        DOCTEST_CHECK(copy.Read(97).integer_value == 97);

        copy.resize(10);
        copy.resize(100000);
        DOCTEST_CHECK(copy.Read(9).integer_value == 9);
        DOCTEST_CHECK(copy.Read(97).type == config::CONFIG_VALUE_BOOL);
        DOCTEST_CHECK(values.Read(97).integer_value == 97);
    }

    DOCTEST_TEST_CASE("defaults and overrides are freshly allocated on every commit") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Defaults(&state));
        DOCTEST_REQUIRE(config::Commit_State(&state));
        char* first = TEST_CONFIG_ARENA::target;
        DOCTEST_CHECK(std::strcmp(first, "default") == 0);
        DOCTEST_CHECK(config::Is_Active_String(first));
        arena::s_arena* first_arena =
            config::Acquire_String_Arena(config::CONFIG_GROUP_INTERFACE);
        DOCTEST_REQUIRE(first_arena != nullptr);

        state.values[0].string_value = "override";
        DOCTEST_REQUIRE(config::Commit_State(&state));
        char* second = TEST_CONFIG_ARENA::target;
        DOCTEST_CHECK(std::strcmp(second, "override") == 0);
        DOCTEST_CHECK(second != first);
        DOCTEST_CHECK(std::strcmp(first, "default") == 0);
        DOCTEST_CHECK(config::Is_Active_String(second));
        DOCTEST_CHECK_FALSE(config::Is_Active_String(first));
        DOCTEST_CHECK_FALSE(config::Is_Active_String(nullptr));
        const char unrelated[] = "unrelated";
        DOCTEST_CHECK_FALSE(config::Is_Active_String(unrelated));
        DOCTEST_CHECK(arena::Get_Stats(first_arena).reference_count == 1);
        arena::Release(first_arena);

        DOCTEST_REQUIRE(config::Commit_State(&state));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target != second);
        DOCTEST_CHECK(std::strcmp(TEST_CONFIG_ARENA::target, "override") == 0);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("vector leaf writes grow by element and use fill defaults") {
        config::Shutdown_Runtime();
        TEST_CONFIG_ARENA::dynamic = {{3, 4}};
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&state));
        DOCTEST_REQUIRE(state.values.size() == 2);
        DOCTEST_REQUIRE(state.values[1].aggregate_values.size() == 2);
        const config::s_typed_value original_vector = state.values[1];

        config::s_typed_value value;
        value.type = config::CONFIG_VALUE_UINT8;
        value.integer_value = 9;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&state, 1, 0, value));
        DOCTEST_CHECK(config::State_Value_Is_Non_Default(&state, 1));
        DOCTEST_CHECK(config::State_Leaf_Is_Non_Default(&state, 1, 0));
        DOCTEST_REQUIRE(state.non_default_variable_ids.size() == 1);
        DOCTEST_CHECK(state.non_default_variable_ids[0] == 1);
        value.integer_value = 3;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&state, 1, 0, value));
        DOCTEST_CHECK_FALSE(config::State_Value_Is_Non_Default(&state, 1));
        DOCTEST_CHECK_FALSE(config::State_Leaf_Is_Non_Default(&state, 1, 0));
        DOCTEST_CHECK(state.non_default_variable_ids.empty());

        value.integer_value = 9;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&state, 1, 7, value));
        DOCTEST_REQUIRE(state.values[1].aggregate_values.size() == 8);
        DOCTEST_CHECK(state.values[1].aggregate_values[0].integer_value == 3);
        DOCTEST_CHECK(state.values[1].aggregate_values[1].integer_value == 4);
        for (size_t leaf = 2; leaf < 7; ++leaf) {
            DOCTEST_CHECK(state.values[1].aggregate_values[leaf].integer_value == 0);
        }
        DOCTEST_CHECK(state.values[1].aggregate_values[7].integer_value == 9);
        DOCTEST_CHECK(config::State_Value_Is_Non_Default(&state, 1));
        DOCTEST_CHECK(config::State_Leaf_Is_Non_Default(&state, 1, 7));
        DOCTEST_CHECK(state.non_default_values.size() == 1);

        config::s_typed_value wrong_type;
        wrong_type.type = config::CONFIG_VALUE_STRING;
        wrong_type.string_value = "invalid";
        const size_t size_before_rejected_write =
            state.values[1].aggregate_values.size();
        DOCTEST_CHECK_FALSE(config::Set_State_Leaf_Value(
            &state, 1, 20, wrong_type));
        DOCTEST_CHECK(state.values[1].aggregate_values.size() ==
            size_before_rejected_write);

        DOCTEST_REQUIRE(config::Validate_State(&state));
        DOCTEST_REQUIRE(config::Commit_State(&state));
        DOCTEST_REQUIRE(TEST_CONFIG_ARENA::dynamic.size() == 4);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 3);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].bar == 4);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[3].foo == 0);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[3].bar == 9);

        DOCTEST_REQUIRE(config::Set_State_Value(
            &state, 1, original_vector));
        DOCTEST_CHECK_FALSE(config::State_Value_Is_Non_Default(&state, 1));
        DOCTEST_CHECK(state.non_default_aggregates.empty());
        DOCTEST_REQUIRE(config::Validate_State(&state));
        DOCTEST_REQUIRE(config::Commit_State(&state));
        DOCTEST_REQUIRE(TEST_CONFIG_ARENA::dynamic.size() == 1);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 3);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].bar == 4);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("failed write sets restore repeated leaves vector growth and tracking") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&state));
        config::s_typed_value value = state.values[1].aggregate_values[0];
        value.integer_value = 8;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&state, 1, 0, value));
        DOCTEST_REQUIRE(config::Validate_State(&state));
        const config::s_config_state original = state;

        config::s_config_write_set writes;
        config::s_config_write write;
        write.variable_id = 1;
        write.value = value;
        write.value.integer_value = 3;
        writes.writes.push_back(write);
        write.value.integer_value = 9;
        writes.writes.push_back(write);
        write.leaf_index = 7;
        writes.writes.push_back(write);
        write.variable_id = 0;
        write.leaf_index = 0;
        write.value = state.values[0];
        write.value.string_value = "override";
        writes.writes.push_back(write);

        DOCTEST_SUBCASE("malformed target after successful writes") {
            state.values[0] = {};
            state.values[0].type = config::CONFIG_VALUE_AGGREGATE;
        }
        DOCTEST_SUBCASE("fill default failure after successful writes") {
            write.variable_id = 1;
            write.leaf_index = 9;
            write.value = value;
            writes.writes.push_back(write);
            TEST_CONFIG_ARENA::fail_default_leaf = 9;
        }

        const config::s_config_state before = state;
        config::s_cfg_diagnostic diagnostic;
        const bool applied = config::Apply_Write_Set(&writes, &state, &diagnostic);
        TEST_CONFIG_ARENA::fail_default_leaf = UINT32_MAX;
        DOCTEST_CHECK_FALSE(applied);
        for (size_t index = 0; index < before.values.size(); ++index) {
            DOCTEST_CHECK(config::Config_Values_Equal(state.values[index], before.values[index]));
        }
        DOCTEST_CHECK_FALSE(diagnostic.message.empty());
        DOCTEST_CHECK(config::Config_Values_Equal(state.values[1], original.values[1]));
        DOCTEST_CHECK(state.validated == original.validated);
        DOCTEST_CHECK(state.non_default_values == original.non_default_values);
        DOCTEST_REQUIRE(state.non_default_variable_ids.size() == 1);
        DOCTEST_CHECK(state.non_default_variable_ids[0] == 1);
        DOCTEST_CHECK(state.non_default_variable_positions[1] == 0);
        DOCTEST_REQUIRE(state.non_default_aggregates.size() == 1);
        DOCTEST_CHECK(state.non_default_aggregate_positions[1] == 0);
        DOCTEST_CHECK(state.non_default_aggregates[0].leaf_count == 1);
        DOCTEST_CHECK(state.non_default_aggregates[0].leaves == original.non_default_aggregates[0].leaves);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("vector stages roll back growth even after shrinking to baseline size") {
        config::Shutdown_Runtime();
        config::s_effective_config defaults;
        DOCTEST_REQUIRE(config::Initialize_Effective_Config(&defaults));
        config::s_config_baseline baseline;
        DOCTEST_REQUIRE(config::Freeze_Config_Baseline(&defaults, &baseline));
        config::s_config_stage stage;
        DOCTEST_REQUIRE(config::Begin_Config_Stage(&baseline, &stage));
        config::s_config_state grown = baseline.effective.state;
        config::s_typed_value value = grown.values[1].aggregate_values[0];
        value.integer_value = 9;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&grown, 1, 7, value));
        const uint32_t variable_id = 1;
        const config::s_config_snapshot_descriptor descriptor = {
            &variable_id, 1, UINT32_C(1) << config::CONFIG_GROUP_GAMEPLAY};
        DOCTEST_REQUIRE(config::Apply_Config_State_To_Stage(&descriptor, &grown, &stage));
        DOCTEST_REQUIRE(config::Config_Delta_Contains(&stage.touched, 1, 7));
        DOCTEST_SUBCASE("still grown") {}
        DOCTEST_SUBCASE("shrunk back to baseline") {
            DOCTEST_REQUIRE(config::Apply_Config_State_To_Stage(
                &descriptor, &baseline.effective.state, &stage));
        }
        DOCTEST_REQUIRE(config::Rollback_Config_Stage(&stage));
        DOCTEST_CHECK(config::Config_Values_Equal(stage.effective.state.values[1], baseline.effective.state.values[1]));
        DOCTEST_CHECK(stage.effective.state.non_default_variable_ids.empty());
        DOCTEST_CHECK(stage.effective.state.non_default_aggregates.empty());
        DOCTEST_CHECK(stage.touched.variable_ids.empty());
        DOCTEST_CHECK(stage.touched.aggregates.empty());
        DOCTEST_CHECK(stage.effective.state.validated);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("transitions restore leaves resize vectors and respect group masks") {
        config::Shutdown_Runtime();
        config::s_config_state defaults;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&defaults));
        DOCTEST_REQUIRE(config::Commit_State(&defaults));
        char* original_string = TEST_CONFIG_ARENA::target;
        config::s_config_state grown = defaults;
        config::s_typed_value value = defaults.values[1].aggregate_values[0];
        value.integer_value = 9;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&grown, 1, 0, value));
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&grown, 1, 7, value));
        config::s_typed_value label = defaults.values[0];
        label.string_value = "changed";
        DOCTEST_REQUIRE(config::Set_State_Value(&grown, 0, label));
        const uint32_t gameplay = UINT32_C(1) << config::CONFIG_GROUP_GAMEPLAY;
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &defaults, &grown));
        DOCTEST_REQUIRE(TEST_CONFIG_ARENA::dynamic.size() == 4);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 9);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[3].bar == 9);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target == original_string);

        config::s_config_state restored_leaf = grown;
        DOCTEST_REQUIRE(config::Set_State_Leaf_Value(&restored_leaf, 1, 0, defaults.values[1].aggregate_values[0]));
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &grown, &restored_leaf));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 3);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[3].bar == 9);
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &restored_leaf, &restored_leaf));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target == original_string);
        DOCTEST_REQUIRE(config::Commit_Config_Transition(gameplay, &restored_leaf, &defaults));
        DOCTEST_REQUIRE(TEST_CONFIG_ARENA::dynamic.size() == 1);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].foo == 3);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::dynamic[0].bar == 4);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("string transitions retain retired arenas and reject invalid candidates") {
        config::Shutdown_Runtime();
        config::s_config_state prior;
        DOCTEST_REQUIRE(config::Initialize_Tracked_Defaults(&prior));
        DOCTEST_REQUIRE(config::Commit_State(&prior));
        char* original = TEST_CONFIG_ARENA::target;
        arena::s_arena* retained = config::Acquire_String_Arena(config::CONFIG_GROUP_INTERFACE);
        config::s_config_state current = prior;
        config::s_typed_value label = prior.values[0];
        label.string_value = "changed";
        DOCTEST_REQUIRE(config::Set_State_Value(&current, 0, label));
        DOCTEST_REQUIRE(config::Commit_Config_Transition(UINT32_MAX, &prior, &current));
        DOCTEST_CHECK(std::strcmp(original, "default") == 0);
        DOCTEST_CHECK(std::strcmp(TEST_CONFIG_ARENA::target, "changed") == 0);
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target != original);
        DOCTEST_CHECK(config::Is_Active_String(TEST_CONFIG_ARENA::target));
        DOCTEST_CHECK_FALSE(config::Is_Active_String(original));
        DOCTEST_CHECK(arena::Get_Stats(retained).reference_count == 1);
        arena::Release(retained);

        config::s_config_state invalid = current;
        label.string_value = "this value exceeds the configured limit";
        DOCTEST_REQUIRE(config::Set_State_Value(&invalid, 0, label));
        char* unchanged = TEST_CONFIG_ARENA::target;
        DOCTEST_CHECK_FALSE(config::Commit_Config_Transition(UINT32_MAX, &current, &invalid));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target == unchanged);
        DOCTEST_REQUIRE(config::Commit_Config_Transition(UINT32_MAX, &current, &prior));
        DOCTEST_CHECK(std::strcmp(TEST_CONFIG_ARENA::target, "default") == 0);
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("nullable arena strings commit empty values as null") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Defaults(&state));
        state.values[0].string_value.clear();
        DOCTEST_REQUIRE(config::Commit_State(&state));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target == nullptr);

        config::s_config_state current;
        DOCTEST_REQUIRE(config::Read_Current_State(&current));
        DOCTEST_REQUIRE(current.values.size() == 2);
        DOCTEST_CHECK(current.values[0].string_value.empty());
        config::Shutdown_Runtime();
    }

    DOCTEST_TEST_CASE("failed whole-state validation preserves the active generation") {
        config::Shutdown_Runtime();
        config::s_config_state state;
        DOCTEST_REQUIRE(config::Initialize_Defaults(&state));
        DOCTEST_REQUIRE(config::Commit_State(&state));
        char* original = TEST_CONFIG_ARENA::target;
        arena::s_arena* original_arena =
            config::Acquire_String_Arena(config::CONFIG_GROUP_INTERFACE);
        DOCTEST_REQUIRE(original_arena != nullptr);

        state.values[0].string_value = "this value exceeds the configured limit";
        DOCTEST_CHECK_FALSE(config::Commit_State(&state));
        DOCTEST_CHECK(TEST_CONFIG_ARENA::target == original);
        DOCTEST_CHECK(std::strcmp(TEST_CONFIG_ARENA::target, "default") == 0);
        arena::s_arena* current_arena =
            config::Acquire_String_Arena(config::CONFIG_GROUP_INTERFACE);
        DOCTEST_CHECK(current_arena == original_arena);

        arena::Release(current_arena);
        arena::Release(original_arena);
        config::Shutdown_Runtime();
    }
}