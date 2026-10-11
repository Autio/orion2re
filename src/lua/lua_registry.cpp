#include "lua/lua_registry.h"

#include <algorithm>
#include <utility>

namespace orion2re::lua {
    namespace {
        s_lua_registry active_registry_;

        bool Fail(const std::string& source, const std::string& message, config::s_cfg_diagnostic* diagnostic) {
            diagnostic->location = {};
            diagnostic->location.source = source;
            diagnostic->location.line = 1;
            diagnostic->location.column = 1;
            diagnostic->message = message;
            return false;
        }

        bool Valid_Package_Id(const std::string& id) {
            if (id.empty() || id.front() < 'a' || id.front() > 'z') {
                return false;
            }
            return std::all_of(id.begin(), id.end(), [](char character) {
                return (character >= 'a' && character <= 'z') ||
                    (character >= '0' && character <= '9') || character == '_' ||
                    character == '-';
            });
        }

    } // namespace

    bool Stage_Registry(
        const std::vector<config::s_mod_manifest>* packages_high_to_low_priority,
        s_lua_registry* candidate,
        config::s_cfg_diagnostic* diagnostic) {
        if (packages_high_to_low_priority == nullptr || candidate == nullptr ||
            diagnostic == nullptr) {
            return false;
        }
        s_lua_registry staged;

        staged.packages_high_to_low_priority.reserve(
            packages_high_to_low_priority->size());
        for (const config::s_mod_manifest& package :
            *packages_high_to_low_priority) {
            if (!Valid_Package_Id(package.id)) {
                return Fail("mods/" + package.id + "/mod.json5",
                    "invalid Lua package ID '" + package.id + "'",
                    diagnostic);
            }
            for (const s_lua_package_record& prior :
                staged.packages_high_to_low_priority) {
                if (prior.id == package.id) {
                    return Fail("mods/" + package.id + "/mod.json5",
                        "duplicate Lua package identity '" + package.id + "'",
                        diagnostic);
                }
            }
            staged.packages_high_to_low_priority.push_back(
                {package.id, package.version_text});
        }

        *diagnostic = {};
        *candidate = std::move(staged);
        return true;
    }

    void Commit_Registry(s_lua_registry* candidate) {
        if (candidate != nullptr) {
            active_registry_ = std::move(*candidate);
        }
    }

    const s_lua_registry* Get_Active_Registry() {
        return &active_registry_;
    }

    void Reset_Registry() {
        active_registry_ = {};
    }

} // namespace orion2re::lua