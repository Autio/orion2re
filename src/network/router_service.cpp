#include "network/router_service.h"

#include "network/router_core.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#include <uv.h>

namespace network::router {

    struct s_router_client_connection;

    struct s_router_write_request {
        uv_write_t request;
        uv_buf_t buffer;
        Bit_Stream data;
        s_router_client_connection* connection = nullptr;
        bool close_after_write = false;
        uint32_t byte_count = 0;
    };

    struct s_router_udp_send_request {
        uv_udp_send_t request;
        uv_buf_t buffer;
        Bit_Stream data;
    };

    struct s_router_service_state {
        s_router_config config;
        s_router_core_state core;
        uv_loop_t loop{};
        uv_tcp_t tcp_server{};
        uv_udp_t udp_socket{};
        uv_async_t stop_async{};
        uv_signal_t sigint{};
        uv_signal_t sigterm{};
        bool loop_initialized = false;
        bool tcp_server_initialized = false;
        bool udp_socket_initialized = false;
        bool stop_async_initialized = false;
        bool sigint_initialized = false;
        bool sigterm_initialized = false;
        bool shutting_down = false;
        t_router_connection_id next_connection_id = 1;
        std::unordered_map<t_router_connection_id, s_router_client_connection*> clients;
        std::atomic_bool running{false};
        std::atomic_bool stop_requested{false};
        std::atomic<uint16_t> bound_tcp_port{0};
        std::atomic<uint16_t> bound_discovery_port{0};
    };

    struct s_router_client_connection {
        s_router_service_state* service = nullptr;
        t_router_connection_id connection_id = 0;
        uv_tcp_t tcp;
        Bit_Stream read_buffer;
        s_router_frame_codec frame_codec;
        uint32_t pending_write_bytes = 0;
        uint64_t bytes_read = 0;
        uint64_t bytes_written = 0;
        uint32_t frames_read = 0;
        uint32_t frames_written = 0;
        char peer_address[ROUTER_ADDRESS_SIZE] = "unknown";
        uint16_t peer_port = 0;
        bool closing = false;
    };

    void Router_Service_Alloc_Callback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer);
    void Router_Service_Tcp_Connection_Callback(uv_stream_t* server, int status);
    void Router_Service_Read_Callback(uv_stream_t* stream, ssize_t bytes_read, const uv_buf_t* buffer);
    void Router_Service_Write_Callback(uv_write_t* request, int status);
    void Router_Service_Client_Close_Callback(uv_handle_t* handle);
    void Router_Service_Stop_Async_Callback(uv_async_t* handle);
    void Router_Service_Signal_Callback(uv_signal_t* handle, int signum);
    void Router_Service_Udp_Recv_Callback(uv_udp_t* handle, ssize_t bytes_read, const uv_buf_t* buffer, const sockaddr* address, unsigned flags);
    void Router_Service_Udp_Send_Callback(uv_udp_send_t* request, int status);

    void Router_Service_Log(s_router_service_state* state, const char* message) {
        if (state == nullptr || message == nullptr) {
            return;
        }
        if (state->config.log_callback != nullptr) {
            state->config.log_callback(message, state->config.log_user_data);
            return;
        }
        std::fprintf(stderr, "orion2-router: %s\n", message);
    }

    void Router_Service_Logf(s_router_service_state* state, const char* format, ...) {
        char message[256]{};
        va_list args;
        va_start(args, format);
        std::vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        Router_Service_Log(state, message);
    }

    void Router_Service_Log_Verbosef(s_router_service_state* state, const char* format, ...) {
        if (state == nullptr || !state->config.verbose_logging) {
            return;
        }

        char message[256]{};
        va_list args;
        va_start(args, format);
        std::vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        Router_Service_Log(state, message);
    }

    const char* Router_Message_Kind_Name(e_router_message_kind kind) {
        switch (kind) {
            case ROUTER_MESSAGE_HELLO: return "HELLO";
            case ROUTER_MESSAGE_WELCOME: return "WELCOME";
            case ROUTER_MESSAGE_REGISTER_SESSION: return "REGISTER_SESSION";
            case ROUTER_MESSAGE_SESSION_REGISTERED: return "SESSION_REGISTERED";
            case ROUTER_MESSAGE_UNREGISTER_SESSION: return "UNREGISTER_SESSION";
            case ROUTER_MESSAGE_LIST_SESSIONS: return "LIST_SESSIONS";
            case ROUTER_MESSAGE_SESSION_LIST: return "SESSION_LIST";
            case ROUTER_MESSAGE_JOIN_SESSION: return "JOIN_SESSION";
            case ROUTER_MESSAGE_JOINED_SESSION: return "JOINED_SESSION";
            case ROUTER_MESSAGE_LEAVE_SESSION: return "LEAVE_SESSION";
            case ROUTER_MESSAGE_LEFT_SESSION: return "LEFT_SESSION";
            case ROUTER_MESSAGE_SEND_TO: return "SEND_TO";
            case ROUTER_MESSAGE_SEND_BROADCAST: return "SEND_BROADCAST";
            case ROUTER_MESSAGE_GAME_DATA: return "GAME_DATA";
            case ROUTER_MESSAGE_OK: return "OK";
            case ROUTER_MESSAGE_ERROR: return "ERROR";
            case ROUTER_MESSAGE_SET_SESSION_OPEN: return "SET_SESSION_OPEN";
            case ROUTER_MESSAGE_MEMBER_LIST: return "MEMBER_LIST";
            case ROUTER_MESSAGE_MEMBER_JOINED: return "MEMBER_JOINED";
            case ROUTER_MESSAGE_MEMBER_LEFT: return "MEMBER_LEFT";
            case ROUTER_MESSAGE_SESSION_CLOSED: return "SESSION_CLOSED";
            case ROUTER_MESSAGE_REMOVE_MEMBER: return "REMOVE_MEMBER";
            default: return "UNKNOWN";
        }
    }

    const s_router_client_state* Router_Service_Core_Client(const s_router_service_state* state, t_router_connection_id connection_id) {
        if (state == nullptr) {
            return nullptr;
        }
        auto client_it = state->core.clients.find(connection_id);
        if (client_it == state->core.clients.end()) {
            return nullptr;
        }
        return &client_it->second;
    }

    void Router_Service_Set_Peer_Info(s_router_client_connection* connection) {
        if (connection == nullptr) {
            return;
        }

        sockaddr_storage address{};
        int address_size = sizeof(address);
        if (uv_tcp_getpeername(&connection->tcp, (sockaddr*)&address, &address_size) != 0) {
            Router_Copy_String(connection->peer_address, sizeof(connection->peer_address), "unknown");
            connection->peer_port = 0;
            return;
        }

        if (address.ss_family == AF_INET) {
            sockaddr_in* ipv4 = (sockaddr_in*)&address;
            uv_ip4_name(ipv4, connection->peer_address, sizeof(connection->peer_address));
            connection->peer_port = (uint16_t)ntohs(ipv4->sin_port);
        } else if (address.ss_family == AF_INET6) {
            sockaddr_in6* ipv6 = (sockaddr_in6*)&address;
            uv_ip6_name(ipv6, connection->peer_address, sizeof(connection->peer_address));
            connection->peer_port = (uint16_t)ntohs(ipv6->sin6_port);
        } else {
            Router_Copy_String(connection->peer_address, sizeof(connection->peer_address), "unknown");
            connection->peer_port = 0;
        }
    }

    bool Router_Service_Log_Error_Output(s_router_service_state* state, const s_router_client_connection* connection, const s_router_message* request, const std::vector<s_router_outbound_message>* output) {
        for (const s_router_outbound_message& outbound : *output) {
            if (outbound.connection_id != connection->connection_id || outbound.message.header.kind != ROUTER_MESSAGE_ERROR) {
                continue;
            }

            s_router_error_payload error;
            std::string parse_error;
            if (Decode_Error_Payload(&outbound.message.payload, &error, &parse_error)) {
                Router_Service_Logf(
                    state,
                    "reject peer=%s:%u conn=%u request=%s code=%u message=\"%s\"",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    Router_Message_Kind_Name(request->header.kind),
                    (unsigned)error.code,
                    error.message);
            } else {
                Router_Service_Logf(
                    state,
                    "reject peer=%s:%u conn=%u request=%s code=? message=\"invalid error payload\"",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    Router_Message_Kind_Name(request->header.kind));
            }
            return true;
        }
        return false;
    }

    void Router_Service_Log_Action(
        s_router_service_state* state,
        const s_router_client_connection* connection,
        const s_router_message* request,
        const std::vector<s_router_outbound_message>* output,
        uint32_t before_network_id,
        uint32_t before_session_id) {
        if (state == nullptr || connection == nullptr || request == nullptr || output == nullptr) {
            return;
        }
        if (Router_Service_Log_Error_Output(state, connection, request, output)) {
            return;
        }

        const s_router_client_state* client = Router_Service_Core_Client(state, connection->connection_id);
        const uint32_t network_id = client != nullptr ? client->network_id : before_network_id;
        const uint32_t session_id = client != nullptr ? client->session_id : before_session_id;

        switch (request->header.kind) {
            case ROUTER_MESSAGE_HELLO:
                if (client != nullptr && client->greeted) {
                    Router_Service_Logf(
                        state,
                        "client hello peer=%s:%u conn=%u network=%u name=\"%s\" version=%u router=\"%s\"",
                        connection->peer_address,
                        (unsigned)connection->peer_port,
                        (unsigned)connection->connection_id,
                        (unsigned)network_id,
                        client->client_name,
                        (unsigned)client->protocol_version,
                        state->core.router_id);
                }
                break;
            case ROUTER_MESSAGE_LIST_SESSIONS: {
                std::vector<s_router_session_info> sessions;
                Router_Core_List_Open_Sessions(&state->core, &sessions);
                Router_Service_Log_Verbosef(
                    state,
                    "session list peer=%s:%u conn=%u network=%u count=%u",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)sessions.size());
                break;
            }
            case ROUTER_MESSAGE_REGISTER_SESSION: {
                const auto session_it = state->core.sessions.find(session_id);
                if (session_it != state->core.sessions.end()) {
                    const s_router_session_info& info = session_it->second.info;
                    Router_Service_Logf(
                        state,
                        "session created peer=%s:%u conn=%u network=%u session=%u name=\"%s\" host=\"%s\" players=%u/%u build=\"%s\" open=%u",
                        connection->peer_address,
                        (unsigned)connection->peer_port,
                        (unsigned)connection->connection_id,
                        (unsigned)network_id,
                        (unsigned)info.session_id,
                        info.session_name,
                        info.host_name,
                        (unsigned)info.current_players,
                        (unsigned)info.max_players,
                        info.build_string,
                        info.open ? 1U : 0U);
                }
                break;
            }
            case ROUTER_MESSAGE_JOIN_SESSION: {
                const auto session_it = state->core.sessions.find(session_id);
                if (session_it != state->core.sessions.end()) {
                    const s_router_session_info& info = session_it->second.info;
                    Router_Service_Logf(
                        state,
                        "member joined peer=%s:%u conn=%u network=%u session=%u name=\"%s\" players=%u/%u",
                        connection->peer_address,
                        (unsigned)connection->peer_port,
                        (unsigned)connection->connection_id,
                        (unsigned)network_id,
                        (unsigned)info.session_id,
                        client != nullptr ? client->client_name : "",
                        (unsigned)info.current_players,
                        (unsigned)info.max_players);
                }
                break;
            }
            case ROUTER_MESSAGE_LEAVE_SESSION:
                Router_Service_Logf(
                    state,
                    "member left peer=%s:%u conn=%u network=%u session=%u reason=leave",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)before_network_id,
                    (unsigned)before_session_id);
                break;
            case ROUTER_MESSAGE_REMOVE_MEMBER:
                Router_Service_Logf(
                    state,
                    "member removed by owner network=%u session=%u target=%u",
                    (unsigned)network_id,
                    (unsigned)before_session_id,
                    (unsigned)request->header.target_network_id);
                break;
            case ROUTER_MESSAGE_UNREGISTER_SESSION:
                Router_Service_Logf(
                    state,
                    "session closed peer=%s:%u conn=%u network=%u session=%u reason=unregister",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)(request->header.session_id != 0 ? request->header.session_id : before_session_id));
                break;
            case ROUTER_MESSAGE_SET_SESSION_OPEN: {
                const bool open = (request->header.flags & ROUTER_FLAG_SESSION_OPEN) != 0;
                const auto session_it = state->core.sessions.find(request->header.session_id != 0 ? request->header.session_id : session_id);
                const uint32_t logged_session_id = session_it != state->core.sessions.end() ? session_it->second.info.session_id : (request->header.session_id != 0 ? request->header.session_id : session_id);
                Router_Service_Logf(
                    state,
                    open ? "session reopened peer=%s:%u conn=%u network=%u session=%u" : "session locked/game-starting peer=%s:%u conn=%u network=%u session=%u",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)logged_session_id);
                break;
            }
            case ROUTER_MESSAGE_SEND_TO: {
                const s_router_frame_codec_stats compression = connection->frame_codec.Stats();
                Router_Service_Log_Verbosef(
                    state,
                    "game data direct peer=%s:%u conn=%u network=%u session=%u target=%u bytes=%u compressed_bytes=%u compression_saved=%d",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)session_id,
                    (unsigned)request->header.target_network_id,
                    (unsigned)compression.last_uncompressed_payload_bytes_in,
                    (unsigned)compression.last_compressed_payload_bytes_in,
                    (int)(compression.last_uncompressed_payload_bytes_in - compression.last_compressed_payload_bytes_in));
                break;
            }
            case ROUTER_MESSAGE_SEND_BROADCAST: {
                const s_router_frame_codec_stats compression = connection->frame_codec.Stats();
                Router_Service_Log_Verbosef(
                    state,
                    "game data broadcast peer=%s:%u conn=%u network=%u session=%u recipients=%u bytes=%u compressed_bytes=%u compression_saved=%d include_self=%u",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)session_id,
                    (unsigned)output->size(),
                    (unsigned)compression.last_uncompressed_payload_bytes_in,
                    (unsigned)compression.last_compressed_payload_bytes_in,
                    (int)(compression.last_uncompressed_payload_bytes_in - compression.last_compressed_payload_bytes_in),
                    (request->header.flags & ROUTER_FLAG_BROADCAST_INCLUDE_SENDER) != 0 ? 1U : 0U);
                break;
            }
            default:
                Router_Service_Log_Verbosef(
                    state,
                    "message peer=%s:%u conn=%u network=%u session=%u kind=%s bytes=%u outputs=%u",
                    connection->peer_address,
                    (unsigned)connection->peer_port,
                    (unsigned)connection->connection_id,
                    (unsigned)network_id,
                    (unsigned)session_id,
                    Router_Message_Kind_Name(request->header.kind),
                    (unsigned)request->payload.Size(),
                    (unsigned)output->size());
                break;
        }
    }

    void Router_Service_Close_Handle(uv_handle_t* handle) {
        if (handle != nullptr && !uv_is_closing(handle)) {
            uv_close(handle, nullptr);
        }
    }

    int Router_Service_Start_Stop_Async(s_router_service_state* state) {
        const int result = uv_async_init(&state->loop, &state->stop_async, Router_Service_Stop_Async_Callback);
        if (result != 0) {
            return result;
        }
        state->stop_async.data = state;
        state->stop_async_initialized = true;
        return 0;
    }

    int Router_Service_Start_Signals(s_router_service_state* state) {
        int result = uv_signal_init(&state->loop, &state->sigint);
        if (result != 0) {
            return result;
        }
        state->sigint.data = state;
        state->sigint_initialized = true;
        result = uv_signal_start(&state->sigint, Router_Service_Signal_Callback, SIGINT);
        if (result != 0) {
            return result;
        }

        result = uv_signal_init(&state->loop, &state->sigterm);
        if (result != 0) {
            return result;
        }
        state->sigterm.data = state;
        state->sigterm_initialized = true;
        return uv_signal_start(&state->sigterm, Router_Service_Signal_Callback, SIGTERM);
    }

    int Router_Service_Start_Tcp_Listener(s_router_service_state* state) {
        int result = uv_tcp_init(&state->loop, &state->tcp_server);
        if (result != 0) {
            return result;
        }
        state->tcp_server.data = state;
        state->tcp_server_initialized = true;

        sockaddr_in address{};
        result = uv_ip4_addr(state->config.listen_host, state->config.tcp_port, &address);
        if (result != 0) {
            return result;
        }

        result = uv_tcp_bind(&state->tcp_server, (const sockaddr*)&address, 0);
        if (result != 0) {
            return result;
        }

        result = uv_listen((uv_stream_t*)&state->tcp_server, 128, Router_Service_Tcp_Connection_Callback);
        if (result != 0) {
            return result;
        }

        sockaddr_in bound_address{};
        int bound_address_size = sizeof(bound_address);
        result = uv_tcp_getsockname(&state->tcp_server, (sockaddr*)&bound_address, &bound_address_size);
        if (result != 0) {
            return result;
        }
        state->bound_tcp_port.store((uint16_t)ntohs(bound_address.sin_port));
        return 0;
    }

    int Router_Service_Start_Udp_Discovery(s_router_service_state* state) {
        int result = uv_udp_init(&state->loop, &state->udp_socket);
        if (result != 0) {
            return result;
        }
        state->udp_socket.data = state;
        state->udp_socket_initialized = true;

        sockaddr_in address{};
        result = uv_ip4_addr("0.0.0.0", state->config.discovery_port, &address);
        if (result != 0) {
            return result;
        }

        result = uv_udp_bind(&state->udp_socket, (const sockaddr*)&address, UV_UDP_REUSEADDR);
        if (result != 0) {
            return result;
        }

        sockaddr_in bound_address{};
        int bound_address_size = sizeof(bound_address);
        result = uv_udp_getsockname(&state->udp_socket, (sockaddr*)&bound_address, &bound_address_size);
        if (result != 0) {
            return result;
        }
        state->bound_discovery_port.store((uint16_t)ntohs(bound_address.sin_port));

        return uv_udp_recv_start(&state->udp_socket, Router_Service_Alloc_Callback, Router_Service_Udp_Recv_Callback);
    }

    void Router_Service_Send_Output(s_router_service_state* state, const s_router_outbound_message* output);
    void Router_Service_Send_Protocol_Error_And_Close(s_router_service_state* state, s_router_client_connection* connection, const char* message);

    void Router_Service_Send_Outputs(s_router_service_state* state, const std::vector<s_router_outbound_message>* output) {
        for (const s_router_outbound_message& message : *output) {
            Router_Service_Send_Output(state, &message);
        }
    }

    void Router_Service_Close_Client(s_router_service_state* state, s_router_client_connection* connection, bool notify_core) {
        if (connection == nullptr || connection->closing) {
            return;
        }

        const s_router_client_state* client = Router_Service_Core_Client(state, connection->connection_id);
        const uint32_t network_id = client != nullptr ? client->network_id : 0;
        const uint32_t session_id = client != nullptr ? client->session_id : 0;
        if (notify_core) {
            Router_Service_Logf(
                state,
                "connection dropped peer=%s:%u conn=%u network=%u session=%u",
                connection->peer_address,
                (unsigned)connection->peer_port,
                (unsigned)connection->connection_id,
                (unsigned)network_id,
                (unsigned)session_id);
        }
        Router_Service_Log_Verbosef(
            state,
            "connection closing peer=%s:%u conn=%u network=%u session=%u notify=%u",
            connection->peer_address,
            (unsigned)connection->peer_port,
            (unsigned)connection->connection_id,
            (unsigned)network_id,
            (unsigned)session_id,
            notify_core ? 1U : 0U);

        connection->closing = true;
        uv_read_stop((uv_stream_t*)&connection->tcp);
        std::vector<s_router_outbound_message> output;
        Router_Core_Close_Connection(&state->core, connection->connection_id, &output);
        if (session_id != 0) {
            Router_Service_Logf(
                state,
                "member left peer=%s:%u conn=%u network=%u session=%u reason=%s",
                connection->peer_address,
                (unsigned)connection->peer_port,
                (unsigned)connection->connection_id,
                (unsigned)network_id,
                (unsigned)session_id,
                notify_core ? "disconnect" : "shutdown");
            if (state->core.sessions.find(session_id) == state->core.sessions.end()) {
                Router_Service_Logf(state, "session deleted session=%u reason=last-member-left", (unsigned)session_id);
            }
        }
        if (notify_core) {
            Router_Service_Send_Outputs(state, &output);
        }
        uv_close((uv_handle_t*)&connection->tcp, Router_Service_Client_Close_Callback);
    }

    void Router_Service_Begin_Shutdown(s_router_service_state* state) {
        if (state->shutting_down) {
            return;
        }
        state->shutting_down = true;

        Router_Service_Logf(state, "router shutting down clients=%u sessions=%u", (unsigned)state->clients.size(), (unsigned)state->core.sessions.size());

        std::vector<s_router_client_connection*> clients;
        clients.reserve(state->clients.size());
        for (auto& entry : state->clients) {
            clients.push_back(entry.second);
        }

        for (s_router_client_connection* connection : clients) {
            Router_Service_Close_Client(state, connection, false);
        }

        if (state->tcp_server_initialized) {
            Router_Service_Close_Handle((uv_handle_t*)&state->tcp_server);
        }
        if (state->udp_socket_initialized) {
            Router_Service_Close_Handle((uv_handle_t*)&state->udp_socket);
        }
        if (state->sigint_initialized) {
            uv_signal_stop(&state->sigint);
            Router_Service_Close_Handle((uv_handle_t*)&state->sigint);
        }
        if (state->sigterm_initialized) {
            uv_signal_stop(&state->sigterm);
            Router_Service_Close_Handle((uv_handle_t*)&state->sigterm);
        }
        if (state->stop_async_initialized) {
            Router_Service_Close_Handle((uv_handle_t*)&state->stop_async);
        }
    }

    void Router_Service_Send_Output(s_router_service_state* state, const s_router_outbound_message* output) {
        auto connection_it = state->clients.find(output->connection_id);
        if (connection_it == state->clients.end() || connection_it->second->closing) {
            return;
        }

        s_router_write_request* write_request = new s_router_write_request{};

        s_router_client_connection* connection = connection_it->second;
        const uint32_t max_encoded_size = Encoded_Frame_Max_Size(&output->message);
        if (max_encoded_size > state->config.max_pending_write_bytes ||
            connection->pending_write_bytes > state->config.max_pending_write_bytes - max_encoded_size) {
            Router_Service_Logf(
                state,
                "dropping outbound router message for connection %u because pending write bytes would exceed %u",
                connection->connection_id,
                state->config.max_pending_write_bytes);
            delete write_request;
            return;
        }

        Encode_Frame(&output->message, &connection->frame_codec, &write_request->data);
        write_request->byte_count = write_request->data.Size();

        connection->pending_write_bytes += write_request->byte_count;
        connection->bytes_written += write_request->byte_count;
        connection->frames_written++;

        write_request->buffer = uv_buf_init((char*)write_request->data.Data(), write_request->data.Size());
        write_request->request.data = write_request;
        write_request->connection = connection;

        const int result = uv_write(&write_request->request, (uv_stream_t*)&connection->tcp, &write_request->buffer, 1, Router_Service_Write_Callback);
        if (result != 0) {
            connection->pending_write_bytes -= write_request->byte_count;
            Router_Service_Logf(state, "closing connection %u after uv_write failed with %u", connection->connection_id, (uint32_t)(-result));
            delete write_request;
            Router_Service_Close_Client(state, connection, true);
        }
    }

    void Router_Service_Send_Protocol_Error_And_Close(s_router_service_state* state, s_router_client_connection* connection, const char* message) {
        if (connection == nullptr || connection->closing) {
            return;
        }

        s_router_message error_message;
        error_message.header.kind = ROUTER_MESSAGE_ERROR;
        error_message.header.target_network_id = Router_Core_Client_Network_Id(&state->core, connection->connection_id);
        Encode_Error_Payload(ROUTER_ERROR_INVALID_MESSAGE, message != nullptr ? message : "protocol error", &error_message.payload);

        Router_Service_Log(state, message != nullptr ? message : "protocol error; closing connection");

        connection->closing = true;
        uv_read_stop((uv_stream_t*)&connection->tcp);

        std::vector<s_router_outbound_message> output;
        Router_Core_Close_Connection(&state->core, connection->connection_id, &output);
        Router_Service_Send_Outputs(state, &output);

        s_router_write_request* write_request = new s_router_write_request{};
        Encode_Frame(&error_message, &connection->frame_codec, &write_request->data);
        write_request->byte_count = write_request->data.Size();
        write_request->buffer = uv_buf_init((char*)write_request->data.Data(), write_request->data.Size());
        write_request->request.data = write_request;
        write_request->connection = connection;
        write_request->close_after_write = true;

        const int result = uv_write(&write_request->request, (uv_stream_t*)&connection->tcp, &write_request->buffer, 1, Router_Service_Write_Callback);
        if (result != 0) {
            delete write_request;
            uv_close((uv_handle_t*)&connection->tcp, Router_Service_Client_Close_Callback);
        }
    }

    void Router_Service_Send_Discovery_Reply(s_router_service_state* state, const sockaddr* address) {
        if (address == nullptr || !state->udp_socket_initialized || uv_is_closing((uv_handle_t*)&state->udp_socket)) {
            return;
        }

        s_router_udp_send_request* send_request = new s_router_udp_send_request{};
        Build_Discovery_Reply(&state->config, state->bound_tcp_port.load(), &send_request->data);
        send_request->buffer = uv_buf_init((char*)send_request->data.Data(), send_request->data.Size());
        send_request->request.data = send_request;

        const int result = uv_udp_send(&send_request->request, &state->udp_socket, &send_request->buffer, 1, address, Router_Service_Udp_Send_Callback);
        if (result != 0) {
            delete send_request;
        }
    }

    void Router_Service_Alloc_Callback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer) {
        (void)handle;
        const size_t allocation_size = suggested_size == 0 ? 4096 : suggested_size;
        buffer->base = (char*)std::malloc(allocation_size);
        buffer->len = (unsigned int)allocation_size;
    }

    void Router_Service_Tcp_Connection_Callback(uv_stream_t* server, int status) {
        s_router_service_state* state = (s_router_service_state*)server->data;
        if (status != 0 || state == nullptr || state->shutting_down) {
            return;
        }

        s_router_client_connection* connection = new s_router_client_connection{};
        connection->service = state;
        connection->connection_id = state->next_connection_id++;

        int result = uv_tcp_init(&state->loop, &connection->tcp);
        if (result != 0) {
            delete connection;
            return;
        }
        connection->tcp.data = connection;

        result = uv_accept(server, (uv_stream_t*)&connection->tcp);
        if (result != 0) {
            uv_close((uv_handle_t*)&connection->tcp, Router_Service_Client_Close_Callback);
            return;
        }

        Router_Service_Set_Peer_Info(connection);
        uv_tcp_nodelay(&connection->tcp, 1);
        state->clients[connection->connection_id] = connection;
        Router_Core_Open_Connection(&state->core, connection->connection_id);
        Router_Service_Logf(
            state,
            "connection accepted peer=%s:%u conn=%u",
            connection->peer_address,
            (unsigned)connection->peer_port,
            (unsigned)connection->connection_id);

        result = uv_read_start((uv_stream_t*)&connection->tcp, Router_Service_Alloc_Callback, Router_Service_Read_Callback);
        if (result != 0) {
            Router_Service_Close_Client(state, connection, true);
        }
    }

    void Router_Service_Read_Callback(uv_stream_t* stream, ssize_t bytes_read, const uv_buf_t* buffer) {
        s_router_client_connection* connection = (s_router_client_connection*)stream->data;
        if (connection == nullptr || connection->service == nullptr) {
            if (buffer != nullptr) {
                std::free(buffer->base);
            }
            return;
        }

        s_router_service_state* state = connection->service;
        if (bytes_read > 0) {
            const uint8_t* bytes = (const uint8_t*)buffer->base;
            connection->bytes_read += (uint64_t)bytes_read;
            connection->read_buffer.Write_Bytes(bytes, (uint32_t)bytes_read);

            for (;;) {
                s_router_message message;
                std::string error_message;
                const e_frame_read_status status = Try_Read_Frame(&connection->read_buffer, state->config.max_frame_size, &connection->frame_codec, &message, &error_message);
                if (status == FRAME_READ_NEED_MORE_DATA) {
                    break;
                }
                if (status == FRAME_READ_PROTOCOL_ERROR) {
                    Router_Service_Send_Protocol_Error_And_Close(state, connection, error_message.c_str());
                    break;
                }

                std::vector<s_router_outbound_message> output;
                connection->frames_read++;
                const s_router_client_state* before_client = Router_Service_Core_Client(state, connection->connection_id);
                const uint32_t before_network_id = before_client != nullptr ? before_client->network_id : 0;
                const uint32_t before_session_id = before_client != nullptr ? before_client->session_id : 0;
                Router_Core_Handle_Message(&state->core, connection->connection_id, &message, &output);
                Router_Service_Log_Action(state, connection, &message, &output, before_network_id, before_session_id);
                Router_Service_Send_Outputs(state, &output);
                if (connection->closing) {
                    break;
                }
            }
        } else if (bytes_read < 0) {
            Router_Service_Close_Client(state, connection, true);
        }

        if (buffer != nullptr) {
            std::free(buffer->base);
        }
    }

    void Router_Service_Write_Callback(uv_write_t* request, int status) {
        s_router_write_request* write_request = (s_router_write_request*)request->data;
        if (write_request != nullptr && write_request->connection != nullptr && write_request->byte_count <= write_request->connection->pending_write_bytes) {
            write_request->connection->pending_write_bytes -= write_request->byte_count;
        }
        if (write_request != nullptr && write_request->connection != nullptr && write_request->close_after_write) {
            if (!uv_is_closing((uv_handle_t*)&write_request->connection->tcp)) {
                uv_close((uv_handle_t*)&write_request->connection->tcp, Router_Service_Client_Close_Callback);
            }
        } else if (status != 0 && write_request != nullptr && write_request->connection != nullptr) {
            s_router_client_connection* connection = write_request->connection;
            if (connection->service != nullptr) {
                Router_Service_Close_Client(connection->service, connection, true);
            }
        }
        delete write_request;
    }

    void Router_Service_Client_Close_Callback(uv_handle_t* handle) {
        s_router_client_connection* connection = (s_router_client_connection*)handle->data;
        if (connection == nullptr) {
            return;
        }

        s_router_service_state* state = connection->service;
        if (state != nullptr) {
            const s_router_frame_codec_stats compression = connection->frame_codec.Stats();
            Router_Service_Log_Verbosef(
                state,
                "traffic peer=%s:%u conn=%u frames_in=%u frames_out=%u bytes_in=%llu bytes_out=%llu compressed_payload_in=%llu/%llu compressed_payload_out=%llu/%llu compression_saved_in=%lld compression_saved_out=%lld",
                connection->peer_address,
                (unsigned)connection->peer_port,
                (unsigned)connection->connection_id,
                (unsigned)connection->frames_read,
                (unsigned)connection->frames_written,
                (unsigned long long)connection->bytes_read,
                (unsigned long long)connection->bytes_written,
                (unsigned long long)compression.compressed_payload_bytes_in,
                (unsigned long long)compression.uncompressed_payload_bytes_in,
                (unsigned long long)compression.compressed_payload_bytes_out,
                (unsigned long long)compression.uncompressed_payload_bytes_out,
                (long long)(compression.uncompressed_payload_bytes_in - compression.compressed_payload_bytes_in),
                (long long)(compression.uncompressed_payload_bytes_out - compression.compressed_payload_bytes_out));
            Router_Service_Log_Verbosef(
                state,
                "connection closed peer=%s:%u conn=%u",
                connection->peer_address,
                (unsigned)connection->peer_port,
                (unsigned)connection->connection_id);
            state->clients.erase(connection->connection_id);
        }
        delete connection;
    }

    void Router_Service_Stop_Async_Callback(uv_async_t* handle) {
        s_router_service_state* state = (s_router_service_state*)handle->data;
        if (state != nullptr) {
            Router_Service_Begin_Shutdown(state);
        }
    }

    void Router_Service_Signal_Callback(uv_signal_t* handle, int signum) {
        (void)signum;
        s_router_service_state* state = (s_router_service_state*)handle->data;
        if (state != nullptr) {
            Router_Service_Begin_Shutdown(state);
        }
    }

    void Router_Service_Udp_Recv_Callback(uv_udp_t* handle, ssize_t bytes_read, const uv_buf_t* buffer, const sockaddr* address, unsigned flags) {
        (void)flags;
        s_router_service_state* state = (s_router_service_state*)handle->data;
        if (state != nullptr && bytes_read > 0 && address != nullptr) {
            if (Is_Discovery_Probe((const uint8_t*)buffer->base, (uint32_t)bytes_read)) {
                char address_text[ROUTER_ADDRESS_SIZE] = "unknown";
                uint16_t port = 0;
                if (address->sa_family == AF_INET) {
                    sockaddr_in* ipv4 = (sockaddr_in*)address;
                    uv_ip4_name(ipv4, address_text, sizeof(address_text));
                    port = (uint16_t)ntohs(ipv4->sin_port);
                } else if (address->sa_family == AF_INET6) {
                    sockaddr_in6* ipv6 = (sockaddr_in6*)address;
                    uv_ip6_name(ipv6, address_text, sizeof(address_text));
                    port = (uint16_t)ntohs(ipv6->sin6_port);
                }
                Router_Service_Log_Verbosef(state, "discovery probe peer=%s:%u", address_text, (unsigned)port);
                Router_Service_Send_Discovery_Reply(state, address);
            }
        }

        if (buffer != nullptr) {
            std::free(buffer->base);
        }
    }

    void Router_Service_Udp_Send_Callback(uv_udp_send_t* request, int status) {
        (void)status;
        s_router_udp_send_request* send_request = (s_router_udp_send_request*)request->data;
        delete send_request;
    }

    void Build_Discovery_Reply(const s_router_config* config, uint16_t bound_tcp_port, Bit_Stream* output) {
        Encode_Discovery_Reply(
            bound_tcp_port,
            config != nullptr ? config->router_id : "",
            config != nullptr ? config->router_name : "",
            output);
    }

    s_router_service::s_router_service()
        : state(std::make_unique<s_router_service_state>()) {
    }

    s_router_service::~s_router_service() = default;

    void Router_Service_Init(s_router_service* service, const s_router_config* config) {
        if (service == nullptr) {
            return;
        }

        if (service->state == nullptr) {
            service->state = std::make_unique<s_router_service_state>();
        }

        service->state->config = config != nullptr ? *config : s_router_config{};
        Router_Core_Init(&service->state->core, service->state->config.router_id);
    }

    int Router_Service_Run(s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return UV_EINVAL;
        }

        s_router_service_state* state = service->state.get();
        bool expected = false;
        if (!state->running.compare_exchange_strong(expected, true)) {
            return UV_EBUSY;
        }

        state->stop_requested.store(false);
        state->bound_tcp_port.store(0);
        state->shutting_down = false;

        int result = uv_loop_init(&state->loop);
        if (result != 0) {
            state->running.store(false);
            return result;
        }
        state->loop_initialized = true;

        result = Router_Service_Start_Stop_Async(state);
        if (result == 0 && state->config.enable_signal_handlers) {
            result = Router_Service_Start_Signals(state);
        }
        if (result == 0) {
            result = Router_Service_Start_Tcp_Listener(state);
        }
        if (result == 0 && state->config.enable_discovery) {
            result = Router_Service_Start_Udp_Discovery(state);
        }

        if (result == 0) {
            Router_Service_Logf(
                state,
                "router started id=\"%s\" listen=%s:%u discovery=%u discovery_port=%u",
                state->config.router_id,
                state->config.listen_host,
                (unsigned)state->bound_tcp_port.load(),
                state->config.enable_discovery ? 1U : 0U,
                (unsigned)state->bound_discovery_port.load());
        }

        if (result != 0 || state->stop_requested.load()) {
            Router_Service_Begin_Shutdown(state);
        }

        uv_run(&state->loop, UV_RUN_DEFAULT);

        while (uv_loop_close(&state->loop) == UV_EBUSY) {
            uv_run(&state->loop, UV_RUN_NOWAIT);
        }

        state->loop_initialized = false;
        state->tcp_server_initialized = false;
        state->udp_socket_initialized = false;
        state->stop_async_initialized = false;
        state->sigint_initialized = false;
        state->sigterm_initialized = false;
        state->clients.clear();
        state->bound_tcp_port.store(0);
        state->bound_discovery_port.store(0);
        state->running.store(false);
        return result;
    }

    void Router_Service_Request_Stop(s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return;
        }

        service->state->stop_requested.store(true);
        if (service->state->stop_async_initialized) {
            uv_async_send(&service->state->stop_async);
        }
    }

    bool Router_Service_Is_Running(const s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return false;
        }
        return service->state->running.load();
    }

    uint16_t Router_Service_Bound_Tcp_Port(const s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return 0;
        }
        return service->state->bound_tcp_port.load();
    }

    uint16_t Router_Service_Bound_Discovery_Port(const s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return 0;
        }
        return service->state->bound_discovery_port.load();
    }

    const s_router_config* Router_Service_Config(const s_router_service* service) {
        if (service == nullptr || service->state == nullptr) {
            return nullptr;
        }
        return &service->state->config;
    }

} // namespace network::router