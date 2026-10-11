#pragma once

#include "game/netadapter.h"

namespace netcode {
    enum e_router_endpoint_result {
        ROUTER_ENDPOINT_OK,
        ROUTER_ENDPOINT_MISSING,
        ROUTER_ENDPOINT_INVALID,
    };

    netadapter::e_bootstrap_mode __cdecl Get_Router_Bootstrap_Mode(uint32_t mode, uint8_t sync_state);
    e_router_endpoint_result __cdecl Configure_Router_Endpoint(const char* text, netadapter::s_config* config);
    e_router_endpoint_result __cdecl Configure_Router_Endpoint_For_Mode(uint32_t mode, const char* text, netadapter::s_config* config);
} // namespace netcode