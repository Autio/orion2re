#pragma once

#include "network/router_client.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace network::router {

#define ROUTER_SESSION_BROWSER_DEFAULT_DISCOVERY_TIMEOUT_MS 750U
#define ROUTER_SESSION_BROWSER_DEFAULT_REQUEST_TIMEOUT_MS 1000U
#define ROUTER_SESSION_BROWSER_DEFAULT_CONNECT_TIMEOUT_MS 750U
#define ROUTER_SESSION_BROWSER_MAX_INITIAL_ENDPOINTS 8U

    enum e_router_session_browser_mode {
        ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT,
        ROUTER_SESSION_BROWSER_LAN_DISCOVERY,
    };

    struct s_router_endpoint {
        char host[ROUTER_ADDRESS_SIZE] = "127.0.0.1";
        uint16_t tcp_port = 47800;
        char router_id[ROUTER_ID_SIZE] = {};
        char router_name[ROUTER_NAME_SIZE] = {};
    };

    struct s_router_session_record {
        s_router_endpoint router;
        s_router_session_info session;
    };

    struct s_router_session_browser_config {
        e_router_session_browser_mode mode = ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT;
        char host[ROUTER_ADDRESS_SIZE] = "127.0.0.1";
        uint16_t tcp_port = 47800;
        uint16_t discovery_port = 47800;
        uint32_t discovery_timeout_ms = ROUTER_SESSION_BROWSER_DEFAULT_DISCOVERY_TIMEOUT_MS;
        uint32_t request_timeout_ms = ROUTER_SESSION_BROWSER_DEFAULT_REQUEST_TIMEOUT_MS;
        uint32_t connect_timeout_ms = ROUTER_SESSION_BROWSER_DEFAULT_CONNECT_TIMEOUT_MS;
        char client_name[ROUTER_PLAYER_NAME_SIZE] = "Browser";
        s_router_endpoint initial_endpoints[ROUTER_SESSION_BROWSER_MAX_INITIAL_ENDPOINTS];
        uint32_t initial_endpoint_count = 0;
    };

    struct s_router_session_browser_state;

    struct s_router_session_browser {
        s_router_session_browser();
        ~s_router_session_browser();

        s_router_session_browser(const s_router_session_browser&) = delete;
        s_router_session_browser& operator=(const s_router_session_browser&) = delete;

        std::unique_ptr<s_router_session_browser_state> state;
    };

    void Router_Session_Browser_Init(s_router_session_browser* browser, const s_router_session_browser_config* config);
    void Router_Session_Browser_Shutdown(s_router_session_browser* browser);
    bool Router_Session_Browser_Refresh(s_router_session_browser* browser);
    void Router_Session_Browser_Pump(s_router_session_browser* browser);

    bool Router_Session_Browser_Is_Refreshing(const s_router_session_browser* browser);
    uint32_t Router_Session_Browser_Revision(const s_router_session_browser* browser);
    int Router_Session_Browser_Last_Error(const s_router_session_browser* browser);
    const char* Router_Session_Browser_Last_Error_Message(const s_router_session_browser* browser);
    const std::vector<s_router_session_record>& Router_Session_Browser_Sessions(const s_router_session_browser* browser);

} // namespace network::router