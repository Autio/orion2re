#include "lua/lua_registry.h"

#include <algorithm>

#include "config/semantic_version.h"

namespace orion2re::lua {
    namespace {

        std::vector<s_lua_runtime_source_record> Source_Inventory_(
            const s_lua_registry* registry) {
            std::vector<s_lua_runtime_source_record> result;
            for (const s_lua_runtime_package_record& package :
                registry->runtime_packages) {
                result.insert(result.end(), package.sources.begin(), package.sources.end());
            }
            std::sort(result.begin(), result.end(), [](const s_lua_runtime_source_record& left, const s_lua_runtime_source_record& right) {
                return left.path != right.path
                    ? left.path < right.path
                    : left.source_hash < right.source_hash;
            });
            result.erase(std::unique(result.begin(), result.end(), [](const s_lua_runtime_source_record& left, const s_lua_runtime_source_record& right) {
                return left.path == right.path &&
                    left.source_hash == right.source_hash;
            }),
                result.end());
            return result;
        }

        std::string Package_Version_(const s_lua_registry* registry,
            const std::string& package_id) {
            for (const s_lua_package_record& package :
                registry->packages_high_to_low_priority) {
                if (package.id == package_id) return package.version;
            }
            for (const s_lua_runtime_package_record& package :
                registry->runtime_packages) {
                if (package.package_id == package_id) {
                    return package.package_version;
                }
            }
            return {};
        }

        std::vector<s_lua_runtime_source_record> Package_Sources_(
            const s_lua_registry* registry, const std::string& package_id) {
            std::vector<s_lua_runtime_source_record> result;
            for (const s_lua_runtime_package_record& package :
                registry->runtime_packages) {
                if (package.package_id == package_id) {
                    result.insert(
                        result.end(), package.sources.begin(), package.sources.end());
                    for (const s_lua_runtime_migration_record& migration :
                        package.migrations) {
                        result.push_back({migration.path, migration.source_hash});
                    }
                }
            }
            std::sort(result.begin(), result.end(), [](const s_lua_runtime_source_record& left, const s_lua_runtime_source_record& right) {
                return left.path != right.path
                    ? left.path < right.path
                    : left.source_hash < right.source_hash;
            });
            result.erase(std::unique(result.begin(), result.end(), [](const s_lua_runtime_source_record& left, const s_lua_runtime_source_record& right) {
                return left.path == right.path &&
                    left.source_hash == right.source_hash;
            }),
                result.end());
            return result;
        }

        std::vector<std::string> Source_Package_Ids_(
            const s_lua_registry* registry) {
            std::vector<std::string> result;
            for (const s_lua_runtime_package_record& package :
                registry->runtime_packages) {
                result.push_back(package.package_id);
            }
            std::sort(result.begin(), result.end());
            result.erase(std::unique(result.begin(), result.end()), result.end());
            return result;
        }

        std::string Quoted_(const std::string& value) {
            return "'" + value + "'";
        }

        bool Mismatch_(std::string* diagnostic, const std::string& message) {
            *diagnostic = message;
            return false;
        }

        bool Equal_Runtime_Package_(const s_lua_runtime_package_record& left,
            const s_lua_runtime_package_record& right) {
            if (left.package_id != right.package_id ||
                !config::Semantic_Versions_Equal(
                    left.package_version, right.package_version) ||
                left.api != right.api ||
                left.storage_version != right.storage_version ||
                left.migrations.size() != right.migrations.size() ||
                left.services.size() != right.services.size() ||
                left.callbacks.size() != right.callbacks.size()) {
                return false;
            }
            for (size_t index = 0; index < left.migrations.size(); ++index) {
                const s_lua_runtime_migration_record& a = left.migrations[index];
                const s_lua_runtime_migration_record& b = right.migrations[index];
                if (a.from != b.from || a.to != b.to || a.path != b.path) {
                    return false;
                }
            }
            for (size_t index = 0; index < left.services.size(); ++index) {
                if (left.services[index].name != right.services[index].name ||
                    left.services[index].version != right.services[index].version) {
                    return false;
                }
            }
            for (size_t index = 0; index < left.callbacks.size(); ++index) {
                const s_lua_runtime_callback_record& a = left.callbacks[index];
                const s_lua_runtime_callback_record& b = right.callbacks[index];
                if (a.kind != b.kind ||
                    a.extension_name != b.extension_name ||
                    a.schema != b.schema ||
                    a.registration_ordinal != b.registration_ordinal) {
                    return false;
                }
            }
            return true;
        }

        bool Has_Lua_Identity_(const s_lua_registry* registry) {
            return !registry->runtime_packages.empty();
        }

        bool Package_Version_Changed_(const s_lua_registry* expected,
            const s_lua_registry* actual,
            const std::string& package_id) {
            return !config::Semantic_Versions_Equal(
                Package_Version_(expected, package_id),
                Package_Version_(actual, package_id));
        }

        bool Equal_Runtime_Package_For_Load_(
            const s_lua_runtime_package_record& left,
            const s_lua_runtime_package_record& right,
            bool upgraded) {
            if (left.package_id != right.package_id || left.api != right.api ||
                left.services.size() != right.services.size()) {
                return false;
            }
            for (size_t index = 0; index < left.services.size(); ++index) {
                if (left.services[index].name != right.services[index].name ||
                    left.services[index].version != right.services[index].version) {
                    return false;
                }
            }
            if (upgraded) return true;
            if (left.storage_version != right.storage_version ||
                left.migrations.size() != right.migrations.size() ||
                left.callbacks.size() != right.callbacks.size()) {
                return false;
            }
            for (size_t index = 0; index < left.migrations.size(); ++index) {
                const s_lua_runtime_migration_record& a = left.migrations[index];
                const s_lua_runtime_migration_record& b = right.migrations[index];
                if (a.from != b.from || a.to != b.to || a.path != b.path) {
                    return false;
                }
            }
            for (size_t index = 0; index < left.callbacks.size(); ++index) {
                const s_lua_runtime_callback_record& a = left.callbacks[index];
                const s_lua_runtime_callback_record& b = right.callbacks[index];
                if (a.kind != b.kind ||
                    a.extension_name != b.extension_name ||
                    a.schema != b.schema ||
                    a.registration_ordinal != b.registration_ordinal) {
                    return false;
                }
            }
            return true;
        }

    } // namespace

    bool Compare_Registry_Sources(const s_lua_registry* expected,
        const s_lua_registry* actual,
        std::string* diagnostic) {
        if (expected == nullptr || actual == nullptr || diagnostic == nullptr) {
            return false;
        }
        const std::vector<std::string> left_packages =
            Source_Package_Ids_(expected);
        const std::vector<std::string> right_packages =
            Source_Package_Ids_(actual);
        std::vector<std::string> packages = left_packages;
        packages.insert(
            packages.end(), right_packages.begin(), right_packages.end());
        std::sort(packages.begin(), packages.end());
        packages.erase(std::unique(packages.begin(), packages.end()),
            packages.end());
        for (const std::string& package_id : packages) {
            const std::string expected_version =
                Package_Version_(expected, package_id);
            const std::string actual_version =
                Package_Version_(actual, package_id);
            if (!expected_version.empty() && !actual_version.empty() &&
                !config::Semantic_Versions_Equal(
                    expected_version, actual_version)) {
                continue;
            }
            const std::vector<s_lua_runtime_source_record> left_sources =
                Package_Sources_(expected, package_id);
            const std::vector<s_lua_runtime_source_record> right_sources =
                Package_Sources_(actual, package_id);
            if (left_sources.size() != right_sources.size()) {
                return Mismatch_(diagnostic,
                    "loaded Lua file count differs for package " +
                        Quoted_(package_id));
            }
            for (size_t index = 0; index < left_sources.size(); ++index) {
                if (left_sources[index].path != right_sources[index].path) {
                    return Mismatch_(diagnostic,
                        "loaded Lua file differs for package " +
                            Quoted_(package_id) + ": save has " +
                            Quoted_(left_sources[index].path) +
                            ", active mod has " +
                            Quoted_(right_sources[index].path));
                }
                if (left_sources[index].source_hash !=
                    right_sources[index].source_hash) {
                    return Mismatch_(diagnostic,
                        "loaded Lua file content differs: " +
                            Quoted_(left_sources[index].path));
                }
            }
        }
        diagnostic->clear();
        return true;
    }

    bool Compare_Registry_Fingerprints(const s_lua_registry* expected,
        const s_lua_registry* actual,
        std::string* diagnostic) {
        if (expected == nullptr || actual == nullptr || diagnostic == nullptr) {
            return false;
        }
        if (expected->fingerprint_version != actual->fingerprint_version) {
            return Mismatch_(diagnostic,
                "Lua fingerprint format differs: expected " +
                    std::to_string(expected->fingerprint_version) + ", got " +
                    std::to_string(actual->fingerprint_version));
        }
        if (expected->lua_version != actual->lua_version) {
            return Mismatch_(diagnostic,
                "Lua runtime version differs: expected " +
                    Quoted_(expected->lua_version) + ", got " +
                    Quoted_(actual->lua_version));
        }
        if (expected->profile_version != actual->profile_version) {
            return Mismatch_(diagnostic,
                "Lua runtime profile differs: expected " +
                    std::to_string(expected->profile_version) + ", got " +
                    std::to_string(actual->profile_version));
        }
        if (expected->packages_high_to_low_priority.size() !=
            actual->packages_high_to_low_priority.size()) {
            return Mismatch_(diagnostic,
                "Lua package count differs: expected " +
                    std::to_string(
                        expected->packages_high_to_low_priority.size()) +
                    ", got " +
                    std::to_string(
                        actual->packages_high_to_low_priority.size()));
        }
        for (size_t index = 0;
             index < expected->packages_high_to_low_priority.size();
             ++index) {
            const s_lua_package_record& left =
                expected->packages_high_to_low_priority[index];
            const s_lua_package_record& right =
                actual->packages_high_to_low_priority[index];
            if (left.id != right.id) {
                return Mismatch_(diagnostic,
                    "Lua package order differs at index " +
                        std::to_string(index) + ": expected " +
                        Quoted_(left.id) + ", got " + Quoted_(right.id));
            }
            if (!config::Semantic_Versions_Equal(
                    left.version, right.version)) {
                return Mismatch_(diagnostic,
                    "Lua package " + Quoted_(left.id) +
                        " version differs: expected " +
                        Quoted_(left.version) + ", got " +
                        Quoted_(right.version));
            }
        }
        if (expected->runtime_packages.size() != actual->runtime_packages.size()) {
            return Mismatch_(diagnostic,
                "persistent Lua package/API/storage identity differs");
        }
        for (size_t index = 0; index < expected->runtime_packages.size();
             ++index) {
            if (!Equal_Runtime_Package_(expected->runtime_packages[index],
                    actual->runtime_packages[index])) {
                return Mismatch_(diagnostic,
                    "persistent Lua package/API/storage identity differs");
            }
        }
        if (expected->prototype_identity != actual->prototype_identity) {
            return Mismatch_(diagnostic,
                "Lua prototype identity differs: expected " +
                    Quoted_(expected->prototype_identity) + ", got " +
                    Quoted_(actual->prototype_identity));
        }
        if (expected->synchronized_config_hash !=
            actual->synchronized_config_hash) {
            return Mismatch_(diagnostic,
                "synchronized gameplay/technology configuration differs");
        }
        const std::vector<s_lua_runtime_source_record> left_sources =
            Source_Inventory_(expected);
        const std::vector<s_lua_runtime_source_record> right_sources =
            Source_Inventory_(actual);
        if (left_sources.size() != right_sources.size()) {
            return Mismatch_(diagnostic,
                "Lua source count differs: expected " +
                    std::to_string(left_sources.size()) + ", got " +
                    std::to_string(right_sources.size()));
        }
        for (size_t index = 0; index < left_sources.size(); ++index) {
            if (left_sources[index].path != right_sources[index].path) {
                return Mismatch_(diagnostic,
                    "Lua source path differs at index " +
                        std::to_string(index) + ": expected " +
                        Quoted_(left_sources[index].path) + ", got " +
                        Quoted_(right_sources[index].path));
            }
            if (left_sources[index].source_hash !=
                right_sources[index].source_hash) {
                return Mismatch_(diagnostic,
                    "Lua source hash differs for " +
                        Quoted_(left_sources[index].path) + ": expected " +
                        Quoted_(left_sources[index].source_hash) + ", got " +
                        Quoted_(right_sources[index].source_hash));
            }
        }
        for (size_t package_index = 0;
             package_index < expected->runtime_packages.size();
             ++package_index) {
            const auto& left =
                expected->runtime_packages[package_index].sources;
            const auto& right = actual->runtime_packages[package_index].sources;
            if (left.size() != right.size()) {
                return Mismatch_(diagnostic,
                    "Lua source package ownership differs");
            }
            for (size_t source_index = 0; source_index < left.size();
                 ++source_index) {
                if (left[source_index].path != right[source_index].path ||
                    left[source_index].source_hash !=
                        right[source_index].source_hash) {
                    return Mismatch_(diagnostic,
                        "Lua source package ownership differs");
                }
            }
        }
        if (expected->fingerprint != actual->fingerprint ||
            expected->fingerprint.empty()) {
            return Mismatch_(diagnostic, "Lua fingerprint differs");
        }
        diagnostic->clear();
        return true;
    }

    bool Compare_Registry_Save_Compatibility(const s_lua_registry* expected,
        const s_lua_registry* actual,
        bool allow_source_hash_mismatch,
        std::string* diagnostic,
        bool require_synchronized_config_hash) {
        if (expected == nullptr || actual == nullptr || diagnostic == nullptr) {
            return false;
        }
        if (!Has_Lua_Identity_(expected) && !Has_Lua_Identity_(actual)) {
            diagnostic->clear();
            return true;
        }
        if (expected->fingerprint_version != actual->fingerprint_version) {
            return Mismatch_(diagnostic, "Lua fingerprint format differs");
        }
        if (expected->lua_version != actual->lua_version) {
            return Mismatch_(diagnostic, "Lua runtime version differs");
        }
        if (expected->profile_version != actual->profile_version) {
            return Mismatch_(diagnostic, "Lua runtime profile differs");
        }
        if (expected->packages_high_to_low_priority.size() !=
            actual->packages_high_to_low_priority.size()) {
            return Mismatch_(diagnostic, "Lua package count differs");
        }
        for (size_t index = 0;
             index < expected->packages_high_to_low_priority.size(); ++index) {
            if (expected->packages_high_to_low_priority[index].id !=
                actual->packages_high_to_low_priority[index].id) {
                return Mismatch_(diagnostic, "Lua package order differs");
            }
        }
        if (expected->runtime_packages.size() != actual->runtime_packages.size()) {
            return Mismatch_(diagnostic,
                "persistent Lua package/API/storage identity differs");
        }
        for (size_t index = 0; index < expected->runtime_packages.size();
             ++index) {
            const s_lua_runtime_package_record& left =
                expected->runtime_packages[index];
            const s_lua_runtime_package_record& right =
                actual->runtime_packages[index];
            const bool upgraded = !config::Semantic_Versions_Equal(
                left.package_version, right.package_version);
            if (!Equal_Runtime_Package_For_Load_(left, right, upgraded)) {
                return Mismatch_(diagnostic,
                    "persistent Lua package/API/storage identity differs");
            }
        }
        if (expected->prototype_identity != actual->prototype_identity) {
            return Mismatch_(diagnostic, "Lua prototype identity differs");
        }
        if (require_synchronized_config_hash &&
            expected->synchronized_config_hash !=
            actual->synchronized_config_hash) {
            return Mismatch_(diagnostic,
                "synchronized gameplay/technology configuration differs");
        }
        const std::vector<std::string> expected_packages =
            Source_Package_Ids_(expected);
        const std::vector<std::string> actual_packages =
            Source_Package_Ids_(actual);
        if (expected_packages != actual_packages) {
            return Mismatch_(diagnostic, "Lua source package set differs");
        }
        for (const std::string& package_id : expected_packages) {
            const bool upgraded =
                Package_Version_Changed_(expected, actual, package_id);
            if (!upgraded) {
                const std::vector<s_lua_runtime_source_record> left_sources =
                    Package_Sources_(expected, package_id);
                const std::vector<s_lua_runtime_source_record> right_sources =
                    Package_Sources_(actual, package_id);
                if (left_sources.size() != right_sources.size()) {
                    return Mismatch_(diagnostic,
                        "loaded Lua file count differs for package " +
                            Quoted_(package_id));
                }
                for (size_t index = 0; index < left_sources.size(); ++index) {
                    if (left_sources[index].path != right_sources[index].path) {
                        return Mismatch_(diagnostic,
                            "loaded Lua file differs for package " +
                                Quoted_(package_id));
                    }
                    if (!allow_source_hash_mismatch &&
                        left_sources[index].source_hash !=
                            right_sources[index].source_hash) {
                        return Mismatch_(diagnostic,
                            "loaded Lua file content differs: " +
                                Quoted_(left_sources[index].path));
                    }
                }
            }
        }
        diagnostic->clear();
        return true;
    }

} // namespace orion2re::lua