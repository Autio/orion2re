#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/cfg_parser.h"
#include "config/config_runtime.h"

namespace config {

    struct s_config_write {
        t_variable_id variable_id = 0;
        uint32_t leaf_index = 0;
        std::string canonical_path;
        s_typed_value value;
        s_source_location provenance;
        std::string package_id;
    };

    struct s_config_write_set {
        std::vector<s_config_write> writes;
    };

    bool Decode_Assignment(const s_cfg_statement* statement, s_config_write_set* write_set, s_cfg_diagnostic* diagnostic);
    bool Decode_Assignment_With_Schema(const s_cfg_statement* statement,
        const s_runtime_schema* schema,
        s_config_write_set* write_set,
        s_cfg_diagnostic* diagnostic);

} // namespace config