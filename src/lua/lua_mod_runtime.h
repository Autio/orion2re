#ifndef ORION2RE_LUA_MOD_RUNTIME_H
#define ORION2RE_LUA_MOD_RUNTIME_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "lua/lua_callback.h"
#include "lua/lua_runtime.h"
#include "lua/lua_storage.h"

namespace config {
    struct s_cfg_diagnostic;
    struct s_mod_manifest;
} // namespace config

namespace orion2re::lua {

    struct s_lua_mod_runtime;
    struct s_lua_mod_runtime_set;
    struct s_lua_storage_restore_transaction;
    struct s_lua_game_traits;
    struct s_lua_registry;

    enum e_lua_game_state_capability : std::uint32_t {
        LUA_GAME_STATE_READ_ONLY,
        LUA_GAME_STATE_MUTABLE,
        LUA_GAME_STATE_TECHNOLOGY_MUTABLE,
        LUA_GAME_STATE_PLANET_MUTABLE,
        LUA_GAME_STATE_GALAXY_MUTABLE,
        LUA_GAME_STATE_PLAYER_SETUP_MUTABLE,
    };

    struct s_lua_event_descriptor {
        const char* name = nullptr;
        uint32_t schema = 1;
        e_lua_game_state_capability game_state = LUA_GAME_STATE_READ_ONLY;
    };

    struct s_lua_rule_descriptor {
        const char* name = nullptr;
        uint32_t schema = 1;
    };

    struct s_lua_callback_info {
        e_lua_callback_kind kind = LUA_CALLBACK_EVENT;
        std::string extension_name;
        uint32_t schema = 1;
        size_t registration_ordinal = 0;
        std::string registration_source;
        int registration_line = 0;
        std::string callback_source;
        int callback_line = 0;
        std::string function_name;
    };

    struct s_lua_module_info {
        std::string name;
        std::string path;
        std::string source_hash;
    };

    struct s_lua_export_info {
        std::string name;
        uint32_t version = 0;
        size_t registration_ordinal = 0;
        std::string registration_source;
        int registration_line = 0;
        std::vector<std::string> methods;
    };

    struct s_lua_service_info {
        std::string name;
        uint32_t version = 0;
        std::string registration_source;
        int registration_line = 0;
        std::string callback_source;
        int callback_line = 0;
        std::string function_name;
    };

    struct s_lua_configuration_change {
        std::string package_id;
        std::string old_package_version;
        std::string new_package_version;
        uint32_t old_storage_version = 0;
        uint32_t new_storage_version = 0;
    };

    struct s_lua_mod_limits {
        s_lua_limits runtime;
        s_lua_storage_limits storage;
        size_t callbacks = 256;
        size_t exports = 64;
        size_t services = 16;
        size_t export_methods = 64;
        size_t modules = 64;
        size_t module_nesting = 32;
        size_t marshalled_values = 4096;
        size_t marshalled_entries = 4096;
        size_t marshalled_bytes = 256 * 1024;
        size_t marshalling_depth = 32;
        size_t cross_mod_call_depth = 16;
        size_t package_source_bytes = 1024 * 1024;
    };

    bool Create_Mod_Runtime(const char* package_id, const char* package_version, const char* main_source, size_t main_source_size, const char* main_source_name, size_t package_source_bytes, const s_lua_event_descriptor* event_descriptors, size_t event_descriptor_count, const s_lua_rule_descriptor* rule_descriptors, size_t rule_descriptor_count, const s_lua_mod_limits* limits, s_lua_mod_runtime** runtime, s_lua_result* result);
    void Destroy_Mod_Runtime(s_lua_mod_runtime* runtime);
    size_t Get_Mod_Callback_Count(const s_lua_mod_runtime* runtime);
    bool Get_Mod_Callback_Info(const s_lua_mod_runtime* runtime,
        size_t index,
        s_lua_callback_info* info);
    size_t Get_Mod_Module_Count(const s_lua_mod_runtime* runtime);
    bool Get_Mod_Module_Info(const s_lua_mod_runtime* runtime,
        size_t index,
        s_lua_module_info* info);
    size_t Get_Mod_Export_Count(const s_lua_mod_runtime* runtime);
    bool Get_Mod_Export_Info(const s_lua_mod_runtime* runtime,
        size_t index,
        s_lua_export_info* info);
    size_t Get_Mod_Service_Count(const s_lua_mod_runtime* runtime);
    bool Get_Mod_Service_Info(const s_lua_mod_runtime* runtime,
        size_t index,
        s_lua_service_info* info);
    bool Dispatch_Mod_Event(s_lua_mod_runtime* runtime, const char* event_name, t_lua_argument_builder build_event, void* user_data, s_lua_result* result);
    bool Dispatch_Mod_Rule(s_lua_mod_runtime* runtime,
        const char* rule_name,
        t_lua_argument_builder build_context,
        void* user_data,
        std::int64_t minimum,
        std::int64_t maximum,
        std::int64_t* value,
        s_lua_result* result);
    bool Game_State_Access_Allowed();
    bool Game_State_Mutation_Allowed();
    bool Player_Setup_State_Mutation_Allowed();
    bool Technology_State_Mutation_Allowed();
    bool Planet_State_Mutation_Allowed();
    bool Galaxy_State_Mutation_Allowed();
    bool Rule_Modifier_Active();
    bool Stage_Mod_Runtimes(
        const std::vector<config::s_mod_manifest>* packages_high_to_low_priority,
        const s_lua_mod_limits* limits,
        s_lua_registry* registry,
        s_lua_mod_runtime_set** candidate,
        config::s_cfg_diagnostic* diagnostic);
    void Destroy_Mod_Runtime_Set(s_lua_mod_runtime_set* runtime_set);
    void Commit_Mod_Runtimes(s_lua_mod_runtime_set** candidate);
    uint64_t* Get_Mod_Runtime_Ship_Generations(
        s_lua_mod_runtime_set* runtime_set);
    const s_lua_mod_runtime_set* Get_Active_Mod_Runtimes();
    size_t Get_Mod_Runtime_Count(const s_lua_mod_runtime_set* runtime_set);
    const s_lua_mod_runtime* Get_Mod_Runtime(
        const s_lua_mod_runtime_set* runtime_set, size_t index);
    s_lua_mod_runtime* Get_Mod_Runtime(
        s_lua_mod_runtime_set* runtime_set, size_t index);
    s_lua_mod_runtime* Find_Mod_Runtime(
        s_lua_mod_runtime_set* runtime_set, const char* package_id);
    const s_lua_mod_runtime* Find_Mod_Runtime(
        const s_lua_mod_runtime_set* runtime_set, const char* package_id);
    const char* Get_Mod_Runtime_Package_Id(const s_lua_mod_runtime* runtime);
    const char* Get_Mod_Runtime_Package_Version(
        const s_lua_mod_runtime* runtime);
    uint32_t Get_Mod_Runtime_Api(const s_lua_mod_runtime* runtime);
    uint32_t Get_Mod_Runtime_Storage_Version(
        const s_lua_mod_runtime* runtime);
    bool Check_Network_Mod_Runtime_Compatibility(
        const s_lua_mod_runtime_set* runtime_set,
        std::string* diagnostic);
    bool Serialize_Mod_Storage(s_lua_mod_runtime* runtime,
        std::vector<uint8_t>* bytes,
        std::string* diagnostic);
    bool Restore_Mod_Storage(s_lua_mod_runtime* runtime,
        uint32_t saved_version,
        const uint8_t* bytes,
        size_t size,
        std::string* diagnostic);
    bool Prepare_Active_Mod_Storage_Restore(s_lua_mod_runtime* runtime,
        uint32_t saved_version,
        const uint8_t* bytes,
        size_t size,
        s_lua_storage_restore_transaction** transaction,
        std::string* diagnostic);
    void Commit_Active_Mod_Storage_Restore(
        s_lua_storage_restore_transaction** transaction);
    void Discard_Active_Mod_Storage_Restore(
        s_lua_storage_restore_transaction** transaction);
    bool Dispatch_Active_Initialized_Event(std::string* diagnostic);
    bool Dispatch_Active_Loaded_Event(std::string* diagnostic);
    bool Dispatch_Configuration_Changed_Event(
        s_lua_mod_runtime_set* runtime_set,
        const std::vector<s_lua_configuration_change>* changes,
        std::string* diagnostic);
    bool Dispatch_Active_Configuration_Reloaded_Event(
        const std::vector<std::string>* changed_groups,
        std::string* diagnostic);
    bool Has_Active_Galaxy_Generator();
    bool Dispatch_Active_Galaxy_Generator(
        bool* invoked, std::string* diagnostic);
    bool Dispatch_Loaded_Event(s_lua_mod_runtime_set* runtime_set,
        std::string* diagnostic);
    s_lua_game_traits* Get_Game_Traits(
        s_lua_mod_runtime_set* runtime_set);
    s_lua_game_traits* Get_Active_Game_Traits();
    void Reset_Mod_Runtimes();
    void Shutdown_Lua();

} // namespace orion2re::lua

#endif