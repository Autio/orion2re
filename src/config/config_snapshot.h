#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/mod_manifest.h"
#include "config/config_runtime.h"

namespace SERIAL {
    class Reader_;
    class Writer_;
} // namespace SERIAL

namespace config {

    constexpr uint32_t CONFIG_MAX_SNAPSHOT_SIZE = 3 * 1024 * 1024;
    constexpr uint32_t CONFIG_MAX_SNAPSHOT_PATH_SIZE = 1024;
    constexpr uint32_t CONFIG_MAX_SNAPSHOT_MOD_ID_SIZE = 255;
    constexpr uint32_t CONFIG_MAX_SNAPSHOT_MOD_VERSION_SIZE = 64;
    constexpr uint32_t CONFIG_MAX_SNAPSHOT_MOD_COUNT = 4096;

    struct s_config_snapshot {
        s_config_state state;
        std::vector<s_mod_requirement> required_mods;
        bool has_required_mods = false;
    };

    struct s_prepared_config_snapshot {
        std::vector<uint8_t> bytes;
    };

    bool Prepare_Config_Snapshot(
        const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state,
        const std::vector<s_mod_requirement>* required_mods,
        s_prepared_config_snapshot* snapshot);
    void Write_Prepared_Config_Snapshot(SERIAL::Writer_* writer,
        const s_prepared_config_snapshot* snapshot);

    uint32_t Config_Snapshot_Size(const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state,
        const std::vector<s_mod_requirement>* required_mods = nullptr);
    void Write_Config_Snapshot(SERIAL::Writer_* writer,
        const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state,
        const std::vector<s_mod_requirement>* required_mods = nullptr);
    bool Read_Config_Snapshot(SERIAL::Reader_* reader,
        const s_config_snapshot_descriptor* descriptor,
        s_config_snapshot* snapshot);
    bool Read_Config_Snapshot(SERIAL::Reader_* reader,
        const s_config_snapshot_descriptor* descriptor,
        s_config_state* state);
    bool Apply_Config_Snapshot(SERIAL::Reader_* reader,
        const s_config_snapshot_descriptor* descriptor);

} // namespace config