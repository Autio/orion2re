#pragma once

#include "network/router_protocol.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace network::router {

#define ROUTER_DEFAULT_PORT 47800U
#define ROUTER_DEFAULT_MAX_PENDING_WRITE_BYTES (4U * 1024U * 1024U)

    using t_router_log_callback = void (*)(const char* message, void* user_data);

    struct s_router_config {
        char listen_host[ROUTER_ADDRESS_SIZE] = "0.0.0.0";
        uint16_t tcp_port = ROUTER_DEFAULT_PORT;
        bool enable_discovery = true;
        bool enable_signal_handlers = true;
        bool verbose_logging = false;
        uint16_t discovery_port = ROUTER_DEFAULT_PORT;
        char router_id[ROUTER_ID_SIZE] = "local-router";
        char router_name[ROUTER_NAME_SIZE] = "Orion2 Router";
        uint32_t max_frame_size = ROUTER_DEFAULT_MAX_FRAME_SIZE;
        uint32_t max_pending_write_bytes = ROUTER_DEFAULT_MAX_PENDING_WRITE_BYTES;
        t_router_log_callback log_callback = nullptr;
        void* log_user_data = nullptr;
    };

    void Build_Discovery_Reply(const s_router_config* config, uint16_t bound_tcp_port, Bit_Stream* output);

    struct s_router_service_state;

    struct s_router_service {
        s_router_service();
        ~s_router_service();

        s_router_service(const s_router_service&) = delete;
        s_router_service& operator=(const s_router_service&) = delete;

        std::unique_ptr<s_router_service_state> state;
    };

    void Router_Service_Init(s_router_service* service, const s_router_config* config);
    int Router_Service_Run(s_router_service* service);
    void Router_Service_Request_Stop(s_router_service* service);
    bool Router_Service_Is_Running(const s_router_service* service);
    uint16_t Router_Service_Bound_Tcp_Port(const s_router_service* service);
    uint16_t Router_Service_Bound_Discovery_Port(const s_router_service* service);
    const s_router_config* Router_Service_Config(const s_router_service* service);

} // namespace network::router