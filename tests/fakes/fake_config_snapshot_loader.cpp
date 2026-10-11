#include "pch.h"

#include "config/config_loader.h"
#include "config/config_snapshot.h"

namespace TEST_CONFIG_SNAPSHOT {
    int commit_count;
    config::s_config_state committed_state;
    std::vector<config::s_mod_requirement> required_mods;
    config::s_active_config active_config;
    bool reapply_succeeds = true;
    std::string reapply_error;
} // namespace TEST_CONFIG_SNAPSHOT

namespace config {

    bool Commit_Config_Snapshot(const s_config_snapshot_descriptor*,
        const s_config_state* state,
        s_cfg_diagnostic*) {
        ++TEST_CONFIG_SNAPSHOT::commit_count;
        TEST_CONFIG_SNAPSHOT::committed_state = *state;
        return true;
    }

    bool Reapply_Config_Snapshot(const std::vector<s_mod_requirement>& required_mods,
        const s_config_snapshot_descriptor*,
        const s_config_state* state,
        s_cfg_diagnostic* diagnostic,
        const orion2re::lua::s_lua_save_snapshot*) {
        if (!TEST_CONFIG_SNAPSHOT::reapply_succeeds) {
            diagnostic->message = TEST_CONFIG_SNAPSHOT::reapply_error;
            return false;
        }
        ++TEST_CONFIG_SNAPSHOT::commit_count;
        TEST_CONFIG_SNAPSHOT::committed_state = *state;
        TEST_CONFIG_SNAPSHOT::required_mods = required_mods;
        return true;
    }

    const s_active_config* Get_Active_Config() {
        return &TEST_CONFIG_SNAPSHOT::active_config;
    }

    bool Get_Prepared_Active_Config_Snapshot(
        const s_config_snapshot_descriptor* descriptor,
        const s_prepared_config_snapshot** snapshot,
        s_cfg_diagnostic*) {
        static s_prepared_config_snapshot prepared;
        if (snapshot == nullptr ||
            !Prepare_Config_Snapshot(descriptor,
                &TEST_CONFIG_SNAPSHOT::active_config.baseline.effective.state,
                &TEST_CONFIG_SNAPSHOT::active_config.required_mods,
                &prepared)) {
            return false;
        }
        *snapshot = &prepared;
        return true;
    }

    bool Get_Canonical_Active_Config_Identity(
        const std::vector<uint8_t>** identity,
        s_cfg_diagnostic*) {
        static const std::vector<uint8_t> serialization = {'O', '2', 'C', 'I'};
        if (identity == nullptr) return false;
        *identity = &serialization;
        return true;
    }

} // namespace config