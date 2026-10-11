#include "pch.h"

#include "game/netcode_bootstrap.h"

namespace netcode {
    namespace {
        bool Is_Router_Host_Char(char value) {
            return (value >= '0' && value <= '9') ||
                (value >= 'A' && value <= 'Z') ||
                (value >= 'a' && value <= 'z') ||
                value == '.' || value == '-';
        }

        bool Is_Valid_Router_Host(const char* host, uint32_t host_len) {
            if (host_len == 0 || host[0] == '-' || host[host_len - 1] == '-' ||
                host[0] == '.' || host[host_len - 1] == '.') {
                return false;
            }

            uint32_t label_len = 0;
            bool numeric_address = true;
            uint32_t address_parts = 1;
            uint32_t address_part = 0;
            uint32_t address_digits = 0;
            for (uint32_t index = 0; index < host_len; ++index) {
                char value = host[index];
                if (value == '.') {
                    if (label_len == 0 || host[index - 1] == '-' ||
                        (numeric_address && (address_digits == 0 || address_part > 255))) {
                        return false;
                    }
                    label_len = 0;
                    ++address_parts;
                    address_part = 0;
                    address_digits = 0;
                    continue;
                }
                if (label_len == 0 && value == '-') {
                    return false;
                }
                if (++label_len > 63) {
                    return false;
                }
                if (value >= '0' && value <= '9') {
                    address_part = address_part * 10 + (uint32_t)(value - '0');
                    ++address_digits;
                } else {
                    numeric_address = false;
                }
            }
            return !numeric_address ||
                (address_parts == 4 && address_digits != 0 && address_part <= 255);
        }
    } // namespace

    netadapter::e_bootstrap_mode __cdecl Get_Router_Bootstrap_Mode(uint32_t mode, uint8_t sync_state) {
        if (mode == 0) {
            if (sync_state == NET_SYNC_STATE_HOST_STARTUP || sync_state == NET_SYNC_STATE_LOAD_GAME_HOST_SETUP) {
                return netadapter::BOOTSTRAP_EMBEDDED_LAN_HOST;
            }
            return netadapter::BOOTSTRAP_LAN_DISCOVERY;
        }

        return netadapter::BOOTSTRAP_DIRECT_ENDPOINT;
    }

    e_router_endpoint_result __cdecl Configure_Router_Endpoint(const char* text, netadapter::s_config* config) {
        if (text == nullptr || text[0] == '\0') {
            return ROUTER_ENDPOINT_MISSING;
        }
        if (config == nullptr) {
            return ROUTER_ENDPOINT_INVALID;
        }

        char host[ROUTER_ADDRESS_SIZE] = {};
        uint32_t host_len = 0;
        const char* cursor = text;
        while (*cursor != '\0' && *cursor != ':' && host_len + 1 < sizeof(host)) {
            if (!Is_Router_Host_Char(*cursor)) {
                return ROUTER_ENDPOINT_INVALID;
            }
            host[host_len++] = *cursor++;
        }
        if (host_len == 0) {
            return ROUTER_ENDPOINT_INVALID;
        }
        if (!Is_Valid_Router_Host(host, host_len)) {
            return ROUTER_ENDPOINT_INVALID;
        }

        uint16_t port = config->tcp_port;
        if (*cursor == ':') {
            ++cursor;
            uint32_t parsed_port = 0;
            if (*cursor == '\0') {
                return ROUTER_ENDPOINT_INVALID;
            }
            while (*cursor != '\0') {
                if (*cursor < '0' || *cursor > '9') {
                    return ROUTER_ENDPOINT_INVALID;
                }
                parsed_port = parsed_port * 10 + (uint32_t)(*cursor - '0');
                if (parsed_port > 65535) {
                    return ROUTER_ENDPOINT_INVALID;
                }
                ++cursor;
            }
            if (parsed_port == 0) {
                return ROUTER_ENDPOINT_INVALID;
            }
            port = (uint16_t)parsed_port;
        } else if (*cursor != '\0') {
            return ROUTER_ENDPOINT_INVALID;
        }

        network::router::Router_Copy_String(config->host, sizeof(config->host), host);
        config->tcp_port = port;
        return ROUTER_ENDPOINT_OK;
    }

    e_router_endpoint_result __cdecl Configure_Router_Endpoint_For_Mode(uint32_t mode, const char* text, netadapter::s_config* config) {
        if (mode != 1) {
            return ROUTER_ENDPOINT_OK;
        }
        return Configure_Router_Endpoint(text, config);
    }
} // namespace netcode