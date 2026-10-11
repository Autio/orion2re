#include "lua/lua_registry.h"

#include <algorithm>
#include <limits>

#include "config/config_identity.h"
#include "game/sha256.h"

namespace orion2re::lua {
    namespace {

        struct s_canonical_writer {
            std::vector<std::uint8_t>* output;
        };

        void Write_U32_(s_canonical_writer* writer, std::uint32_t value) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                writer->output->push_back(static_cast<std::uint8_t>(value >> shift));
            }
        }

        bool Write_String_(s_canonical_writer* writer, const std::string* value) {
            if (value->size() > std::numeric_limits<std::uint32_t>::max()) {
                return false;
            }
            Write_U32_(writer, static_cast<std::uint32_t>(value->size()));
            if (!value->empty()) {
                const auto* bytes = reinterpret_cast<const std::uint8_t*>(
                    value->data());
                writer->output->insert(writer->output->end(), bytes, bytes + value->size());
            }
            return true;
        }

        bool Source_Less_(const s_lua_runtime_source_record& left,
            const s_lua_runtime_source_record& right) {
            if (left.path != right.path) return left.path < right.path;
            return left.source_hash < right.source_hash;
        }

        bool Source_Equal_(const s_lua_runtime_source_record& left,
            const s_lua_runtime_source_record& right) {
            return left.path == right.path &&
                left.source_hash == right.source_hash;
        }

        std::vector<s_lua_runtime_source_record> Source_Inventory_(
            const s_lua_registry* registry) {
            std::vector<s_lua_runtime_source_record> result;
            for (const s_lua_runtime_package_record& package :
                registry->runtime_packages) {
                result.insert(result.end(), package.sources.begin(), package.sources.end());
            }
            std::sort(result.begin(), result.end(), Source_Less_);
            result.erase(std::unique(
                             result.begin(), result.end(), Source_Equal_),
                result.end());
            return result;
        }

    } // namespace

    bool Finalize_Registry_Fingerprint(
        s_lua_registry* registry, const config::s_config_state* configuration, std::string* diagnostic) {
        if (registry == nullptr || configuration == nullptr) {
            if (diagnostic != nullptr) {
                *diagnostic = "invalid Lua fingerprint input";
            }
            return false;
        }

        std::vector<std::uint8_t> config_identity;
        if (!config::Build_Sparse_Config_Identity(
                &config::synchronized_config_snapshot, configuration, &config_identity, diagnostic)) {
            return false;
        }
        return Finalize_Registry_Fingerprint_With_Config_Identity(
            registry, &config_identity, diagnostic);
    }

    bool Finalize_Registry_Fingerprint_With_Config_Identity(
        s_lua_registry* registry,
        const std::vector<std::uint8_t>* configuration_identity,
        std::string* diagnostic) {
        if (registry == nullptr || configuration_identity == nullptr ||
            configuration_identity->empty()) {
            if (diagnostic != nullptr) {
                *diagnostic = "invalid Lua fingerprint input";
            }
            return false;
        }
        const crypto::Sha256Digest config_digest = crypto::sha256(
            std::string_view(
                reinterpret_cast<const char*>(configuration_identity->data()),
                configuration_identity->size()));

        std::vector<std::uint8_t> serialization;
        serialization.insert(serialization.end(), {'O', '2', 'L', 'S'});
        s_canonical_writer writer{&serialization};
        Write_U32_(&writer, registry->fingerprint_version);
        const std::vector<s_lua_runtime_source_record> sources =
            Source_Inventory_(registry);
        if (sources.size() > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        Write_U32_(&writer, static_cast<std::uint32_t>(sources.size()));
        for (const s_lua_runtime_source_record& source : sources) {
            if (!Write_String_(&writer, &source.path) ||
                !Write_String_(&writer, &source.source_hash)) {
                return false;
            }
        }

        registry->fingerprint_serialization = std::move(serialization);
        const bool sparse_identity = configuration_identity->size() >= 8 &&
            (*configuration_identity)[0] == 'O' && (*configuration_identity)[1] == '2' &&
            (*configuration_identity)[2] == 'C' && (*configuration_identity)[3] == 'I' &&
            (*configuration_identity)[7] == config::CONFIG_SPARSE_IDENTITY_FORMAT_VERSION;
        registry->synchronized_config_hash = (sparse_identity ? "v2:" : "") + crypto::sha256_hex(config_digest);
        registry->fingerprint = crypto::sha256_hex(std::string_view(
            reinterpret_cast<const char*>(registry->fingerprint_serialization.data()),
            registry->fingerprint_serialization.size()));
        if (diagnostic != nullptr) {
            diagnostic->clear();
        }
        return true;
    }

} // namespace orion2re::lua