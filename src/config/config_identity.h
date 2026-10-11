#ifndef ORION2RE_CONFIG_IDENTITY_H
#define ORION2RE_CONFIG_IDENTITY_H

#include <cstdint>
#include <string>
#include <vector>

#include "config/config_runtime.h"

namespace config {

    inline constexpr std::uint32_t CONFIG_IDENTITY_FORMAT_VERSION = 1;
    inline constexpr std::uint32_t CONFIG_SPARSE_IDENTITY_FORMAT_VERSION = 2;

    bool Build_Canonical_Config_Identity(
        const s_config_snapshot_descriptor* descriptor, const s_config_state* state, std::vector<std::uint8_t>* serialization, std::string* diagnostic);

    // Version 2 identities bind sparse overrides to the captured native baseline.
    // Version 1 remains available for explicit legacy compatibility checks.
    bool Build_Sparse_Config_Identity(const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state, std::vector<uint8_t>* serialization, std::string* diagnostic);

} // namespace config

#endif