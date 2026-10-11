#include "config/config_loader.h"
#include "config/config_snapshot.h"

#include "lua/lua_registry.h"

namespace config {

    namespace {
        bool reconstruct_result = true;
        int reconstruct_calls = 0;
    } // namespace

    struct s_network_config_candidate {};
    const s_config_snapshot_descriptor synchronized_config_snapshot = {};

    const s_active_config* Get_Active_Config() {
        return nullptr;
    }

    bool Get_Prepared_Active_Config_Snapshot(
        const s_config_snapshot_descriptor*,
        const s_prepared_config_snapshot**,
        s_cfg_diagnostic*) {
        return false;
    }

    bool Read_Current_State(s_config_state*) {
        return false;
    }

    bool Find_Variable(const char*, size_t, t_variable_id* variable_id) {
        if (variable_id == nullptr) return false;
        *variable_id = 0;
        return true;
    }

    bool State_Tracks_Non_Default_Values(const s_config_state*) {
        return false;
    }

    bool Record_Active_Native_Config_Write(t_variable_id) {
        return true;
    }

    bool Prepare_Config_Snapshot(const s_config_snapshot_descriptor*,
        const s_config_state*,
        const std::vector<s_mod_requirement>*,
        s_prepared_config_snapshot*) {
        return false;
    }

    uint32_t Config_Snapshot_Size(const s_config_snapshot_descriptor*,
        const s_config_state*,
        const std::vector<s_mod_requirement>*) {
        return 0;
    }

    void Write_Config_Snapshot(SERIAL::Writer_*,
        const s_config_snapshot_descriptor*,
        const s_config_state*,
        const std::vector<s_mod_requirement>*) {
    }

    bool Stage_Network_Config_Snapshot(const void*,
        uint32_t,
        s_network_config_candidate**,
        s_cfg_diagnostic*) {
        return false;
    }

    const orion2re::lua::s_lua_registry* Get_Network_Config_Candidate_Registry(
        const s_network_config_candidate*) {
        return nullptr;
    }

    bool Finalize_Network_Config_Snapshot(
        s_network_config_candidate**,
        const void*,
        uint32_t,
        std::string*) {
        return false;
    }

    bool Commit_Network_Config_Snapshot(
        s_network_config_candidate**,
        s_cfg_diagnostic*) {
        return false;
    }

    void Discard_Network_Config_Snapshot(
        s_network_config_candidate**) {
    }

    bool Reconstruct_Active_Lua_Runtimes(std::string* diagnostic) {
        ++reconstruct_calls;
        if (!reconstruct_result && diagnostic != nullptr) {
            *diagnostic = "reconstructed Lua identity differs";
        }
        return reconstruct_result;
    }

    void Reset_Lua_Reconstruction_Fake() {
        reconstruct_result = true;
        reconstruct_calls = 0;
    }

    void Set_Lua_Reconstruction_Result(bool result) {
        reconstruct_result = result;
    }

    int Lua_Reconstruction_Calls() {
        return reconstruct_calls;
    }

} // namespace config