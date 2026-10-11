#ifndef ORION2RE_LUA_REGISTRY_H
#define ORION2RE_LUA_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "config/mod_manifest.h"
#include "lua/lua_callback.h"
#include "lua/lua_runtime.h"

namespace config {
    struct s_config_state;
}

namespace orion2re::lua {

    inline constexpr std::uint32_t LUA_FINGERPRINT_FORMAT_VERSION = 3;

    struct s_lua_package_record {
        std::string id;
        std::string version;
    };

    struct s_lua_runtime_source_record {
        std::string path;
        std::string source_hash;
    };

    struct s_lua_runtime_callback_record {
        e_lua_callback_kind kind = LUA_CALLBACK_EVENT;
        std::string extension_name;
        std::uint32_t schema = 1;
        std::size_t registration_ordinal = 0;
        std::string registration_source;
        int registration_line = 0;
        std::string callback_source;
        int callback_line = 0;
        std::string function_name;
    };

    struct s_lua_runtime_migration_record {
        std::uint32_t from = 0;
        std::uint32_t to = 0;
        std::string path;
        std::string source_hash;
    };

    struct s_lua_runtime_service_record {
        std::string name;
        std::uint32_t version = 0;
    };

    struct s_lua_runtime_package_record {
        std::string package_id;
        std::string package_version;
        std::uint32_t api = 0;
        std::uint32_t storage_version = 0;
        std::vector<s_lua_runtime_source_record> sources;
        std::vector<s_lua_runtime_migration_record> migrations;
        std::vector<s_lua_runtime_service_record> services;
        std::vector<s_lua_runtime_callback_record> callbacks;
    };

    struct s_lua_registry {
        std::uint32_t fingerprint_version = LUA_FINGERPRINT_FORMAT_VERSION;
        std::string lua_version = LUA_UPSTREAM_VERSION;
        std::uint32_t profile_version = LUA_RUNTIME_PROFILE_VERSION;
        std::vector<s_lua_package_record> packages_high_to_low_priority;
        std::vector<s_lua_runtime_package_record> runtime_packages;
        std::vector<std::uint8_t> fingerprint_serialization;
        std::string prototype_identity;
        std::string synchronized_config_hash;
        std::string fingerprint;
    };

    bool Stage_Registry(
        const std::vector<config::s_mod_manifest>* packages_high_to_low_priority,
        s_lua_registry* candidate,
        config::s_cfg_diagnostic* diagnostic);
    bool Finalize_Registry_Fingerprint(
        s_lua_registry* registry, const config::s_config_state* configuration, std::string* diagnostic);
    bool Finalize_Registry_Fingerprint_With_Config_Identity(
        s_lua_registry* registry,
        const std::vector<std::uint8_t>* configuration_identity,
        std::string* diagnostic);
    bool Compare_Registry_Fingerprints(const s_lua_registry* expected,
        const s_lua_registry* actual,
        std::string* diagnostic);
    bool Compare_Registry_Sources(const s_lua_registry* expected,
        const s_lua_registry* actual,
        std::string* diagnostic);
    bool Compare_Registry_Save_Compatibility(const s_lua_registry* expected,
        const s_lua_registry* actual,
        bool allow_source_hash_mismatch,
        std::string* diagnostic,
        bool require_synchronized_config_hash = true);
    void Commit_Registry(s_lua_registry* candidate);
    const s_lua_registry* Get_Active_Registry();
    void Reset_Registry();

} // namespace orion2re::lua

#endif