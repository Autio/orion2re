#include "pch.h"

#include <SDL3/SDL_main.h>

#include "config/config_loader.h"
#include "lua/lua_mod_runtime.h"

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

static void Build_Path_(char* output, size_t output_size, const char* base_path, const char* relative_path) {
    if (base_path != nullptr && base_path[0] != '\0') {
        snprintf(output, output_size, "%s%s", base_path, relative_path);
    } else {
        snprintf(output, output_size, "%s", relative_path);
    }
}

static FILE* _game_log_file;
static SDL_LogOutputFunction _console_log_output;
static void* _console_log_userdata;

static const char* Log_Priority_Name_(SDL_LogPriority priority) {
    switch (priority) {
        case SDL_LOG_PRIORITY_TRACE: return "TRACE";
        case SDL_LOG_PRIORITY_VERBOSE: return "VERBOSE";
        case SDL_LOG_PRIORITY_DEBUG: return "DEBUG";
        case SDL_LOG_PRIORITY_INFO: return "INFO";
        case SDL_LOG_PRIORITY_WARN: return "WARN";
        case SDL_LOG_PRIORITY_ERROR: return "ERROR";
        case SDL_LOG_PRIORITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

static void SDLCALL Game_Log_Output_(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    if (_console_log_output != nullptr) {
        _console_log_output(_console_log_userdata, category, priority, message);
    }

    FILE* log_file = static_cast<FILE*>(userdata);
    if (log_file != nullptr) {
        fprintf(log_file, "%s: %s\n", Log_Priority_Name_(priority), message);
        fflush(log_file);
    }
}

static void Configure_Game_Log_() {
    const char* base_path = SDL_GetBasePath();
    char logs_path[1024];
    char log_file_path[1024];

    Build_Path_(logs_path, sizeof(logs_path), base_path, "logs");
    SDL_CreateDirectory(logs_path);

#if defined(_WIN32)
    const int process_id = _getpid();
    const char path_separator = '\\';
#else
    const int process_id = getpid();
    const char path_separator = '/';
#endif

    snprintf(log_file_path, sizeof(log_file_path), "%s%cgame.%d.log", logs_path, path_separator, process_id);
    _game_log_file = fopen(log_file_path, "w");
    if (_game_log_file != nullptr) {
        SDL_GetLogOutputFunction(&_console_log_output, &_console_log_userdata);
        SDL_SetLogOutputFunction(Game_Log_Output_, _game_log_file);
    }
}

static void Shutdown_Game_Log_() {
    if (_game_log_file == nullptr) {
        return;
    }

    SDL_SetLogOutputFunction(_console_log_output, _console_log_userdata);
    fclose(_game_log_file);
    _game_log_file = nullptr;
}

#if defined(ORION2RE_ASAN_BUILD)
extern "C" const char* __asan_default_options() {
    return "log_path=logs/asan";
}

static void Set_Environment_Variable_If_Unset_(const char* name, const char* value) {
    if (SDL_getenv(name) != nullptr) {
        return;
    }

    SDL_setenv_unsafe(name, value, 0);
}

static void Configure_Asan_Runtime_() {
    const char* base_path = SDL_GetBasePath();
    char logs_path[1024];

    Build_Path_(logs_path, sizeof(logs_path), base_path, "logs");
    SDL_CreateDirectory(logs_path);

#if defined(_WIN32)
    char dumps_path[1024];
    char dump_file_path[1024];

    Build_Path_(dumps_path, sizeof(dumps_path), base_path, "dumps");
    SDL_CreateDirectory(dumps_path);
    Build_Path_(dump_file_path, sizeof(dump_file_path), base_path, "dumps\\orion2re.dmp");
    Set_Environment_Variable_If_Unset_("ASAN_SAVE_DUMPS", dump_file_path);
#endif
}
#endif

int main(int argc, char** argv) {
    Configure_Game_Log_();

#if defined(ORION2RE_ASAN_BUILD)
    Configure_Asan_Runtime_();
#endif

    char* current_directory = SDL_GetCurrentDirectory();
    const char* base_path = SDL_GetBasePath();
    const char* vfs_base_path = current_directory != nullptr ? current_directory : base_path;

    if (vfs::Init(vfs_base_path) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "main: vfs::Init failed for base path '%s'", vfs_base_path != nullptr ? vfs_base_path : "");
        if (current_directory != nullptr) {
            SDL_free(current_directory);
        }
        Shutdown_Game_Log_();
        return -1;
    }

    config::s_effective_config effective_config;
    config::s_cfg_diagnostic config_diagnostic;
    bool config_loaded = false;
    if (!config::Load_Startup_Config("orion2re.cfg", "base_game", &effective_config, &config_loaded, &config_diagnostic)) {
        char config_error[2048];
        snprintf(config_error, sizeof(config_error), "Failed to load configuration:\n%s:%u:%u: %s", config_diagnostic.location.source.empty() ? "orion2re.cfg" : config_diagnostic.location.source.c_str(), config_diagnostic.location.line, config_diagnostic.location.column, config_diagnostic.message.c_str());
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", config_error);
        Show_Message_Box(nullptr, config_error, "Orion2 Redone", true);
        orion2re::lua::Shutdown_Lua();
        config::Shutdown_Config_Loader();
        config::Shutdown_Runtime();
        vfs::Shutdown();
        if (current_directory != nullptr) {
            SDL_free(current_directory);
        }
        Shutdown_Game_Log_();
        return -1;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        config_loaded ? "configuration: loaded orion2re.cfg" : "configuration: orion2re.cfg not found; using defaults");

    size_t command_line_length = 0;
    for (int i = 1; i < argc; ++i) {
        command_line_length += strlen(argv[i]);
        if (i + 1 < argc) {
            ++command_line_length;
        }
    }

    char* command_line = nullptr;
    if (command_line_length != 0) {
        command_line = (char*)malloc(command_line_length + 1);
        if (command_line == nullptr) {
            orion2re::lua::Shutdown_Lua();
            config::Shutdown_Config_Loader();
            config::Shutdown_Runtime();
            vfs::Shutdown();
            if (current_directory != nullptr) {
                SDL_free(current_directory);
            }
            Shutdown_Game_Log_();
            return -1;
        }

        size_t write_offset = 0;
        for (int i = 1; i < argc; ++i) {
            const size_t arg_length = strlen(argv[i]);
            memcpy(command_line + write_offset, argv[i], arg_length);
            write_offset += arg_length;

            if (i + 1 < argc) {
                command_line[write_offset++] = ' ';
            }
        }
        command_line[write_offset] = '\0';
    }

    const int result = platform::Run_Main_Loop(command_line);
    if (command_line != nullptr) {
        free(command_line);
    }

    orion2re::lua::Shutdown_Lua();
    ai_state::Shutdown();
    config::Shutdown_Config_Loader();
    config::Shutdown_Runtime();
    vfs::Shutdown();
    if (current_directory != nullptr) {
        SDL_free(current_directory);
    }
    Shutdown_Game_Log_();

    return result;
}