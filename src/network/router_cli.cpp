#include "network/router_cli.h"

#include "network/router_service.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <uv.h>

namespace network::router {

    bool Is_Command_Token(const char* value) {
        return value != nullptr &&
            (std::strcmp(value, "router") == 0 || std::strcmp(value, "--router") == 0);
    }

    void Print_Usage(const char* program_name) {
        std::fprintf(stdout,
            "Usage: %s [router|--router] [options]\n"
            "\n"
            "Options:\n"
            "  --host ADDRESS          TCP listen address (default: 0.0.0.0)\n"
            "  --port PORT             TCP listen port (default: 47800)\n"
            "  --discovery-port PORT   UDP LAN discovery port (default: 47800)\n"
            "  --router-id ID          Router identity advertised to clients\n"
            "  --name NAME             Router display name for LAN discovery\n"
            "  --discovery             Enable UDP LAN discovery replies\n"
            "  --no-discovery          Disable UDP LAN discovery replies\n"
            "  --no-signal-handlers    Disable SIGINT/SIGTERM handlers\n"
            "  --verbose               Enable detailed message/discovery/traffic logging\n"
            "  --quiet                 Disable detailed logging; keep important router events only\n"
            "  --help                  Show this help\n",
            program_name != nullptr ? program_name : "orion2re-router");
    }

    bool Parse_Port(const char* text, uint16_t* port) {
        if (text == nullptr || port == nullptr || *text == '\0') {
            return false;
        }

        char* end = nullptr;
        const long value = std::strtol(text, &end, 10);
        if (*end != '\0' || value < 0 || value > 65535) {
            return false;
        }

        *port = (uint16_t)value;
        return true;
    }

    bool Is_Router_CLI_Command(int argc, char** argv) {
        return argc >= 2 && Is_Command_Token(argv[1]);
    }

    int Run_Router_CLI(int argc, char** argv) {
        s_router_config config;
        config.enable_discovery = false;
        int arg_index = 1;
        if (argc >= 2 && Is_Command_Token(argv[1])) {
            arg_index = 2;
        }

        for (int i = arg_index; i < argc; ++i) {
            const char* arg = argv[i];
            if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
                Print_Usage(argv[0]);
                return 0;
            }
            if (std::strcmp(arg, "--host") == 0 && i + 1 < argc) {
                Router_Copy_String(config.listen_host, sizeof(config.listen_host), argv[++i]);
                continue;
            }
            if (std::strcmp(arg, "--port") == 0 && i + 1 < argc) {
                if (!Parse_Port(argv[++i], &config.tcp_port)) {
                    std::fprintf(stderr, "Invalid --port value.\n");
                    return 2;
                }
                continue;
            }
            if (std::strcmp(arg, "--discovery-port") == 0 && i + 1 < argc) {
                if (!Parse_Port(argv[++i], &config.discovery_port)) {
                    std::fprintf(stderr, "Invalid --discovery-port value.\n");
                    return 2;
                }
                continue;
            }
            if (std::strcmp(arg, "--discovery") == 0) {
                config.enable_discovery = true;
                continue;
            }
            if (std::strcmp(arg, "--router-id") == 0 && i + 1 < argc) {
                Router_Copy_String(config.router_id, sizeof(config.router_id), argv[++i]);
                continue;
            }
            if (std::strcmp(arg, "--name") == 0 && i + 1 < argc) {
                Router_Copy_String(config.router_name, sizeof(config.router_name), argv[++i]);
                continue;
            }
            if (std::strcmp(arg, "--no-discovery") == 0) {
                config.enable_discovery = false;
                continue;
            }
            if (std::strcmp(arg, "--no-signal-handlers") == 0) {
                config.enable_signal_handlers = false;
                continue;
            }
            if (std::strcmp(arg, "--verbose") == 0 || std::strcmp(arg, "-v") == 0) {
                config.verbose_logging = true;
                continue;
            }
            if (std::strcmp(arg, "--quiet") == 0 || std::strcmp(arg, "-q") == 0) {
                config.verbose_logging = false;
                continue;
            }

            std::fprintf(stderr, "Unknown or incomplete router option: %s\n", arg);
            Print_Usage(argv[0]);
            return 2;
        }

        s_router_service service;
        Router_Service_Init(&service, &config);
        std::fprintf(stdout,
            "Starting router '%s' on %s:%u%s\n",
            config.router_id,
            config.listen_host,
            (unsigned)config.tcp_port,
            config.enable_discovery ? " with UDP discovery" : "");
        const int result = Router_Service_Run(&service);
        if (result != 0) {
            std::fprintf(stderr, "Router stopped with libuv error %d: %s\n", result, uv_strerror(result));
            return 1;
        }
        return 0;
    }

} // namespace network::router