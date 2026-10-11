#include <cstdio>
#include <cstring>
#include <string>

#include "generate_cpp.h"
#include "generate_docs.h"
#include "schema_model.h"

#if defined(ORION2RE_CONFIG_TOOL_LINK_GAME)
#include "config/config_runtime.h"
#endif

namespace {
    void Print_Usage_() {
        std::fprintf(stderr,
            "usage:\n"
            "  config-tool validate <schema.json5>\n"
            "  config-tool generate <schema.json5> <output-directory>\n"
            "  config-tool check <schema.json5> <committed-directory> <temporary-directory>\n"
            "  config-tool docs <schema.json5> <output-directory>\n");
    }

#if defined(ORION2RE_CONFIG_TOOL_LINK_GAME)
    config::e_value_type Expected_Value_Type_(
        const config::tool::s_parameter& parameter) {
        if (parameter.scalar_kind == config::tool::SCALAR_BITMAP) {
            return config::CONFIG_VALUE_BITMAP;
        }
        if (parameter.scalar_kind == config::tool::SCALAR_RATIO) {
            return config::CONFIG_VALUE_RATIO;
        }
        switch (parameter.type) {
            case config::tool::PRIMITIVE_BOOL: return config::CONFIG_VALUE_BOOL;
            case config::tool::PRIMITIVE_INT8: return config::CONFIG_VALUE_INT8;
            case config::tool::PRIMITIVE_UINT8: return config::CONFIG_VALUE_UINT8;
            case config::tool::PRIMITIVE_INT16: return config::CONFIG_VALUE_INT16;
            case config::tool::PRIMITIVE_UINT16: return config::CONFIG_VALUE_UINT16;
            case config::tool::PRIMITIVE_INT32: return config::CONFIG_VALUE_INT32;
            case config::tool::PRIMITIVE_UINT32: return config::CONFIG_VALUE_UINT32;
            case config::tool::PRIMITIVE_STRING: return config::CONFIG_VALUE_STRING;
            case config::tool::PRIMITIVE_BITMAP: return config::CONFIG_VALUE_BITMAP;
            case config::tool::PRIMITIVE_RATIO: return config::CONFIG_VALUE_RATIO;
        }
        return config::CONFIG_VALUE_BOOL;
    }

    void Copy_Default_Value_(config::tool::s_parameter* parameter,
        const config::s_typed_value& value) {
        parameter->default_bool = value.bool_value;
        parameter->default_integer = value.integer_value;
        parameter->default_string =
            config::tool::Decode_Game_String(value.string_value);
        parameter->default_bitmap.clear();
        for (uint8_t byte : value.bitmap_value) {
            parameter->default_bitmap.push_back(byte);
        }
        parameter->default_ratio_numerator = value.ratio_numerator;
        parameter->default_ratio_denominator = value.ratio_denominator;
        parameter->default_from_native = false;
    }

    bool Read_Native_Defaults_(config::tool::s_schema* schema,
        std::string* diagnostic) {
        config::s_config_state defaults;
        if (!config::Initialize_Defaults(&defaults) ||
            defaults.values.size() != schema->parameters.size()) {
            *diagnostic = "generated configuration bindings do not match the schema";
            return false;
        }
        for (size_t index = 0; index < schema->parameters.size(); ++index) {
            config::tool::s_parameter& parameter = schema->parameters[index];
            if (!parameter.default_from_native) {
                continue;
            }
            std::string runtime_name;
            const config::s_typed_value& value = defaults.values[index];
            if (!config::Get_Variable_Name(static_cast<config::t_variable_id>(index),
                    &runtime_name) ||
                runtime_name != parameter.path) {
                *diagnostic = parameter.path +
                    ": generated configuration binding does not match the schema";
                return false;
            }
            if (parameter.aggregate) {
                if (value.type != config::CONFIG_VALUE_AGGREGATE ||
                    value.aggregate_values.size() != parameter.aggregate_leaves.size()) {
                    *diagnostic = parameter.path +
                        ": generated aggregate binding does not match the schema";
                    return false;
                }
                const config::t_variable_id variable_id =
                    static_cast<config::t_variable_id>(index);
                for (size_t leaf_index = 0;
                     leaf_index < parameter.aggregate_leaves.size();
                     ++leaf_index) {
                    config::tool::s_parameter& leaf =
                        parameter.aggregate_leaves[leaf_index];
                    const config::s_typed_value& leaf_value =
                        value.aggregate_values[leaf_index];
                    std::string runtime_leaf_name;
                    if (!config::Get_Variable_Leaf_Name(
                            variable_id,
                            static_cast<uint32_t>(leaf_index),
                            &runtime_leaf_name) ||
                        runtime_leaf_name != leaf.path ||
                        leaf_value.type != Expected_Value_Type_(leaf)) {
                        *diagnostic = leaf.path +
                            ": generated aggregate field binding does not match the schema";
                        return false;
                    }
                    Copy_Default_Value_(&leaf, leaf_value);
                }
                parameter.default_from_native = false;
                continue;
            }
            if (value.type != Expected_Value_Type_(parameter)) {
                *diagnostic = parameter.path +
                    ": generated configuration binding does not match the schema";
                return false;
            }
            Copy_Default_Value_(&parameter, value);
        }
        return true;
    }
#endif
} // namespace

int main(int argc, char** argv) {
    const bool validate = argc == 3 && std::strcmp(argv[1], "validate") == 0;
    const bool generate = argc == 4 && std::strcmp(argv[1], "generate") == 0;
    const bool check = argc == 5 && std::strcmp(argv[1], "check") == 0;
    const bool docs = argc == 4 && std::strcmp(argv[1], "docs") == 0;
    if (!validate && !generate && !check && !docs) {
        Print_Usage_();
        return 2;
    }

    config::tool::s_schema schema;
    std::string diagnostic;
    if (!config::tool::Load_And_Validate_Schema(argv[2], &schema, &diagnostic)) {
        std::fprintf(stderr, "%s\n", diagnostic.c_str());
        return 1;
    }

    bool success = true;
    if (generate) {
        success = config::tool::Generate_Cpp(&schema, argv[3], &diagnostic);
    } else if (check) {
        success = config::tool::Check_Generated_Cpp(&schema, argv[3], argv[4], &diagnostic);
    } else if (docs) {
#if defined(ORION2RE_CONFIG_TOOL_LINK_GAME)
        success = Read_Native_Defaults_(&schema, &diagnostic) &&
            config::tool::Generate_Documentation(&schema, argv[3], &diagnostic);
        config::Shutdown_Runtime();
#else
        diagnostic = "documentation generation requires config-tool linked against the game";
        success = false;
#endif
    }

    if (!success) {
        std::fprintf(stderr, "%s\n", diagnostic.c_str());
        return 1;
    }
    return 0;
}