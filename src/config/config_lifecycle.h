#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/cfg_parser.h"
#include "config/config_runtime.h"

namespace config {

    struct s_value_provenance {
        bool has_override = false;
        std::string package_id;
        s_source_location location;
        uint32_t statement = 0;
    };

    struct s_prior_value {
        uint32_t leaf_index = 0;
        s_typed_value value;
        s_value_provenance provenance;
    };

    struct s_effective_config {
        s_config_state state;
        s_config_values<s_value_provenance> provenance;
        s_config_values<std::vector<s_prior_value>> prior_values;
    };

    struct s_config_delta {
        std::vector<uint64_t> variables;
        std::vector<t_variable_id> variable_ids;

        struct s_aggregate {
            t_variable_id variable_id = CONFIG_NO_VARIABLE;
            std::vector<uint64_t> leaves;
            bool vector_length = false;
        };
        std::vector<s_aggregate> aggregates;
    };

    struct s_config_baseline {
        s_effective_config effective;
    };

    struct s_config_stage {
        const s_config_baseline* baseline = nullptr;
        s_effective_config effective;
        s_config_delta touched;
    };

    bool Initialize_Effective_Config(s_effective_config* effective);
    bool Read_Effective_Native_Config(s_effective_config* effective);
    bool Freeze_Config_Baseline(s_effective_config* effective,
        s_config_baseline* baseline);
    bool Begin_Config_Stage(const s_config_baseline* baseline,
        s_config_stage* stage);
    bool Begin_Default_Config_Stage(s_config_stage* stage);
    bool Rollback_Config_Stage(s_config_stage* stage);
    bool Freeze_Config_Stage(s_config_stage* stage,
        s_config_baseline* baseline);
    bool Finish_Config_Stage(s_config_stage* stage,
        s_effective_config* effective);
    bool Config_Delta_Contains(const s_config_delta* delta,
        t_variable_id variable_id,
        uint32_t leaf_index);
    bool Apply_Config_State_To_Stage(
        const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state,
        s_config_stage* stage);
    bool Apply_Config_Document_To_Stage(const s_cfg_document* document,
        const char* package_id,
        s_config_stage* stage,
        s_cfg_diagnostic* diagnostic);
    bool Apply_Config_Document_To_Candidate(const s_cfg_document* document,
        const char* package_id,
        s_effective_config* candidate,
        s_cfg_diagnostic* diagnostic);
    bool Apply_Config_Document(const s_cfg_document* document, const char* package_id, s_effective_config* effective, s_cfg_diagnostic* diagnostic);
    bool Merge_Config_Group(t_config_group group, const s_effective_config* candidate, s_effective_config* effective);
    bool Apply_Config_Group(t_config_group group, const s_effective_config* candidate, s_effective_config* effective);

} // namespace config