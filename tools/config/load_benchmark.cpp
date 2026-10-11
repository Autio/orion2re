#include "pch.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "config/config_loader.h"
#include "config/config_runtime.h"
#include "config/config_decode.h"
#include "config/config_snapshot.h"
#include "game/allocate.h"
#include "game/savegame.h"
#include "game/serialization.h"
#include "game/vfs.h"
#include "lua/lua_mod_runtime.h"
#include "lua/lua_registry.h"
#include "lua/lua_save.h"

namespace {
    template <typename Function>
    double Time_Ms(Function function) {
        const auto start = std::chrono::steady_clock::now();
        function();
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();
    }

    bool Benchmark_Consumers_(const SAVEGAME::s_metadata& metadata,
        const s_settings& settings,
        uint32_t seed) {
        MOX::settings = settings;
        std::string error;
        std::vector<uint8_t> buffer(SAVEGAME::BUFFER_CAPACITY);
        const auto save = [&](const char* phase, std::vector<uint8_t>* output) {
            SERIAL::Writer_ writer(buffer.data(), static_cast<uint32_t>(buffer.size()), "benchmark save overflow");
            bool ok = false;
            const double ms = Time_Ms([&] {
                ok = SAVEGAME::Write_Game(&writer, metadata.description, seed, &error);
            });
            std::printf("%s_ms=%.3f save_ok=%d bytes=%u error=%s\n", phase, ms, ok ? 1 : 0, writer.Length(), error.c_str());
            if (ok) output->assign(buffer.begin(), buffer.begin() + writer.Length());
            return ok;
        };
        std::vector<uint8_t> first, cached, changed;
        if (!save("save_cache_miss", &first) || !save("save_cache_hit", &cached) || first != cached) return false;

        constexpr char path[] = "race.picks.starting";
        config::t_variable_id id = config::CONFIG_NO_VARIABLE;
        if (!config::Find_Variable(path, sizeof(path) - 1, &id)) return false;
        config::s_config_write_set writes;
        config::s_config_write write;
        write.variable_id = id;
        write.canonical_path = path;
        if (!config::Read_State_Value(&config::Get_Active_Config()->baseline.effective.state, id, &write.value)) return false;
        write.value.integer_value = write.value.integer_value == 12 ? 10 : 12;
        writes.writes.push_back(write);
        config::s_cfg_diagnostic diagnostic;
        if (!config::Commit_Active_Config_Writes(&writes, &diagnostic) ||
            !save("save_after_mutation", &changed) || changed == first) return false;

        // Validate the produced save outside the write timing. Never overwrite
        // the input file, and require the changed configuration to round-trip.
        SERIAL::Reader_ reader(changed.data(), static_cast<uint32_t>(changed.size()), "benchmark save roundtrip underflow");
        SAVEGAME::s_metadata decoded{};
        s_settings decoded_settings{};
        uint32_t decoded_seed = 0;
        if (!SAVEGAME::Read_Game(&reader, &decoded, &decoded_settings, &decoded_seed, &error) ||
            reader.Remaining() != 0 || decoded_seed != seed ||
            config::Get_Active_Config()->baseline.effective.state.values[id].integer_value != write.value.integer_value) return false;
        std::printf("save_roundtrip_ok=1\n");

        const config::s_prepared_config_snapshot* prepared = nullptr;
        for (const char* phase : {"network_snapshot_cache_miss", "network_snapshot_cache_hit"}) {
            bool ok = false;
            const double ms = Time_Ms([&] {
                ok = config::Get_Prepared_Active_Config_Snapshot(&config::synchronized_config_snapshot, &prepared, &diagnostic);
            });
            std::printf("%s_ms=%.3f snapshot_ok=%d\n", phase, ms, ok ? 1 : 0);
            if (!ok) return false;
        }
        const auto config_bytes = prepared->bytes;
        orion2re::lua::s_lua_save_snapshot lua_snapshot;
        const std::vector<uint8_t>* identity = nullptr;
        if (!orion2re::lua::Capture_Lua_Save_Snapshot(&lua_snapshot, &error) ||
            !config::Get_Canonical_Active_Config_Identity(&identity, &diagnostic) ||
            !orion2re::lua::Finalize_Registry_Fingerprint_With_Config_Identity(&lua_snapshot.registry, identity, &error)) return false;
        const auto lua_size = orion2re::lua::Lua_Save_Snapshot_Size(&lua_snapshot);
        if (lua_size == 0) return false;
        std::vector<uint8_t> lua_bytes(lua_size);
        SERIAL::Writer_ lua_writer(lua_bytes.data(), lua_size, "benchmark Lua snapshot overflow");
        orion2re::lua::Write_Lua_Save_Snapshot(&lua_writer, &lua_snapshot);
        config::s_network_config_candidate* candidate = nullptr;
        bool ok = false;
        const double stage_ms = Time_Ms([&] {
            ok = config::Stage_Network_Config_Snapshot(config_bytes.data(), static_cast<uint32_t>(config_bytes.size()), &candidate, &diagnostic);
        });
        std::printf("network_stage_ms=%.3f stage_ok=%d error=%s\n", stage_ms, ok ? 1 : 0, diagnostic.message.c_str());
        if (!ok) return false;
        const double finalize_ms = Time_Ms([&] {
            ok = config::Finalize_Network_Config_Snapshot(&candidate, lua_bytes.data(), lua_writer.Length(), &error);
        });
        std::printf("network_finalize_ms=%.3f finalize_ok=%d error=%s\n", finalize_ms, ok ? 1 : 0, error.c_str());
        config::Discard_Network_Config_Snapshot(&candidate);
        if (!ok) return false;
        const double reconstruct_ms = Time_Ms([&] {
            ok = config::Reconstruct_Active_Lua_Runtimes(&error);
        });
        std::printf("host_reconstruct_ms=%.3f reconstruct_ok=%d error=%s\n", reconstruct_ms, ok ? 1 : 0, error.c_str());
        return ok;
    }
} // namespace

int main(int argc, char** argv) {
    const auto main_start = std::chrono::steady_clock::now();
    if (argc != 3 && argc != 4) {
        std::fprintf(stderr, "usage: config-load-benchmark GAME_ROOT SAVE_FILE [LOAD_COUNT]\n");
        return 1;
    }
    if (vfs::Init(argv[1]) == 0) return 1;
    config::s_effective_config effective;
    config::s_cfg_diagnostic diagnostic;
    bool loaded = false;
    bool startup_ok = false;
    const double startup_ms = Time_Ms([&] {
        startup_ok = config::Load_Startup_Config(
            "orion2re.cfg", "base_game", &effective, &loaded, &diagnostic);
    });
    const std::chrono::duration<double, std::milli> main_to_config =
        std::chrono::steady_clock::now() - main_start;
    const double main_to_config_ms = main_to_config.count();
    if (!startup_ok || !loaded) {
        std::fprintf(stderr, "startup failed or config absent: %s\n", diagnostic.message.c_str());
        return 2;
    }
    std::printf("startup_ms=%.3f main_to_config_ms=%.3f\n", startup_ms, main_to_config_ms);
    for (const auto& package : config::Get_Active_Config()->required_mods) {
        std::printf("package=%s version=%s\n", package.id.c_str(), package.version.c_str());
    }
    std::fflush(stdout);
    const int load_count = argc == 4 ? std::atoi(argv[3]) : 1;
    if (load_count < 1 || load_count > 100) return 1;

    // Read_Game backs up native state before parsing; use the game's normal
    // allocation path even though this harness does not create a window.
    allocate::Allocate_Data_Space();
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) return 3;
    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (bytes.empty() || bytes.size() > UINT32_MAX) return 3;
    SERIAL::Reader_ reader(bytes.data(), static_cast<uint32_t>(bytes.size()), "benchmark save underflow");
    SAVEGAME::s_metadata metadata = {};
    s_settings settings = {};
    uint32_t seed = 0;
    std::string error;
    bool load_ok = false;
    const double load_ms = Time_Ms([&] {
        load_ok = SAVEGAME::Read_Game(&reader, &metadata, &settings, &seed, &error);
    });
    std::printf("load_ms=%.3f load_ok=%d remaining=%u version=%u error=%s\n",
        load_ms,
        load_ok ? 1 : 0,
        reader.Remaining(),
        metadata.version,
        error.c_str());
    std::fflush(stdout);
    for (int index = 1; load_ok && index < load_count; ++index) {
        SERIAL::Reader_ repeated(bytes.data(), static_cast<uint32_t>(bytes.size()), "benchmark save underflow");
        const double repeated_ms = Time_Ms([&] {
            load_ok = SAVEGAME::Read_Game(&repeated, &metadata, &settings, &seed, &error);
        });
        std::printf("repeat_load_ms=%.3f load_ok=%d remaining=%u error=%s\n", repeated_ms, load_ok ? 1 : 0, repeated.Remaining(), error.c_str());
        load_ok = load_ok && repeated.Remaining() == 0;
        std::fflush(stdout);
    }
    const bool consumers_ok = load_ok && reader.Remaining() == 0 && Benchmark_Consumers_(metadata, settings, seed);
    std::fflush(stdout);
    orion2re::lua::Shutdown_Lua();
    config::Shutdown_Config_Loader();
    config::Shutdown_Runtime();
    vfs::Shutdown();
    return consumers_ok ? 0 : 4;
}