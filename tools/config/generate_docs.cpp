#include "generate_docs.h"

#include <algorithm>
#include <filesystem>

#include "file_util.h"

namespace config::tool {
    namespace {
        std::string File_Name_(std::string id) {
            std::replace(id.begin(), id.end(), '_', '-');
            return id;
        }

        std::string Text_(const std::string& text) {
            std::string escaped;
            escaped.reserve(text.size());
            for (char character : text) {
                if (character == '\\' || character == '[' || character == ']' ||
                    character == '*' || character == '_' || character == '<' ||
                    character == '>') {
                    escaped += '\\';
                }
                escaped += character;
            }
            return escaped;
        }

        std::string Code_(const std::string& text) {
            std::string escaped;
            escaped.reserve(text.size());
            for (char character : text) {
                escaped += character == '`' ? "&#96;" : std::string(1, character);
            }
            return "`" + escaped + "`";
        }

        std::string Parameter_Path_(const s_parameter_declaration& parameter) {
            return parameter.name_space.empty() ? parameter.id : parameter.name_space + " " + parameter.id;
        }

        std::string Parameter_Prefix_(const s_parameter_declaration& parameter) {
            return parameter.name_space.empty() ? parameter.id : parameter.name_space + "." + parameter.id;
        }

        std::string Anchor_(const char* kind, const std::string& id) {
            return std::string(kind) + "-" + File_Name_(id);
        }

        const s_group* Group_(const s_schema* schema, const std::string& id) {
            for (const s_group& group : schema->groups) {
                if (group.id == id) {
                    return &group;
                }
            }
            return nullptr;
        }

        uint32_t Parameter_Node_(const s_schema* schema, size_t parameter_index) {
            for (size_t index = 0; index < schema->nodes.size(); ++index) {
                if (schema->nodes[index].parameter == parameter_index) {
                    return static_cast<uint32_t>(index);
                }
            }
            return UINT32_MAX;
        }

        std::string Cfg_Node_Path_(const s_schema* schema, uint32_t node_index) {
            std::vector<std::string> parts;
            while (node_index != UINT32_MAX) {
                const s_node& node = schema->nodes[node_index];
                if (!node.id.empty()) {
                    parts.push_back(node.id);
                } else if (node.parent != UINT32_MAX) {
                    const s_node& array = schema->nodes[node.parent];
                    std::string selector = std::to_string(node.external_index);
                    for (const s_symbol& alias : array.index_aliases) {
                        if (alias.value == node.external_index) {
                            selector = alias.name;
                            break;
                        }
                    }
                    parts.push_back(selector);
                }
                node_index = node.parent;
            }
            std::reverse(parts.begin(), parts.end());
            std::string result;
            for (const std::string& part : parts) {
                result += result.empty() ? part : " " + part;
            }
            return result;
        }

        std::string Cfg_Path_(const s_schema* schema, size_t parameter_index) {
            const uint32_t node = Parameter_Node_(schema, parameter_index);
            if (node != UINT32_MAX) {
                return Cfg_Node_Path_(schema, node);
            }
            std::string result = schema->parameters[parameter_index].path;
            std::replace(result.begin(), result.end(), '.', ' ');
            return result;
        }

        std::string Cfg_String_(const std::string& value) {
            std::string result = "\"";
            for (char character : value) {
                switch (character) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += character; break;
                }
            }
            return result + "\"";
        }

        std::string Symbolic_Integer_(const s_parameter& parameter) {
            for (const s_symbol& symbol : parameter.values) {
                if (symbol.value == parameter.default_integer) {
                    return symbol.name;
                }
            }
            if (parameter.scalar_kind != SCALAR_MASK) {
                return std::to_string(parameter.default_integer);
            }
            uint64_t remaining = static_cast<uint64_t>(parameter.default_integer);
            std::string result;
            for (const s_symbol& flag : parameter.flags) {
                const uint64_t value = static_cast<uint64_t>(flag.value);
                if (value != 0 && (remaining & value) == value) {
                    result += result.empty() ? flag.name : "," + flag.name;
                    remaining &= ~value;
                }
            }
            return remaining == 0 && !result.empty() ? result : std::to_string(parameter.default_integer);
        }

        std::string Symbolic_Bitmap_(const s_parameter& parameter) {
            for (const s_bitmap_symbol& symbol : parameter.bitmap_values) {
                if (symbol.value == parameter.default_bitmap) {
                    return symbol.name;
                }
            }
            std::vector<uint8_t> remaining = parameter.default_bitmap;
            std::string result;
            for (const s_bitmap_symbol& flag : parameter.bitmap_flags) {
                bool contained = flag.value.size() == remaining.size();
                bool nonzero = false;
                for (size_t index = 0; contained && index < remaining.size(); ++index) {
                    contained = (remaining[index] & flag.value[index]) == flag.value[index];
                    nonzero = nonzero || flag.value[index] != 0;
                }
                if (contained && nonzero) {
                    result += result.empty() ? flag.name : "," + flag.name;
                    for (size_t index = 0; index < remaining.size(); ++index) {
                        remaining[index] &= static_cast<uint8_t>(~flag.value[index]);
                    }
                }
            }
            const bool empty = std::all_of(remaining.begin(), remaining.end(), [](uint8_t byte) { return byte == 0; });
            return empty && !result.empty() ? result : "none";
        }

        std::string Default_Value_(const s_parameter& parameter) {
            switch (parameter.scalar_kind) {
                case SCALAR_ENUM:
                case SCALAR_MASK: return Symbolic_Integer_(parameter);
                case SCALAR_BITMAP: return Symbolic_Bitmap_(parameter);
                case SCALAR_RATIO:
                    return std::to_string(parameter.default_ratio_numerator) + "/" +
                        std::to_string(parameter.default_ratio_denominator);
                case SCALAR_PRIMITIVE:
                    if (parameter.type == PRIMITIVE_BOOL) {
                        return parameter.default_bool ? "true" : "false";
                    }
                    if (parameter.type == PRIMITIVE_STRING) {
                        return Cfg_String_(parameter.default_string);
                    }
                    return std::to_string(parameter.default_integer);
            }
            return {};
        }

        bool Leaf_Record_(const s_schema* schema, uint32_t node_index) {
            if (schema->nodes[node_index].kind != NODE_RECORD) {
                return false;
            }
            bool has_fields = false;
            for (const s_node& node : schema->nodes) {
                if (node.parent == node_index) {
                    has_fields = true;
                    if (node.kind != NODE_SCALAR) {
                        return false;
                    }
                }
            }
            return has_fields;
        }

        bool Compact_Node_(const s_schema* schema, uint32_t node_index) {
            if (Leaf_Record_(schema, node_index)) {
                return true;
            }
            if (schema->nodes[node_index].kind != NODE_ARRAY &&
                schema->nodes[node_index].kind != NODE_MAP &&
                schema->nodes[node_index].kind != NODE_VECTOR) {
                return false;
            }
            bool has_elements = false;
            for (size_t index = 0; index < schema->nodes.size(); ++index) {
                const s_node& node = schema->nodes[index];
                if (node.parent == node_index) {
                    has_elements = true;
                    if (node.kind != NODE_SCALAR &&
                        !Leaf_Record_(schema, static_cast<uint32_t>(index))) {
                        return false;
                    }
                }
            }
            return has_elements;
        }

        std::string Default_Configuration_(const s_schema* schema,
            const s_parameter_declaration& declaration) {
            const auto append_parameter_defaults = [](const s_parameter& parameter,
                                                       size_t first_leaf,
                                                       size_t leaf_count,
                                                       bool* has_value,
                                                       std::string* output) {
                if (!parameter.aggregate) {
                    *output += (*has_value ? " " : "") + Default_Value_(parameter);
                    *has_value = true;
                    return;
                }
                const size_t end = std::min(
                    parameter.aggregate_leaves.size(), first_leaf + leaf_count);
                for (size_t leaf = first_leaf; leaf < end; ++leaf) {
                    *output += (*has_value ? " " : "") +
                        Default_Value_(parameter.aggregate_leaves[leaf]);
                    *has_value = true;
                }
            };
            const std::string prefix = Parameter_Prefix_(declaration);
            std::string defaults;
            size_t parameter_index = 0;
            while (parameter_index < schema->parameters.size()) {
                const s_parameter& parameter = schema->parameters[parameter_index];
                if (parameter.path != prefix && !parameter.path.starts_with(prefix + ".")) {
                    ++parameter_index;
                    continue;
                }
                uint32_t assignment_node = Parameter_Node_(schema, parameter_index);
                while (assignment_node != UINT32_MAX) {
                    const uint32_t parent = schema->nodes[assignment_node].parent;
                    if (parent == UINT32_MAX || !Compact_Node_(schema, parent) ||
                        schema->nodes[parent].first_parameter != parameter_index) {
                        break;
                    }
                    assignment_node = parent;
                }
                size_t last_parameter = assignment_node == UINT32_MAX ? parameter_index : schema->nodes[assignment_node].last_parameter;
                while (last_parameter > parameter_index &&
                    (last_parameter >= schema->parameters.size() ||
                        (schema->parameters[last_parameter].path != prefix &&
                            !schema->parameters[last_parameter].path.starts_with(prefix + ".")))) {
                    --last_parameter;
                }
                const std::string path = assignment_node == UINT32_MAX ? Cfg_Path_(schema, parameter_index) : Cfg_Node_Path_(schema, assignment_node);
                if (assignment_node != UINT32_MAX &&
                    (schema->nodes[assignment_node].kind == NODE_ARRAY ||
                        schema->nodes[assignment_node].kind == NODE_MAP ||
                        schema->nodes[assignment_node].kind == NODE_VECTOR)) {
                    const s_node& array = schema->nodes[assignment_node];
                    const size_t count = array.implicit_terminator ? array.explicit_default_count : static_cast<size_t>(array.array_count);
                    if (count == 0) {
                        parameter_index = last_parameter + 1;
                        continue;
                    }
                    defaults += path + " =\n";
                    size_t child_index = 0;
                    for (const s_node& child : schema->nodes) {
                        if (child.parent != assignment_node || child_index >= count) {
                            continue;
                        }
                        defaults += "    ";
                        bool has_value = false;
                        bool terminated = false;
                        for (size_t index = child.first_parameter;
                             index <= child.last_parameter;
                             ++index) {
                            const s_parameter& parameter = schema->parameters[index];
                            const size_t first_leaf = array.kind == NODE_VECTOR ? child.first_leaf : 0;
                            const size_t leaf_count = parameter.aggregate ? parameter.element_leaf_count : 1;
                            append_parameter_defaults(
                                parameter, first_leaf, leaf_count, &has_value, &defaults);
                            if (array.has_terminator && !array.implicit_terminator &&
                                child.kind == NODE_SCALAR) {
                                const s_parameter& value = parameter.aggregate
                                    ? parameter.aggregate_leaves[first_leaf]
                                    : parameter;
                                terminated = value.default_integer == array.terminator;
                            }
                        }
                        ++child_index;
                        defaults += terminated || child_index == count ? ";\n" : "\n";
                        if (terminated) {
                            break;
                        }
                    }
                } else {
                    defaults += path + " = ";
                    bool has_value = false;
                    for (size_t index = parameter_index; index <= last_parameter; ++index) {
                        const s_parameter& parameter = schema->parameters[index];
                        append_parameter_defaults(parameter, 0, parameter.aggregate ? parameter.aggregate_leaves.size() : 1, &has_value, &defaults);
                    }
                    defaults += ";\n";
                }
                parameter_index = last_parameter + 1;
            }
            return defaults;
        }

        const s_type_declaration* Type_(const s_schema* schema, const std::string& id) {
            for (const s_type_declaration& declaration : schema->type_declarations) {
                if (declaration.id == id) {
                    return &declaration;
                }
            }
            return nullptr;
        }

        const s_record_declaration* Record_(const s_schema* schema, const std::string& id) {
            for (const s_record_declaration& declaration : schema->record_declarations) {
                if (declaration.id == id) {
                    return &declaration;
                }
            }
            return nullptr;
        }

        std::string Type_Link_(const s_schema* schema, const s_documented_type& type, const char* prefix) {
            if (!type.reference.empty()) {
                if (const s_type_declaration* declaration = Type_(schema, type.reference);
                    declaration != nullptr && !declaration->internal) {
                    return "[" + Text_(declaration->metadata.title) + "](" + prefix +
                        "allowed-values.md#" + Anchor_("type", declaration->id) + ")";
                }
                if (const s_record_declaration* declaration = Record_(schema, type.reference);
                    declaration != nullptr && !declaration->internal) {
                    return "[" + Text_(declaration->metadata.title) + "](" + prefix +
                        "record-layouts.md#" + Anchor_("record", declaration->id) + ")";
                }
            }
            if (type.node_kind == NODE_ARRAY) {
                return std::to_string(type.array_count) + " entries";
            }
            if (type.node_kind == NODE_MAP) {
                return std::to_string(type.array_count) + " keyed entries";
            }
            if (type.node_kind == NODE_VECTOR) {
                return "zero-based dynamically sized entries";
            }
            if (type.node_kind == NODE_RECORD) {
                return "named fields";
            }
            switch (type.scalar_kind) {
                case SCALAR_ENUM: return "one listed choice";
                case SCALAR_MASK:
                case SCALAR_BITMAP: return "one or more listed options";
                case SCALAR_RATIO: return "a ratio";
                case SCALAR_PRIMITIVE:
                    if (type.primitive == PRIMITIVE_BOOL) {
                        return Code_("true") + " or " + Code_("false");
                    }
                    return type.primitive == PRIMITIVE_STRING ? "text" : "a number";
            }
            return "a value";
        }

        void Constraints_(const s_documented_type& type, std::string* output) {
            const s_constraints& constraints = type.constraints;
            if (constraints.has_min_length || constraints.has_max_length) {
                *output += "- **Accepted length:** ";
                if (constraints.has_min_length && constraints.has_max_length) {
                    *output += std::to_string(constraints.min_length) + " to " +
                        std::to_string(constraints.max_length) + " characters\n";
                } else if (constraints.has_min_length) {
                    *output += "at least " + std::to_string(constraints.min_length) +
                        " characters\n";
                } else {
                    *output += "up to " + std::to_string(constraints.max_length) +
                        " characters\n";
                }
            } else if (constraints.has_min || constraints.has_max) {
                *output += "- **Accepted values:** ";
                if (constraints.has_min) {
                    *output += std::to_string(constraints.min);
                } else {
                    *output += "type minimum";
                }
                *output += " to ";
                if (constraints.has_max) {
                    *output += std::to_string(constraints.max);
                } else {
                    *output += "type maximum";
                }
                *output += "\n";
            }
        }

        void Symbols_(const char* heading, const std::vector<s_symbol>& symbols, std::string* output) {
            if (symbols.empty()) {
                return;
            }
            *output += "\n#### " + std::string(heading) + "\n\n";
            for (const s_symbol& symbol : symbols) {
                *output += "- " + Code_(symbol.name);
                if (!symbol.description.empty()) {
                    *output += ": " + Text_(symbol.description);
                }
                *output += "\n";
            }
        }

        void Bitmap_Symbols_(const char* heading,
            const std::vector<s_bitmap_symbol>& symbols,
            std::string* output) {
            if (symbols.empty()) {
                return;
            }
            *output += "\n#### " + std::string(heading) + "\n\n";
            for (const s_bitmap_symbol& symbol : symbols) {
                *output += "- " + Code_(symbol.name);
                if (!symbol.description.empty()) {
                    *output += ": " + Text_(symbol.description);
                }
                *output += "\n";
            }
        }

        void Type_Details_(const s_schema* schema, const s_documented_type& type, const char* prefix, std::string* output) {
            *output += "- **Accepted input:** " + Type_Link_(schema, type, prefix) + "\n";
            Constraints_(type, output);
            if ((type.node_kind == NODE_ARRAY || type.node_kind == NODE_MAP) &&
                !type.array_index_type.empty()) {
                s_documented_type index_type;
                index_type.reference = type.array_index_type;
                *output += "- **Entry names:** " + Type_Link_(schema, index_type, prefix) + "\n";
            }
            Symbols_("Allowed choices", type.values, output);
            Symbols_("Available flags", type.flags, output);
            Bitmap_Symbols_("Allowed choices", type.bitmap_values, output);
            Bitmap_Symbols_("Available flags", type.bitmap_flags, output);
        }

        void Examples_(const s_metadata& metadata, std::string* output) {
            if (metadata.examples.empty()) {
                return;
            }
            *output += "\n#### Examples\n\n```cfg\n";
            for (const std::string& example : metadata.examples) {
                *output += example + "\n";
            }
            *output += "```\n";
        }

        void Fields_(const s_schema* schema, const std::vector<s_documented_field>& fields, const char* prefix, std::string* output) {
            *output += "\n#### Fields\n\n";
            for (const s_documented_field& field : fields) {
                if (field.internal) {
                    continue;
                }
                *output += "- " + Code_(field.id);
                if (!field.metadata.title.empty()) {
                    *output += " - **" + Text_(field.metadata.title) + "**";
                }
                if (!field.metadata.description.empty()) {
                    *output += ": " + Text_(field.metadata.description);
                }
                if (field.type != nullptr) {
                    *output += " Accepted input: " + Type_Link_(schema, *field.type, prefix) + ".";
                }
                if (field.has_default) {
                    *output += " Default: " + Code_(field.default_value) + ".";
                }
                if (!field.group.empty()) {
                    const s_group* group = Group_(schema, field.group);
                    if (group != nullptr) {
                        *output += group->save
                            ? " Saved in saved games."
                            : " Local and not saved.";
                        *output += group->synchronize
                            ? " Synchronized and must match."
                            : " May differ between players.";
                    }
                }
                *output += "\n";
            }
        }

        std::string Reference_Link_(const s_schema* schema,
            const s_documentation_reference& reference) {
            if (reference.anchor.starts_with("parameter-")) {
                for (const s_parameter_declaration& parameter : schema->parameter_declarations) {
                    if (reference.anchor == "parameter-" + parameter.id) {
                        return File_Name_(parameter.documentation_category) + ".md#" +
                            Anchor_("parameter", parameter.id);
                    }
                }
            }
            if (reference.anchor.starts_with("type-")) {
                return "allowed-values.md#" + File_Name_(reference.anchor);
            }
            return "record-layouts.md#" + File_Name_(reference.anchor);
        }

        void References_(const s_schema* schema,
            const std::vector<s_documentation_reference>& references,
            std::string* output) {
            if (references.empty()) {
                return;
            }
            *output += "\n#### Used by\n\n";
            for (const s_documentation_reference& reference : references) {
                *output += "- [" + Text_(reference.title) + "](" +
                    Reference_Link_(schema, reference) + ")\n";
            }
        }

        std::vector<s_documentation_category> Categories_(const s_schema* schema) {
            if (!schema->documentation_categories.empty()) {
                return schema->documentation_categories;
            }
            std::vector<s_documentation_category> categories;
            for (const s_group& group : schema->groups) {
                categories.push_back({group.id, group.metadata, {}});
            }
            return categories;
        }

        bool Write_(const std::filesystem::path& path, const std::string& content, std::string* diagnostic) {
            return Write_File_If_Different(path.string().c_str(), content, diagnostic);
        }
    } // namespace

    bool Generate_Documentation(const s_schema* schema, const char* output_path, std::string* diagnostic) {
        if (schema == nullptr || output_path == nullptr || diagnostic == nullptr) {
            return false;
        }
        for (const s_parameter& parameter : schema->parameters) {
            if (parameter.default_from_native) {
                *diagnostic = parameter.path + ": native default value was not loaded";
                return false;
            }
        }

        const std::filesystem::path output_directory(output_path);
        std::error_code error;
        std::filesystem::create_directories(output_directory, error);
        if (error) {
            *diagnostic = output_directory.string() +
                ": unable to create documentation directory";
            return false;
        }

        const std::vector<s_documentation_category> categories = Categories_(schema);
        std::string index = "# Configuration reference\n\n"
                            "Current supported configuration settings, grouped by subsystem. "
                            "The exact paths and built-in defaults are generated from the runtime schema.\n\n";
        for (const s_documentation_category& category : categories) {
            index += "- [" + Text_(category.metadata.title) + "](" +
                File_Name_(category.id) + ".md): " +
                Text_(category.metadata.description) + "\n";
        }
        index += "- [Allowed values](allowed-values.md)\n"
                 "- [Record layouts](record-layouts.md)\n";
        if (!Write_(output_directory / "index.md", index, diagnostic)) {
            return false;
        }

        for (const s_documentation_category& category : categories) {
            std::string page = "# Configuration: " + Text_(category.metadata.title) +
                "\n\n" +
                Text_(category.metadata.description) + "\n\n"
                                                       "## Settings index\n\n";
            size_t setting_count = 0;
            for (const s_parameter_declaration& parameter : schema->parameter_declarations) {
                if (parameter.documentation_category == category.id) {
                    ++setting_count;
                    page += "- [" + Text_(parameter.metadata.title) + "](#" +
                        Anchor_("parameter", parameter.id) + ") - " +
                        Code_(Parameter_Path_(parameter)) + "\n";
                }
            }
            if (setting_count == 0) {
                page += "No settings are currently declared in this category.\n";
            }

            std::string current_namespace;
            for (const s_parameter_declaration& parameter : schema->parameter_declarations) {
                if (parameter.documentation_category != category.id) {
                    continue;
                }
                if (parameter.name_space != current_namespace) {
                    current_namespace = parameter.name_space;
                    page += "\n## " + Text_(current_namespace.empty() ? std::string("General") : current_namespace) + "\n";
                }
                page += "\n<a id=\"" + Anchor_("parameter", parameter.id) +
                    "\"></a>\n### " + Text_(parameter.metadata.title) + "\n\n" +
                    Text_(parameter.metadata.description) + "\n\n";
                if (!parameter.metadata.remarks.empty()) {
                    page += "**Notes:** " + Text_(parameter.metadata.remarks) + "\n\n";
                }
                page += "- **Configuration path:** " + Code_(Parameter_Path_(parameter)) + "\n";
                std::vector<std::string> lifecycle_groups;
                const std::string parameter_prefix = Parameter_Prefix_(parameter);
                for (const s_parameter& leaf : schema->parameters) {
                    if (leaf.path != parameter_prefix &&
                        !leaf.path.starts_with(parameter_prefix + ".")) {
                        continue;
                    }
                    if (std::find(lifecycle_groups.begin(), lifecycle_groups.end(), leaf.group) == lifecycle_groups.end()) {
                        lifecycle_groups.push_back(leaf.group);
                    }
                }
                const s_group* group = lifecycle_groups.size() == 1
                    ? Group_(schema, lifecycle_groups.front())
                    : nullptr;
                if (lifecycle_groups.size() > 1) {
                    page += "- **Persistence:** varies by field\n";
                    page += "- **Multiplayer:** varies by field\n";
                } else if (group != nullptr) {
                    page += "- **Persistence:** " +
                        std::string(group->save ? "saved in saved games" : "local and not saved") + "\n";
                    page += "- **Multiplayer:** " +
                        std::string(group->synchronize ? "synchronized and must match" : "may differ between players") + "\n";
                }
                if (!parameter.metadata.units.empty()) {
                    page += "- **Units:** " + Text_(parameter.metadata.units) + "\n";
                }
                Type_Details_(schema, parameter.type, "", &page);
                if (parameter.type.node_kind == NODE_RECORD) {
                    Fields_(schema, parameter.type.fields, "", &page);
                }
                if (parameter.include_default_configuration) {
                    const std::string defaults = Default_Configuration_(schema, parameter);
                    if (!defaults.empty()) {
                        page += "\n#### Built-in default configuration\n\n```cfg\n" +
                            defaults + "```\n";
                    }
                }
                Examples_(parameter.metadata, &page);
            }
            if (!Write_(output_directory / (File_Name_(category.id) + ".md"),
                    page,
                    diagnostic)) {
                return false;
            }
        }

        std::string types = "# Configuration: Allowed values\n\n"
                            "Names accepted by settings with enumerated, mask, or bitmap values.\n";
        size_t public_types = 0;
        for (const s_type_declaration& declaration : schema->type_declarations) {
            if (declaration.internal) {
                continue;
            }
            ++public_types;
            types += "\n<a id=\"" + Anchor_("type", declaration.id) +
                "\"></a>\n## " + Text_(declaration.metadata.title) + "\n\n" +
                Text_(declaration.metadata.description) + "\n\n";
            if (!declaration.metadata.remarks.empty()) {
                types += "**Notes:** " + Text_(declaration.metadata.remarks) + "\n\n";
            }
            Type_Details_(schema, declaration.type, "", &types);
            Examples_(declaration.metadata, &types);
            References_(schema, declaration.used_by, &types);
        }
        if (public_types == 0) {
            types += "\nThere are no public allowed-value declarations.\n";
        }
        if (!Write_(output_directory / "allowed-values.md", types, diagnostic)) {
            return false;
        }

        std::string records = "# Configuration: Record layouts\n\n"
                              "Named field layouts used by structured configuration settings.\n";
        size_t public_records = 0;
        for (const s_record_declaration& declaration : schema->record_declarations) {
            if (declaration.internal) {
                continue;
            }
            ++public_records;
            records += "\n<a id=\"" + Anchor_("record", declaration.id) +
                "\"></a>\n## " + Text_(declaration.metadata.title) + "\n\n" +
                Text_(declaration.metadata.description) + "\n";
            if (!declaration.metadata.remarks.empty()) {
                records += "\n**Notes:** " + Text_(declaration.metadata.remarks) + "\n";
            }
            Fields_(schema, declaration.fields, "", &records);
            Examples_(declaration.metadata, &records);
            References_(schema, declaration.used_by, &records);
        }
        if (public_records == 0) {
            records += "\nThere are no public record-layout declarations.\n";
        }
        return Write_(output_directory / "record-layouts.md", records, diagnostic);
    }

} // namespace config::tool