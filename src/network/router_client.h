#pragma once

#include "network/router_protocol.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace network::router {

#define ROUTER_CLIENT_DEFAULT_CONNECT_TIMEOUT_MS 5000U
#define ROUTER_CLIENT_DEFAULT_MAX_PENDING_WRITE_BYTES (4U * 1024U * 1024U)

    enum e_router_client_event_kind {
        ROUTER_CLIENT_EVENT_MESSAGE,
        ROUTER_CLIENT_EVENT_WELCOME,
        ROUTER_CLIENT_EVENT_SESSION_LIST,
        ROUTER_CLIENT_EVENT_SESSION_REGISTERED,
        ROUTER_CLIENT_EVENT_JOINED_SESSION,
        ROUTER_CLIENT_EVENT_LEFT_SESSION,
        ROUTER_CLIENT_EVENT_MEMBER_LIST,
        ROUTER_CLIENT_EVENT_MEMBER_JOINED,
        ROUTER_CLIENT_EVENT_MEMBER_LEFT,
        ROUTER_CLIENT_EVENT_SESSION_CLOSED,
        ROUTER_CLIENT_EVENT_GAME_DATA,
        ROUTER_CLIENT_EVENT_OK,
        ROUTER_CLIENT_EVENT_ERROR,
        ROUTER_CLIENT_EVENT_PROTOCOL_ERROR,
        ROUTER_CLIENT_EVENT_DISCONNECTED,
    };

    struct s_router_client_config {
        char host[ROUTER_ADDRESS_SIZE] = "127.0.0.1";
        uint16_t tcp_port = 47800;
        uint32_t max_frame_size = ROUTER_DEFAULT_MAX_FRAME_SIZE;
        uint32_t max_pending_write_bytes = ROUTER_CLIENT_DEFAULT_MAX_PENDING_WRITE_BYTES;
        uint32_t connect_timeout_ms = ROUTER_CLIENT_DEFAULT_CONNECT_TIMEOUT_MS;
    };

    struct s_router_client_event {
        e_router_client_event_kind kind = ROUTER_CLIENT_EVENT_MESSAGE;
        s_router_message message;
        s_router_error_payload error;
        s_router_session_info session;
        s_router_member_event_payload member_event;
        std::string reason;
        std::vector<s_router_session_info> sessions;
        std::vector<s_router_member_info> members;
    };

    struct s_router_client_state;

    struct s_router_client_stats {
        uint64_t wire_bytes_in = 0;
        uint64_t wire_bytes_out = 0;
        uint64_t frames_in = 0;
        uint64_t frames_out = 0;
        uint64_t failed_write_bytes = 0;
        uint64_t failed_write_frames = 0;
        uint32_t pending_write_bytes = 0;
        s_router_frame_codec_stats compression;
    };

    struct s_router_client {
        s_router_client();
        ~s_router_client();

        s_router_client(const s_router_client&) = delete;
        s_router_client& operator=(const s_router_client&) = delete;

        std::unique_ptr<s_router_client_state> state;
    };

    void Router_Client_Init(s_router_client* client, const s_router_client_config* config);
    bool Router_Client_Connect(s_router_client* client);
    void Router_Client_Disconnect(s_router_client* client);
    int Router_Client_Pump(s_router_client* client);

    bool Router_Client_Is_Connected(const s_router_client* client);
    int Router_Client_Last_Error(const s_router_client* client);
    const char* Router_Client_Last_Error_Message(const s_router_client* client);
    uint32_t Router_Client_Local_Network_Id(const s_router_client* client);
    uint32_t Router_Client_Session_Id(const s_router_client* client);
    const char* Router_Client_Router_Id(const s_router_client* client);
    const std::vector<s_router_session_info>& Router_Client_Sessions(const s_router_client* client);
    const std::vector<s_router_member_info>& Router_Client_Members(const s_router_client* client);
    s_router_client_stats Router_Client_Stats(const s_router_client* client);
    size_t Router_Client_Event_Count(const s_router_client* client);
    bool Router_Client_Pop_Event(s_router_client* client, s_router_client_event* event);

    bool Router_Client_Send_Message(s_router_client* client, const s_router_message* message);
    bool Router_Client_Send_Hello(s_router_client* client, const char* client_name, uint32_t protocol_version, uint32_t request_id);
    bool Router_Client_Request_Session_List(s_router_client* client, uint32_t request_id);
    bool Router_Client_Register_Session(s_router_client* client, const s_router_session_registration_payload* registration, uint32_t request_id);
    bool Router_Client_Unregister_Session(s_router_client* client, uint32_t session_id, uint32_t request_id);
    bool Router_Client_Set_Session_Open(s_router_client* client, uint32_t session_id, bool open, uint32_t request_id);
    bool Router_Client_Join_Session(s_router_client* client, uint32_t session_id, const char* build_string, uint32_t request_id);
    bool Router_Client_Leave_Session(s_router_client* client, uint32_t request_id);
    bool Router_Client_Remove_Member(s_router_client* client, uint32_t target_network_id, const char* reason, uint32_t request_id);
    bool Router_Client_Send_To(s_router_client* client, uint32_t target_network_id, const uint8_t* payload, uint32_t payload_size, uint32_t request_id);
    bool Router_Client_Send_Broadcast(s_router_client* client, const uint8_t* payload, uint32_t payload_size, uint32_t flags, uint32_t request_id);

} // namespace network::router