#include "pch.h"

#include <cstring>

#include "config/cfg_parser.h"
#include "config/config_decode.h"
#include "config/config_runtime.h"
#include "doctest/doctest.h"
#include "game/build_queue.h"

namespace {
    const config::s_symbol_descriptor WEAPON_VALUES[] = {
        {"none", 0},
        {"plasma_cannon", 11},
        {"mass_driver", 4},
    };
    const config::s_symbol_descriptor ARC_VALUES[] = {
        {"none", 0},
        {"all", 15},
    };
    const config::s_symbol_descriptor ARC_FLAGS[] = {
        {"forward", 1},
        {"aft", 2},
        {"left", 4},
        {"right", 8},
    };
    const config::s_symbol_descriptor MOD_VALUES[] = {
        {"none", 0},
    };
    const config::s_symbol_descriptor MOD_FLAGS[] = {
        {"heavy", 2},
        {"eccm", 0x200},
    };
    const config::s_symbol_descriptor SPECIAL_VALUES[] = {
        {"none", 0},
        {"augmented_engines", 1},
        {"battle_scanner", 5},
        {"damper_field", 7},
    };
    const uint8_t BITMAP_NONE[] = {0, 0, 0, 0, 0};
    const uint8_t BITMAP_SELF_REPAIRING[] = {0x08, 0, 0, 0, 0x80};
    const uint8_t BITMAP_AUTOMATED_REPAIR[] = {0x08, 0, 0, 0, 0};
    const uint8_t BITMAP_REGENERATION[] = {0, 0, 0, 0, 0x80};
    const uint8_t BITMAP_KNOWN_BITS[] = {0x08, 0, 0, 0, 0x80};
    const config::s_bitmap_symbol_descriptor BITMAP_VALUES[] = {
        {"none", BITMAP_NONE},
        {"self_repairing", BITMAP_SELF_REPAIRING},
    };
    const config::s_bitmap_symbol_descriptor BITMAP_FLAGS[] = {
        {"automated_repair", BITMAP_AUTOMATED_REPAIR},
        {"regeneration", BITMAP_REGENERATION},
    };
    const config::s_array_index_alias_descriptor ARRAY_INDEX_ALIASES[] = {
        {21, "plasma_cannon", 11},
    };

    const config::s_runtime_schema* Aggregate_Schema_() {
        static config::s_type_descriptor types[18];
        static bool initialized = false;
        if (!initialized) {
            const config::e_value_type value_types[] = {
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT16,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT16,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_UINT8,
                config::CONFIG_VALUE_STRING,
                config::CONFIG_VALUE_RATIO,
                config::CONFIG_VALUE_UINT16,
                config::CONFIG_VALUE_BITMAP,
            };
            for (size_t index = 0; index < 18; ++index) {
                types[index].value_type = value_types[index];
                types[index].scalar_kind = config::CONFIG_SCALAR_PRIMITIVE;
                types[index].ratio_component_type = config::CONFIG_VALUE_INT16;
            }
            for (size_t index : {size_t(1), size_t(6)}) {
                types[index].scalar_kind = config::CONFIG_SCALAR_ENUM;
                types[index].values = WEAPON_VALUES;
                types[index].value_count = 3;
                types[index].allow_numeric = true;
                types[index].reject_unknown_numeric = true;
            }
            for (size_t index : {size_t(2), size_t(7)}) {
                types[index].scalar_kind = config::CONFIG_SCALAR_MASK;
                types[index].values = ARC_VALUES;
                types[index].value_count = 2;
                types[index].flags = ARC_FLAGS;
                types[index].flag_count = 4;
                types[index].allow_numeric = true;
                types[index].reject_unknown_numeric = true;
                types[index].known_bits = 15;
            }
            for (size_t index : {size_t(3), size_t(8)}) {
                types[index].scalar_kind = config::CONFIG_SCALAR_MASK;
                types[index].values = MOD_VALUES;
                types[index].value_count = 1;
                types[index].flags = MOD_FLAGS;
                types[index].flag_count = 2;
                types[index].allow_numeric = true;
                types[index].reject_unknown_numeric = true;
                types[index].known_bits = 0x202;
            }
            for (size_t index = 11; index <= 13; ++index) {
                types[index].scalar_kind = config::CONFIG_SCALAR_ENUM;
                types[index].values = SPECIAL_VALUES;
                types[index].value_count = 4;
                types[index].allow_numeric = true;
                types[index].reject_unknown_numeric = true;
            }
            types[14].has_min_length = true;
            types[14].has_max_length = true;
            types[14].min_length = 1;
            types[14].max_length = 8;
            types[15].scalar_kind = config::CONFIG_SCALAR_RATIO;
            types[17].scalar_kind = config::CONFIG_SCALAR_BITMAP;
            types[17].bitmap_values = BITMAP_VALUES;
            types[17].bitmap_value_count = 2;
            types[17].bitmap_flags = BITMAP_FLAGS;
            types[17].bitmap_flag_count = 2;
            types[17].bitmap_known_bits = BITMAP_KNOWN_BITS;
            types[17].bitmap_bit_count = 40;
            initialized = true;
        }

        static config::s_schema_node_descriptor nodes[] = {
            {"guardian", config::CONFIG_NODE_RECORD, config::CONFIG_NO_NODE, 0, 0, 15, config::CONFIG_NO_VARIABLE},
            {"appearance", config::CONFIG_NODE_SCALAR, 0, 0, 0, 0, 0},
            {"weapon", config::CONFIG_NODE_ARRAY, 0, 0, 1, 10, config::CONFIG_NO_VARIABLE},
            {"", config::CONFIG_NODE_RECORD, 2, 1, 1, 5, config::CONFIG_NO_VARIABLE},
            {"type", config::CONFIG_NODE_SCALAR, 3, 0, 1, 1, 1},
            {"arc", config::CONFIG_NODE_SCALAR, 3, 0, 2, 2, 2},
            {"mods", config::CONFIG_NODE_SCALAR, 3, 0, 3, 3, 3},
            {"ammo", config::CONFIG_NODE_SCALAR, 3, 0, 4, 4, 4},
            {"amount", config::CONFIG_NODE_SCALAR, 3, 0, 5, 5, 5},
            {"", config::CONFIG_NODE_RECORD, 2, 2, 6, 10, config::CONFIG_NO_VARIABLE},
            {"type", config::CONFIG_NODE_SCALAR, 9, 0, 6, 6, 6},
            {"arc", config::CONFIG_NODE_SCALAR, 9, 0, 7, 7, 7},
            {"mods", config::CONFIG_NODE_SCALAR, 9, 0, 8, 8, 8},
            {"ammo", config::CONFIG_NODE_SCALAR, 9, 0, 9, 9, 9},
            {"amount", config::CONFIG_NODE_SCALAR, 9, 0, 10, 10, 10},
            {"special", config::CONFIG_NODE_ARRAY, 0, 0, 11, 13, config::CONFIG_NO_VARIABLE},
            {"", config::CONFIG_NODE_SCALAR, 15, 1, 11, 11, 11},
            {"", config::CONFIG_NODE_SCALAR, 15, 2, 12, 12, 12},
            {"", config::CONFIG_NODE_SCALAR, 15, 3, 13, 13, 13},
            {"label", config::CONFIG_NODE_SCALAR, 0, 0, 14, 14, 14},
            {"reduction", config::CONFIG_NODE_SCALAR, 0, 0, 15, 15, 15},
            {"weapon", config::CONFIG_NODE_ARRAY, config::CONFIG_NO_NODE, 0, 16, 16, config::CONFIG_NO_VARIABLE},
            {"", config::CONFIG_NODE_RECORD, 21, 11, 16, 16, config::CONFIG_NO_VARIABLE},
            {"available_mods", config::CONFIG_NODE_SCALAR, 22, 0, 16, 16, 16},
            {"special_devices", config::CONFIG_NODE_SCALAR, config::CONFIG_NO_NODE, 0, 17, 17, 17},
        };
        static bool nodes_initialized = false;
        if (!nodes_initialized) {
            uint32_t last_children[sizeof(nodes) / sizeof(nodes[0])];
            for (size_t index = 0; index < sizeof(nodes) / sizeof(nodes[0]); ++index) {
                nodes[index].first_child = config::CONFIG_NO_NODE;
                nodes[index].next_sibling = config::CONFIG_NO_NODE;
                nodes[index].type = nodes[index].kind == config::CONFIG_NODE_SCALAR ? &types[nodes[index].variable_id] : nullptr;
                last_children[index] = config::CONFIG_NO_NODE;
            }
            for (uint32_t index = 0; index < sizeof(nodes) / sizeof(nodes[0]); ++index) {
                const uint32_t parent = nodes[index].parent;
                if (parent == config::CONFIG_NO_NODE) continue;
                if (nodes[parent].first_child == config::CONFIG_NO_NODE) {
                    nodes[parent].first_child = index;
                } else {
                    nodes[last_children[parent]].next_sibling = index;
                }
                last_children[parent] = index;
            }
            nodes_initialized = true;
        }
        static const config::s_runtime_schema schema = {
            nodes,
            sizeof(nodes) / sizeof(nodes[0]),
            18,
            nullptr,
            ARRAY_INDEX_ALIASES,
            sizeof(ARRAY_INDEX_ALIASES) / sizeof(ARRAY_INDEX_ALIASES[0]),
        };
        return &schema;
    }

    const config::s_runtime_schema* Map_Vector_Schema_() {
        static const config::s_type_descriptor uint8_type = {
            config::CONFIG_VALUE_UINT8,
            false,
            false,
            0,
            0,
            config::CONFIG_SCALAR_PRIMITIVE,
        };
        static const config::s_schema_node_descriptor nodes[] = {
            {"map", config::CONFIG_NODE_MAP, config::CONFIG_NO_NODE, 0, 0, 1, config::CONFIG_NO_VARIABLE, false, 0, 1, config::CONFIG_NO_NODE, nullptr, 0, 2, 2, false, false, 0, 0, 0},
            {"", config::CONFIG_NODE_RECORD, 0, 0, 0, 0, 0, false, 0, 2, config::CONFIG_NO_NODE, nullptr, 0, 0, 0, false, false, 0, 1, 0},
            {"foo", config::CONFIG_NODE_SCALAR, 1, 0, 0, 0, 0, false, 0, config::CONFIG_NO_NODE, 3, &uint8_type, 0, 0, 0, false, false, 0, 0, 0},
            {"bar", config::CONFIG_NODE_SCALAR, 1, 0, 0, 0, 0, false, 0, config::CONFIG_NO_NODE, config::CONFIG_NO_NODE, &uint8_type, 0, 0, 0, false, false, 1, 1, 1},
            {"vector", config::CONFIG_NODE_VECTOR, config::CONFIG_NO_NODE, 0, 2, 2, 2, false, 0, 5, config::CONFIG_NO_NODE, nullptr, 0, 1, 2, false, false, 0, 1, 0},
            {"", config::CONFIG_NODE_RECORD, 4, 0, 2, 2, 2, false, 0, 6, config::CONFIG_NO_NODE, nullptr, 0, 0, 0, false, false, 0, 1, 0},
            {"foo", config::CONFIG_NODE_SCALAR, 5, 0, 2, 2, 2, false, 0, config::CONFIG_NO_NODE, 7, &uint8_type, 0, 0, 0, false, false, 0, 0, 0},
            {"bar", config::CONFIG_NODE_SCALAR, 5, 0, 2, 2, 2, false, 0, config::CONFIG_NO_NODE, config::CONFIG_NO_NODE, &uint8_type, 0, 0, 0, false, false, 1, 1, 1},
        };
        static const config::s_runtime_variable_descriptor variables[] = {
            {"map.0", 1, config::CONFIG_GROUP_GAMEPLAY, true, false, 0, 2, false},
            {"map.1", 1, config::CONFIG_GROUP_GAMEPLAY, true, false, 0, 2, false},
            {"vector", 5, config::CONFIG_GROUP_GAMEPLAY, true, true, 1, 2, false},
        };
        static const config::s_array_index_alias_descriptor aliases[] = {
            {0, "first", 0},
            {0, "second", 1},
        };
        static const config::s_runtime_schema schema = {
            nodes,
            sizeof(nodes) / sizeof(nodes[0]),
            sizeof(variables) / sizeof(variables[0]),
            variables,
            aliases,
            sizeof(aliases) / sizeof(aliases[0]),
        };
        return &schema;
    }

    bool Parse_Assignment_(const char* source, config::s_cfg_statement* statement) {
        config::s_cfg_document document;
        config::s_cfg_diagnostic diagnostic;
        if (!config::Parse_Cfg("decode.cfg", source, std::strlen(source), &document, &diagnostic) || document.statements.size() != 1) {
            return false;
        }
        *statement = std::move(document.statements.front());
        return true;
    }

    bool Decode_(const char* source, config::s_config_write_set* write_set, config::s_cfg_diagnostic* diagnostic) {
        config::s_cfg_statement statement;
        if (!Parse_Assignment_(source, &statement)) {
            return false;
        }
        return config::Decode_Assignment(&statement, write_set, diagnostic);
    }

    bool Decode_Aggregate_(const char* source, config::s_config_write_set* write_set, config::s_cfg_diagnostic* diagnostic) {
        config::s_cfg_statement statement;
        return Parse_Assignment_(source, &statement) &&
            config::Decode_Assignment_With_Schema(
                &statement, Aggregate_Schema_(), write_set, diagnostic);
    }

    bool Decode_Map_Vector_(const char* source,
        config::s_config_write_set* write_set,
        config::s_cfg_diagnostic* diagnostic) {
        config::s_cfg_statement statement;
        return Parse_Assignment_(source, &statement) &&
            config::Decode_Assignment_With_Schema(
                &statement, Map_Vector_Schema_(), write_set, diagnostic);
    }

    void Add_Sentinel_(config::s_config_write_set* write_set) {
        config::s_config_write write;
        write.variable_id = config::CONFIG_NO_VARIABLE;
        write.canonical_path = "sentinel";
        write_set->writes.push_back(std::move(write));
    }
} // namespace

DOCTEST_TEST_CASE("configuration namespaces group paths without becoming assignable") {
    config::s_config_write_set writes;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Decode_(
        "ships guardian weapons 1 amount = 3;", &writes, &diagnostic));
    DOCTEST_REQUIRE(writes.writes.size() == 1);
    DOCTEST_CHECK(writes.writes[0].canonical_path ==
        "ships.guardian.weapons.1.amount");
    DOCTEST_CHECK(writes.writes[0].value.integer_value == 3);

    DOCTEST_REQUIRE_MESSAGE(Decode_(
                                "ships antaran_offensive impossible titan weapons 8 amount = 12;",
                                &writes,
                                &diagnostic),
        diagnostic.message);
    DOCTEST_REQUIRE(writes.writes.size() == 1);
    DOCTEST_CHECK(writes.writes[0].canonical_path ==
        "ships.antaran_offensive.4.4.weapons.8.amount");
    DOCTEST_CHECK(writes.writes[0].value.integer_value == 12);

    DOCTEST_REQUIRE_MESSAGE(Decode_(
                                "ships antaran_defensive impossible titan weapons 8 amount = 6;",
                                &writes,
                                &diagnostic),
        diagnostic.message);
    DOCTEST_REQUIRE(writes.writes.size() == 1);
    DOCTEST_CHECK(writes.writes[0].canonical_path ==
        "ships.antaran_defensive.4.4.weapons.8.amount");
    DOCTEST_CHECK(writes.writes[0].value.integer_value == 6);

    DOCTEST_CHECK_FALSE(Decode_("ships = 1;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message == "configuration namespaces are not assignable");
    DOCTEST_CHECK(writes.writes.empty());
}

DOCTEST_TEST_CASE("maps preserve key boundaries and numeric canonical paths") {
    config::s_config_write_set writes;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE_MESSAGE(Decode_Map_Vector_(
                                "map first foo = 9 10;", &writes, &diagnostic),
        diagnostic.message);
    DOCTEST_REQUIRE(writes.writes.size() == 2);
    DOCTEST_CHECK(writes.writes[0].variable_id == 0);
    DOCTEST_CHECK(writes.writes[0].leaf_index == 0);
    DOCTEST_CHECK(writes.writes[0].canonical_path == "map.0.foo");
    DOCTEST_CHECK(writes.writes[1].canonical_path == "map.0.bar");

    DOCTEST_REQUIRE(Decode_Map_Vector_(
        "map = 1 2 3 4;", &writes, &diagnostic));
    DOCTEST_REQUIRE(writes.writes.size() == 4);
    DOCTEST_CHECK(writes.writes[2].variable_id == 1);
    DOCTEST_CHECK(writes.writes[2].leaf_index == 0);
    DOCTEST_CHECK(writes.writes[2].canonical_path == "map.1.foo");
    DOCTEST_CHECK(writes.writes[3].canonical_path == "map.1.bar");

    DOCTEST_REQUIRE(Decode_Map_Vector_(
        "map second = foo: 7 bar: 8;", &writes, &diagnostic));
    DOCTEST_REQUIRE(writes.writes.size() == 2);
    DOCTEST_CHECK(writes.writes[0].variable_id == 1);
    DOCTEST_CHECK(writes.writes[0].canonical_path == "map.1.foo");

    DOCTEST_CHECK_FALSE(Decode_Map_Vector_(
        "map 2 foo = 1;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message ==
        "unknown configuration variable 'map.2'");
    DOCTEST_CHECK_FALSE(Decode_Map_Vector_(
        "map first foo = 1 2 3;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message ==
        "value 3 exceeds selected configuration object 'map.0.foo'");
}

DOCTEST_TEST_CASE("vectors use zero-based indexes and allow assignments beyond defaults") {
    config::s_config_write_set writes;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE_MESSAGE(Decode_Map_Vector_(
                                "vector 3 foo = 9 10;", &writes, &diagnostic),
        diagnostic.message);
    DOCTEST_REQUIRE(writes.writes.size() == 2);
    DOCTEST_CHECK(writes.writes[0].variable_id == 2);
    DOCTEST_CHECK(writes.writes[0].leaf_index == 6);
    DOCTEST_CHECK(writes.writes[0].canonical_path == "vector.3.foo");
    DOCTEST_CHECK(writes.writes[1].canonical_path == "vector.3.bar");

    DOCTEST_REQUIRE(Decode_Map_Vector_(
        "vector = 1 2 3 4 5;", &writes, &diagnostic));
    DOCTEST_REQUIRE(writes.writes.size() == 5);
    DOCTEST_CHECK(writes.writes[4].leaf_index == 4);
    DOCTEST_CHECK(writes.writes[4].canonical_path == "vector.2.foo");

    DOCTEST_CHECK_FALSE(Decode_Map_Vector_(
        "vector -1 foo = 1;", &writes, &diagnostic));
}

DOCTEST_TEST_CASE("all top-level configuration parameters belong to namespaces") {
    const config::s_runtime_schema* schema = config::Get_Runtime_Schema();
    DOCTEST_REQUIRE(schema != nullptr);

    for (size_t index = 0; index < schema->node_count; ++index) {
        if (schema->nodes[index].parent == config::CONFIG_NO_NODE) {
            DOCTEST_CHECK(schema->nodes[index].kind == config::CONFIG_NODE_NAMESPACE);
        }
    }
}

DOCTEST_TEST_CASE("implicit numeric lists replace their complete bounded value set") {
    config::s_config_write_set writes;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Decode_("ai ai_race_variants = 4 5;", &writes, &diagnostic));
    DOCTEST_REQUIRE(writes.writes.size() == 6);
    DOCTEST_CHECK(writes.writes[0].canonical_path == "ai.ai_race_variants.1");
    DOCTEST_CHECK(writes.writes[0].value.integer_value == 4);
    DOCTEST_CHECK(writes.writes[1].value.integer_value == 5);
    for (size_t index = 2; index < writes.writes.size(); ++index) {
        DOCTEST_CHECK(writes.writes[index].value.integer_value == 0);
    }

    DOCTEST_CHECK_FALSE(Decode_(
        "ai ai_race_variants = 1 2 1;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message == "list values must be unique");
    DOCTEST_CHECK_FALSE(Decode_(
        "ai ai_race_variants = 0;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message ==
        "implicit list terminators cannot be assigned explicitly");
    DOCTEST_CHECK_FALSE(Decode_(
        "ai ai_race_variants = 1 2 3 4 5 1;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message == "implicit list exceeds its maximum item count");
    DOCTEST_CHECK_FALSE(Decode_(
        "ai ai_race_variants 1 = 4;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message ==
        "implicit lists must be assigned as a complete list");
    DOCTEST_CHECK_FALSE(Decode_(
        "ai ai_race_variants = 1: 4;", &writes, &diagnostic));
    DOCTEST_CHECK(diagnostic.message ==
        "implicit lists must be assigned as a complete list");
}

DOCTEST_TEST_SUITE("configuration assignment decode") {
    DOCTEST_TEST_CASE("stable names resolve to runtime variables") {
        config::t_variable_id interval_id = config::CONFIG_NO_VARIABLE;
        DOCTEST_REQUIRE(config::Find_Variable(
            "interface.scroll_interval", std::strlen("interface.scroll_interval"), &interval_id));
        const config::s_type_descriptor* interval =
            config::Get_Variable_Type(interval_id);
        DOCTEST_REQUIRE(interval != nullptr);
        DOCTEST_CHECK(interval->value_type == config::CONFIG_VALUE_UINT8);
        std::string interval_name;
        DOCTEST_REQUIRE(config::Get_Variable_Name(interval_id, &interval_name));
        DOCTEST_CHECK(interval_name == "interface.scroll_interval");
        config::t_variable_id queue_id = config::CONFIG_NO_VARIABLE;
        DOCTEST_REQUIRE(config::Find_Variable(
            "colony.build_queue.1.199", std::strlen("colony.build_queue.1.199"), &queue_id));
        std::string queue_name;
        DOCTEST_REQUIRE(config::Get_Variable_Name(queue_id, &queue_name));
        DOCTEST_CHECK(queue_name == "colony.build_queue.1.199");
        const config::s_schema_node_descriptor* queue_leaf =
            config::Get_Variable_Node(config::Get_Runtime_Schema(), queue_id);
        DOCTEST_REQUIRE(queue_leaf != nullptr);
        config::s_variable_location queue_location = {};
        DOCTEST_REQUIRE(config::Get_Variable_Location(
            config::Get_Runtime_Schema(), queue_id, &queue_location));
        DOCTEST_CHECK(queue_leaf->kind == config::CONFIG_NODE_SCALAR);
        DOCTEST_CHECK(queue_leaf->variable_id + queue_location.variable_offset == queue_id);
        DOCTEST_CHECK(queue_leaf->type == config::Get_Variable_Type(queue_id));
        const config::s_runtime_schema* schema = config::Get_Runtime_Schema();
        const config::s_schema_node_descriptor* colony = nullptr;
        for (size_t index = 0; index < schema->node_count; ++index) {
            if (schema->nodes[index].parent == config::CONFIG_NO_NODE &&
                std::strcmp(schema->nodes[index].id, "colony") == 0) {
                colony = &schema->nodes[index];
                break;
            }
        }
        DOCTEST_REQUIRE(colony != nullptr);
        const config::s_schema_node_descriptor* build_queue = nullptr;
        for (uint32_t child = colony->first_child;
             child != config::CONFIG_NO_NODE;
             child = schema->nodes[child].next_sibling) {
            if (std::strcmp(schema->nodes[child].id, "build_queue") == 0) {
                build_queue = &schema->nodes[child];
                break;
            }
        }
        DOCTEST_REQUIRE(build_queue != nullptr);
        size_t template_count = 0;
        for (uint32_t child = build_queue->first_child;
             child != config::CONFIG_NO_NODE;
             child = schema->nodes[child].next_sibling) {
            ++template_count;
            DOCTEST_CHECK(schema->nodes[child].array_element_count ==
                BUILD_QUEUE::INSTRUCTION_CAPACITY);
            DOCTEST_CHECK(schema->nodes[child].array_element_variable_count == 1);
            DOCTEST_REQUIRE(schema->nodes[child].first_child != config::CONFIG_NO_NODE);
            DOCTEST_CHECK(schema->nodes[schema->nodes[child].first_child].kind ==
                config::CONFIG_NODE_SCALAR);
        }
        DOCTEST_CHECK(build_queue->array_element_count == BUILD_QUEUE::PRESET_COUNT);
        DOCTEST_CHECK(build_queue->array_element_variable_count ==
            BUILD_QUEUE::INSTRUCTION_CAPACITY);
        DOCTEST_CHECK(template_count == 1);
        DOCTEST_CHECK_FALSE(config::Find_Variable("interface.scroll", 16, &interval_id));
        DOCTEST_CHECK_FALSE(config::Find_Variable(
            "interface.SCROLL_INTERVAL", 25, &interval_id));
        DOCTEST_CHECK_FALSE(config::Find_Variable(
            "colony.build_queue.01.199", std::strlen("colony.build_queue.01.199"), &queue_id));
        DOCTEST_CHECK_FALSE(config::Find_Variable(nullptr, 0, &interval_id));
        DOCTEST_CHECK(config::Get_Variable_Type(config::variable_count) == nullptr);
    }

    DOCTEST_TEST_CASE("stable path writes resolve for replay in the current build") {
        config::s_config_write write;
        write.variable_id = 99;
        write.canonical_path = "interface.scroll_interval";
        write.value.type = config::CONFIG_VALUE_UINT8;
        write.value.integer_value = 35;
        write.provenance.source = "savegame";
        config::s_cfg_diagnostic diagnostic;

        DOCTEST_REQUIRE(config::Resolve_Write_Path(&write, &diagnostic));
        config::t_variable_id expected_id = config::CONFIG_NO_VARIABLE;
        DOCTEST_REQUIRE(config::Find_Variable(
            "interface.scroll_interval",
            std::strlen("interface.scroll_interval"),
            &expected_id));
        DOCTEST_CHECK(write.variable_id == expected_id);

        write.canonical_path = "weapons.foo.bar.1.baz";
        DOCTEST_CHECK_FALSE(config::Resolve_Write_Path(&write, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "unknown configuration variable 'weapons.foo.bar.1.baz'");
        DOCTEST_CHECK(diagnostic.location.source == "savegame");
    }

    DOCTEST_TEST_CASE("bool and integer assignments produce typed atomic writes") {
        struct s_integer_case {
            const char* source;
            const char* canonical_path;
            int64_t expected;
        };
        const s_integer_case integers[] = {
            {"interface scroll_interval = 75;", "interface.scroll_interval", 75},
            {"interface scroll_interval = 0x32;", "interface.scroll_interval", 50},
            {"interface scroll_interval = 0b110010;", "interface.scroll_interval", 50},
            {"interface scroll_interval = 25;", "interface.scroll_interval", 25},
        };

        for (const s_integer_case& test_case : integers) {
            config::s_config_write_set write_set;
            config::s_cfg_diagnostic diagnostic;
            DOCTEST_REQUIRE(Decode_(test_case.source, &write_set, &diagnostic));
            DOCTEST_REQUIRE(write_set.writes.size() == 1);
            DOCTEST_CHECK(write_set.writes[0].canonical_path == test_case.canonical_path);
            DOCTEST_CHECK(write_set.writes[0].value.type == config::CONFIG_VALUE_UINT8);
            DOCTEST_CHECK(write_set.writes[0].value.integer_value == test_case.expected);
            DOCTEST_CHECK(write_set.writes[0].provenance.source == "decode.cfg");
            DOCTEST_CHECK(write_set.writes[0].provenance.column > 1);
        }

        config::s_config_write_set enabled;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_(
            "interface no_outpost_prompt = true;", &enabled, &diagnostic));
        DOCTEST_REQUIRE(enabled.writes.size() == 1);
        DOCTEST_CHECK(enabled.writes[0].value.type == config::CONFIG_VALUE_BOOL);
        DOCTEST_CHECK(enabled.writes[0].value.bool_value);
        DOCTEST_CHECK(enabled.writes[0].canonical_path ==
            "interface.no_outpost_prompt");

        config::s_config_write_set disabled;
        DOCTEST_REQUIRE(Decode_(
            "interface warn_blockers = false;", &disabled, &diagnostic));
        DOCTEST_REQUIRE(disabled.writes.size() == 1);
        DOCTEST_CHECK_FALSE(disabled.writes[0].value.bool_value);
    }

    DOCTEST_TEST_CASE("resolution and decode failures retain no partial writes") {
        struct s_case {
            const char* source;
            const char* expected_message;
            uint32_t column;
        };
        const s_case cases[] = {
            {"unknown_setting = 1;", "unknown configuration variable 'unknown_setting'", 1},
            {"interface scroll_interval extra = 50;",
                "unknown configuration variable 'interface.scroll_interval.extra'",
                1},
            {"interface scroll_interval = 256;",
                "invalid value '256' for 'interface.scroll_interval'; expected uint8 in range 0..255",
                29},
            {"interface scroll_interval = -1;",
                "invalid value '-1' for 'interface.scroll_interval'; expected uint8 in range 0..255",
                29},
            {"interface scroll_interval = true;",
                "invalid value 'true' for 'interface.scroll_interval'; expected uint8",
                29},
            {"interface no_outpost_prompt = 1;",
                "invalid value '1' for 'interface.no_outpost_prompt'; expected bool",
                31},
            {"interface no_outpost_prompt = TRUE;",
                "invalid value 'TRUE' for 'interface.no_outpost_prompt'; expected bool",
                31},
            {"interface scroll_interval = -;",
                "an assignment containing only skips has no effect",
                29},
            {"interface scroll_interval = 25 50;",
                "value 2 exceeds selected configuration object 'interface.scroll_interval'",
                32},
        };

        for (const s_case& test_case : cases) {
            config::s_config_write_set write_set;
            config::s_cfg_diagnostic diagnostic;
            Add_Sentinel_(&write_set);
            DOCTEST_CHECK_FALSE(Decode_(test_case.source, &write_set, &diagnostic));
            DOCTEST_CHECK_MESSAGE(diagnostic.message == test_case.expected_message,
                test_case.source,
                ": ",
                diagnostic.message);
            DOCTEST_CHECK(diagnostic.location.source == "decode.cfg");
            DOCTEST_CHECK(diagnostic.location.line == 1);
            DOCTEST_CHECK(diagnostic.location.column == test_case.column);
            DOCTEST_CHECK(write_set.writes.empty());
        }
    }

    DOCTEST_TEST_CASE("non-assignment statements are rejected atomically") {
        config::s_cfg_document document;
        config::s_cfg_diagnostic parse_diagnostic;
        DOCTEST_REQUIRE(config::Parse_Cfg("decode.cfg", "stop;", 5, &document, &parse_diagnostic));
        DOCTEST_REQUIRE(document.statements.size() == 1);

        config::s_config_write_set write_set;
        Add_Sentinel_(&write_set);
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_CHECK_FALSE(config::Decode_Assignment(&document.statements[0],
            &write_set,
            &diagnostic));
        DOCTEST_CHECK(diagnostic.message == "expected an assignment statement");
        DOCTEST_CHECK(write_set.writes.empty());
    }

    DOCTEST_TEST_CASE("record and array assignments follow declaration order and boundaries") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian weapon 1 = plasma_cannon all heavy 0 2;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 5);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "guardian.weapon.1.type");
        DOCTEST_CHECK(writes.writes[0].value.integer_value == 11);
        DOCTEST_CHECK(writes.writes[1].value.integer_value == 15);
        DOCTEST_CHECK(writes.writes[2].value.integer_value == 2);
        DOCTEST_CHECK(writes.writes[4].value.integer_value == 2);

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian weapon = plasma_cannon all heavy 0 2 mass_driver forward none 0 10;",
            &writes,
            &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 10);
        DOCTEST_CHECK(writes.writes[5].canonical_path == "guardian.weapon.2.type");
        DOCTEST_CHECK(writes.writes[5].value.integer_value == 4);
    }

    DOCTEST_TEST_CASE("labeled groups decode as atomic relative scalar-start assignments") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian weapon 1 = type: plasma_cannon mods: heavy,eccm amount: 4;",
            &writes,
            &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 3);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "guardian.weapon.1.type");
        DOCTEST_CHECK(writes.writes[0].value.integer_value == 11);
        DOCTEST_CHECK(writes.writes[1].canonical_path == "guardian.weapon.1.mods");
        DOCTEST_CHECK(writes.writes[1].value.integer_value == 0x202);
        DOCTEST_CHECK(writes.writes[2].canonical_path == "guardian.weapon.1.amount");
        DOCTEST_CHECK(writes.writes[2].value.integer_value == 4);

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian special = 2: battle_scanner 3: damper_field;",
            &writes,
            &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 2);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "guardian.special.2");
        DOCTEST_CHECK(writes.writes[1].canonical_path == "guardian.special.3");
    }

    DOCTEST_TEST_CASE("labeled groups reject unknown and overlapping targets atomically") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;

        Add_Sentinel_(&writes);
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 = type: plasma_cannon unknown: 4;",
            &writes,
            &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "unknown configuration variable 'guardian.weapon.1.unknown'");
        DOCTEST_CHECK(writes.writes.empty());

        Add_Sentinel_(&writes);
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 = mods: heavy 0 4 amount: 5;",
            &writes,
            &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "labeled assignment writes 'guardian.weapon.1.amount' more than once");
        DOCTEST_CHECK(writes.writes.empty());

        Add_Sentinel_(&writes);
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 = type: plasma_cannon amount: 256;",
            &writes,
            &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "invalid value '256' for 'guardian.weapon.1.amount'; expected uint8 in range 0..255");
        DOCTEST_CHECK(writes.writes.empty());
    }

    DOCTEST_TEST_CASE("build queue lists decode comments multiline values and required end markers") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_(
            "colony build_queue 1 = prepend # put ships first\n"
            "  design1\n"
            "  buy10 end;",
            &writes,
            &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 4);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "colony.build_queue.1.0");
        DOCTEST_CHECK(writes.writes[0].value.integer_value == BUILD_QUEUE::INSTRUCTION_PREPEND);
        DOCTEST_CHECK(writes.writes[1].value.integer_value == BUILD_QUEUE::INSTRUCTION_DESIGN1);
        DOCTEST_CHECK(writes.writes[2].value.integer_value == BUILD_QUEUE::INSTRUCTION_BUY10);
        DOCTEST_CHECK(writes.writes[3].value.integer_value == BUILD_QUEUE::INSTRUCTION_END);

        DOCTEST_REQUIRE(Decode_(
            "colony build_queue 1 199 = end;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 1);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "colony.build_queue.1.199");
        DOCTEST_CHECK(writes.writes[0].value.integer_value == BUILD_QUEUE::INSTRUCTION_END);

        DOCTEST_CHECK_FALSE(Decode_(
            "colony build_queue 1 = prepend design1;", &writes, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "assignment to 'colony.build_queue.1' requires its terminator value");

        DOCTEST_CHECK_FALSE(Decode_(
            "colony build_queue 11 = end;", &writes, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "unknown configuration variable 'colony.build_queue.11'");
    }

    DOCTEST_TEST_CASE("build queue list capacity includes its end marker") {
        std::string source = "colony build_queue 2 = ";
        for (int index = 0; index < BUILD_QUEUE::INSTRUCTION_CAPACITY - 1; ++index) {
            source += "append ";
        }
        source += "end;";

        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_(source.c_str(), &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.size() == BUILD_QUEUE::INSTRUCTION_CAPACITY);

        source.insert(source.size() - 4, "append ");
        DOCTEST_CHECK_FALSE(Decode_(source.c_str(), &writes, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "value 201 exceeds selected configuration object 'colony.build_queue.2'");
    }

    DOCTEST_TEST_CASE("enum-backed array aliases resolve to numeric canonical paths") {
        config::s_config_write_set numeric;
        config::s_config_write_set keyed;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "weapon 11 available_mods = 0x202;", &numeric, &diagnostic));
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "weapon plasma_cannon available_mods = 0x202;", &keyed, &diagnostic));
        DOCTEST_REQUIRE(numeric.writes.size() == 1);
        DOCTEST_REQUIRE(keyed.writes.size() == 1);
        DOCTEST_CHECK(keyed.writes[0].variable_id == numeric.writes[0].variable_id);
        DOCTEST_CHECK(keyed.writes[0].canonical_path == numeric.writes[0].canonical_path);
        DOCTEST_CHECK(keyed.writes[0].canonical_path == "weapon.11.available_mods");
        DOCTEST_CHECK(keyed.writes[0].value.integer_value == numeric.writes[0].value.integer_value);

        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "weapon plasma_cannon available_mods = 1 2;", &keyed, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "value 2 exceeds selected configuration object "
            "'weapon.11.available_mods'");

        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "weapon unknown_weapon available_mods = 1;", &keyed, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "unknown configuration variable 'weapon.unknown_weapon'");
    }

    DOCTEST_TEST_CASE("scalar starts and skips remain within the nearest aggregate") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian weapon 1 mods = heavy,eccm - 4;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 2);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "guardian.weapon.1.mods");
        DOCTEST_CHECK(writes.writes[0].value.integer_value == 0x202);
        DOCTEST_CHECK(writes.writes[1].canonical_path == "guardian.weapon.1.amount");
        DOCTEST_CHECK(writes.writes[1].value.integer_value == 4);

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian special 2 = battle_scanner damper_field;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 2);
        DOCTEST_CHECK(writes.writes[0].canonical_path == "guardian.special.2");
        DOCTEST_CHECK(writes.writes[1].canonical_path == "guardian.special.3");

        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 mods = heavy 0 2 4;", &writes, &diagnostic));
        DOCTEST_CHECK(diagnostic.message ==
            "value 4 exceeds selected configuration object 'guardian.weapon.1.mods'");
        DOCTEST_CHECK(writes.writes.empty());
    }

    DOCTEST_TEST_CASE("semantic scalars decode enums masks strings and normalized ratios") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "guardian label = \"Grüße\" 2/6;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 2);
        DOCTEST_REQUIRE(writes.writes[0].value.string_value.size() == 5);
        DOCTEST_CHECK(static_cast<uint8_t>(writes.writes[0].value.string_value[2]) == 0x81);
        DOCTEST_CHECK(writes.writes[1].value.ratio_numerator == 1);
        DOCTEST_CHECK(writes.writes[1].value.ratio_denominator == 3);

        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 type = 99;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.empty());
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian weapon 1 mods = 0x4000;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.empty());
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "guardian reduction = 1/0;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.empty());
    }

    DOCTEST_TEST_CASE("arbitrary width bitmaps decode named values and flags") {
        config::s_config_write_set writes;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_REQUIRE(Decode_Aggregate_(
            "special_devices = automated_repair,regeneration;", &writes, &diagnostic));
        DOCTEST_REQUIRE(writes.writes.size() == 1);
        DOCTEST_CHECK(writes.writes[0].value.type == config::CONFIG_VALUE_BITMAP);
        DOCTEST_REQUIRE(writes.writes[0].value.bitmap_value.size() == 5);
        DOCTEST_CHECK(writes.writes[0].value.bitmap_value[0] == 0x08);
        DOCTEST_CHECK(writes.writes[0].value.bitmap_value[4] == 0x80);

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "special_devices = self_repairing;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes[0].value.bitmap_value[0] == 0x08);
        DOCTEST_CHECK(writes.writes[0].value.bitmap_value[4] == 0x80);

        DOCTEST_REQUIRE(Decode_Aggregate_(
            "special_devices = none;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes[0].value.bitmap_value ==
            std::vector<uint8_t>({0, 0, 0, 0, 0}));

        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "special_devices = 0x8000000008;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.empty());
        DOCTEST_CHECK_FALSE(Decode_Aggregate_(
            "special_devices = automated_repair,unknown;", &writes, &diagnostic));
        DOCTEST_CHECK(writes.writes.empty());
    }
}