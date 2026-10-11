#include "network/router_client.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <thread>
#include <utility>

#include <uv.h>

namespace network::router {

    struct s_router_client_write_request {
        uv_write_t request;
        uv_buf_t buffer;
        Bit_Stream data;
        s_router_client_state* client = nullptr;
        uint32_t byte_count = 0;
    };

    struct s_router_client_state {
        s_router_client_config config;
        uv_loop_t loop{};
        uv_tcp_t tcp{};
        uv_connect_t connect_request{};
        bool loop_initialized = false;
        bool tcp_initialized = false;
        bool tcp_closed = true;
        bool closing = false;
        bool connected = false;
        bool connect_done = false;
        bool disconnected_event_sent = false;
        int connect_status = 0;
        int last_error = 0;
        uint32_t pending_write_bytes = 0;
        uint64_t wire_bytes_in = 0;
        uint64_t wire_bytes_out = 0;
        uint64_t frames_in = 0;
        uint64_t frames_out = 0;
        uint64_t failed_write_bytes = 0;
        uint64_t failed_write_frames = 0;
        uint32_t local_network_id = 0;
        uint32_t session_id = 0;
        char router_id[ROUTER_ID_SIZE] = {};
        Bit_Stream read_buffer;
        s_router_frame_codec frame_codec;
        std::deque<s_router_client_event> events;
        std::vector<s_router_session_info> sessions;
        std::vector<s_router_member_info> members;
    };

    void Router_Client_Alloc_Callback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer);
    void Router_Client_Connect_Callback(uv_connect_t* request, int status);
    void Router_Client_Read_Callback(uv_stream_t* stream, ssize_t bytes_read, const uv_buf_t* buffer);
    void Router_Client_Write_Callback(uv_write_t* request, int status);
    void Router_Client_Close_Callback(uv_handle_t* handle);

    const std::vector<s_router_session_info>& Empty_Sessions() {
        static const std::vector<s_router_session_info> empty;
        return empty;
    }

    const std::vector<s_router_member_info>& Empty_Members() {
        static const std::vector<s_router_member_info> empty;
        return empty;
    }

    void Push_Disconnected_Event(s_router_client_state* state) {
        if (state == nullptr || state->disconnected_event_sent) {
            return;
        }
        state->disconnected_event_sent = true;
        s_router_client_event event;
        event.kind = ROUTER_CLIENT_EVENT_DISCONNECTED;
        state->events.push_back(std::move(event));
    }

    void Router_Client_Begin_Close(s_router_client_state* state, bool queue_disconnected_event) {
        if (state == nullptr || !state->tcp_initialized || state->tcp_closed || state->closing) {
            return;
        }

        state->closing = true;
        state->connected = false;
        uv_read_stop((uv_stream_t*)&state->tcp);
        if (queue_disconnected_event) {
            Push_Disconnected_Event(state);
        }
        if (!uv_is_closing((uv_handle_t*)&state->tcp)) {
            uv_close((uv_handle_t*)&state->tcp, Router_Client_Close_Callback);
        }
    }

    void Update_Member_Cache(std::vector<s_router_member_info>* members, const s_router_member_event_payload* event) {
        if (members == nullptr || event == nullptr) {
            return;
        }

        members->erase(
            std::remove_if(members->begin(), members->end(), [&](const s_router_member_info& member) {
                return member.network_id == event->member.network_id;
            }),
            members->end());

        if (event->event == ROUTER_MEMBER_JOINED) {
            members->push_back(event->member);
        }
    }

    void Push_Protocol_Error(s_router_client_state* state, const char* message) {
        state->last_error = UV_EPROTO;
        s_router_client_event event;
        event.kind = ROUTER_CLIENT_EVENT_PROTOCOL_ERROR;
        event.error.code = ROUTER_ERROR_INVALID_MESSAGE;
        Router_Copy_String(event.error.message, sizeof(event.error.message), message != nullptr ? message : "invalid router payload");
        state->events.push_back(std::move(event));
    }

    void Router_Client_Handle_Message(s_router_client_state* state, s_router_message* message) {
        s_router_client_event event;
        event.message = std::move(*message);

        std::string error;
        switch (event.message.header.kind) {
            case ROUTER_MESSAGE_WELCOME: {
                s_router_welcome_payload welcome;
                if (!Decode_Welcome_Payload(&event.message.payload, &welcome, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->local_network_id = welcome.network_id;
                Router_Copy_String(state->router_id, sizeof(state->router_id), welcome.router_id);
                event.kind = ROUTER_CLIENT_EVENT_WELCOME;
                break;
            }
            case ROUTER_MESSAGE_SESSION_LIST:
                if (!Decode_Session_List_Payload(&event.message.payload, &event.sessions, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->sessions = event.sessions;
                event.kind = ROUTER_CLIENT_EVENT_SESSION_LIST;
                break;
            case ROUTER_MESSAGE_SESSION_REGISTERED:
                if (!Decode_Session_Info_Payload(&event.message.payload, &event.session, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->session_id = event.session.session_id;
                event.kind = ROUTER_CLIENT_EVENT_SESSION_REGISTERED;
                break;
            case ROUTER_MESSAGE_JOINED_SESSION:
                if (!Decode_Session_Info_Payload(&event.message.payload, &event.session, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->session_id = event.session.session_id;
                event.kind = ROUTER_CLIENT_EVENT_JOINED_SESSION;
                break;
            case ROUTER_MESSAGE_LEFT_SESSION: {
                char reason[ROUTER_MEMBER_REMOVAL_REASON_SIZE] = {};
                if (!Decode_Text_Payload(
                        &event.message.payload, sizeof(reason), reason, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->session_id = 0;
                state->members.clear();
                event.reason = reason;
                event.kind = ROUTER_CLIENT_EVENT_LEFT_SESSION;
                break;
            }
            case ROUTER_MESSAGE_MEMBER_LIST:
                if (!Decode_Member_List_Payload(&event.message.payload, &event.members, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                state->members = event.members;
                event.kind = ROUTER_CLIENT_EVENT_MEMBER_LIST;
                break;
            case ROUTER_MESSAGE_MEMBER_JOINED:
                if (!Decode_Member_Event_Payload(&event.message.payload, &event.member_event, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                Update_Member_Cache(&state->members, &event.member_event);
                event.kind = ROUTER_CLIENT_EVENT_MEMBER_JOINED;
                break;
            case ROUTER_MESSAGE_MEMBER_LEFT:
                if (!Decode_Member_Event_Payload(&event.message.payload, &event.member_event, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                Update_Member_Cache(&state->members, &event.member_event);
                event.kind = ROUTER_CLIENT_EVENT_MEMBER_LEFT;
                break;
            case ROUTER_MESSAGE_SESSION_CLOSED:
                state->session_id = 0;
                state->members.clear();
                event.kind = ROUTER_CLIENT_EVENT_SESSION_CLOSED;
                break;
            case ROUTER_MESSAGE_GAME_DATA:
                event.kind = ROUTER_CLIENT_EVENT_GAME_DATA;
                break;
            case ROUTER_MESSAGE_OK:
                event.kind = ROUTER_CLIENT_EVENT_OK;
                break;
            case ROUTER_MESSAGE_ERROR:
                if (!Decode_Error_Payload(&event.message.payload, &event.error, &error)) {
                    Push_Protocol_Error(state, error.c_str());
                    return;
                }
                event.kind = ROUTER_CLIENT_EVENT_ERROR;
                break;
            default:
                event.kind = ROUTER_CLIENT_EVENT_MESSAGE;
                break;
        }

        state->events.push_back(std::move(event));
    }

    bool Router_Client_Wait_For_Connect(s_router_client_state* state) {
        const auto timeout = std::chrono::milliseconds(state->config.connect_timeout_ms);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!state->connect_done && std::chrono::steady_clock::now() < deadline) {
            uv_run(&state->loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (!state->connect_done) {
            state->last_error = UV_ETIMEDOUT;
            return false;
        }
        return state->connect_status == 0;
    }

    void Router_Client_Alloc_Callback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer) {
        (void)handle;
        const size_t allocation_size = suggested_size == 0 ? 4096 : suggested_size;
        buffer->base = (char*)std::malloc(allocation_size);
        buffer->len = (unsigned int)allocation_size;
    }

    void Router_Client_Connect_Callback(uv_connect_t* request, int status) {
        s_router_client_state* state = (s_router_client_state*)request->data;
        if (state == nullptr) {
            return;
        }
        state->connect_done = true;
        state->connect_status = status;
        if (status != 0) {
            state->last_error = status;
        }
    }

    void Router_Client_Read_Callback(uv_stream_t* stream, ssize_t bytes_read, const uv_buf_t* buffer) {
        s_router_client_state* state = (s_router_client_state*)stream->data;
        if (state == nullptr) {
            if (buffer != nullptr) {
                std::free(buffer->base);
            }
            return;
        }

        if (bytes_read > 0) {
            state->wire_bytes_in += (uint64_t)bytes_read;
            const uint8_t* bytes = (const uint8_t*)buffer->base;
            state->read_buffer.Write_Bytes(bytes, (uint32_t)bytes_read);

            for (;;) {
                s_router_message message;
                std::string error_message;
                const e_frame_read_status status = Try_Read_Frame(&state->read_buffer, state->config.max_frame_size, &state->frame_codec, &message, &error_message);
                if (status == FRAME_READ_NEED_MORE_DATA) {
                    break;
                }
                if (status == FRAME_READ_PROTOCOL_ERROR) {
                    Push_Protocol_Error(state, error_message.c_str());
                    Router_Client_Begin_Close(state, true);
                    break;
                }
                state->frames_in++;
                Router_Client_Handle_Message(state, &message);
            }
        } else if (bytes_read < 0) {
            state->last_error = bytes_read == UV_EOF ? 0 : (int)bytes_read;
            Router_Client_Begin_Close(state, true);
        }

        if (buffer != nullptr) {
            std::free(buffer->base);
        }
    }

    void Router_Client_Write_Callback(uv_write_t* request, int status) {
        s_router_client_write_request* write_request = (s_router_client_write_request*)request->data;
        if (write_request != nullptr && write_request->client != nullptr && write_request->byte_count <= write_request->client->pending_write_bytes) {
            write_request->client->pending_write_bytes -= write_request->byte_count;
        }
        if (write_request != nullptr && write_request->client != nullptr) {
            if (status == 0) {
                write_request->client->wire_bytes_out += write_request->byte_count;
                write_request->client->frames_out++;
            } else {
                write_request->client->failed_write_bytes += write_request->byte_count;
                write_request->client->failed_write_frames++;
                write_request->client->last_error = status;
                Router_Client_Begin_Close(write_request->client, true);
            }
        }
        delete write_request;
    }

    void Router_Client_Close_Callback(uv_handle_t* handle) {
        s_router_client_state* state = (s_router_client_state*)handle->data;
        if (state == nullptr) {
            return;
        }
        state->tcp_closed = true;
        state->closing = false;
        state->connected = false;
    }

    s_router_client::s_router_client()
        : state(std::make_unique<s_router_client_state>()) {
    }

    s_router_client::~s_router_client() {
        Router_Client_Disconnect(this);
    }

    void Router_Client_Init(s_router_client* client, const s_router_client_config* config) {
        if (client == nullptr) {
            return;
        }
        if (client->state == nullptr) {
            client->state = std::make_unique<s_router_client_state>();
        }
        if (client->state->loop_initialized) {
            Router_Client_Disconnect(client);
        }

        client->state->config = config != nullptr ? *config : s_router_client_config{};
        client->state->local_network_id = 0;
        client->state->session_id = 0;
        client->state->pending_write_bytes = 0;
        client->state->wire_bytes_in = 0;
        client->state->wire_bytes_out = 0;
        client->state->frames_in = 0;
        client->state->frames_out = 0;
        client->state->failed_write_bytes = 0;
        client->state->failed_write_frames = 0;
        client->state->last_error = 0;
        client->state->connect_status = 0;
        client->state->connect_done = false;
        client->state->disconnected_event_sent = false;
        client->state->read_buffer.Clear();
        client->state->frame_codec.Reset();
        client->state->events.clear();
        client->state->sessions.clear();
        client->state->members.clear();
        client->state->router_id[0] = '\0';
    }

    bool Router_Client_Connect(s_router_client* client) {
        if (client == nullptr || client->state == nullptr || client->state->loop_initialized) {
            return false;
        }

        s_router_client_state* state = client->state.get();
        state->last_error = 0;
        state->connect_done = false;
        state->connect_status = 0;
        state->connected = false;
        state->closing = false;
        state->tcp_closed = true;
        state->disconnected_event_sent = false;

        int result = uv_loop_init(&state->loop);
        if (result != 0) {
            state->last_error = result;
            return false;
        }
        state->loop_initialized = true;

        result = uv_tcp_init(&state->loop, &state->tcp);
        if (result != 0) {
            state->last_error = result;
            Router_Client_Disconnect(client);
            return false;
        }
        state->tcp.data = state;
        state->tcp_initialized = true;
        state->tcp_closed = false;

        sockaddr_in address{};
        result = uv_ip4_addr(state->config.host, state->config.tcp_port, &address);
        if (result != 0) {
            state->last_error = result;
            Router_Client_Disconnect(client);
            return false;
        }

        state->connect_request.data = state;
        result = uv_tcp_connect(&state->connect_request, &state->tcp, (const sockaddr*)&address, Router_Client_Connect_Callback);
        if (result != 0) {
            state->last_error = result;
            Router_Client_Disconnect(client);
            return false;
        }

        if (!Router_Client_Wait_For_Connect(state)) {
            Router_Client_Disconnect(client);
            return false;
        }

        uv_tcp_nodelay(&state->tcp, 1);
        result = uv_read_start((uv_stream_t*)&state->tcp, Router_Client_Alloc_Callback, Router_Client_Read_Callback);
        if (result != 0) {
            state->last_error = result;
            Router_Client_Disconnect(client);
            return false;
        }

        state->connected = true;
        return true;
    }

    void Router_Client_Disconnect(s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return;
        }

        s_router_client_state* state = client->state.get();
        if (!state->loop_initialized) {
            state->connected = false;
            state->tcp_initialized = false;
            state->tcp_closed = true;
            return;
        }

        Router_Client_Begin_Close(state, false);

        while (uv_loop_close(&state->loop) == UV_EBUSY) {
            uv_run(&state->loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        state->loop_initialized = false;
        state->tcp_initialized = false;
        state->tcp_closed = true;
        state->closing = false;
        state->connected = false;
        state->pending_write_bytes = 0;
        state->local_network_id = 0;
        state->session_id = 0;
        state->router_id[0] = '\0';
        state->sessions.clear();
        state->members.clear();
        state->wire_bytes_in = 0;
        state->wire_bytes_out = 0;
        state->frames_in = 0;
        state->frames_out = 0;
        state->failed_write_bytes = 0;
        state->failed_write_frames = 0;
        state->frame_codec.Reset();
    }

    int Router_Client_Pump(s_router_client* client) {
        if (client == nullptr || client->state == nullptr || !client->state->loop_initialized) {
            return 0;
        }
        uv_run(&client->state->loop, UV_RUN_NOWAIT);
        return client->state->last_error;
    }

    bool Router_Client_Is_Connected(const s_router_client* client) {
        return client != nullptr && client->state != nullptr && client->state->connected;
    }

    int Router_Client_Last_Error(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return 0;
        }
        return client->state->last_error;
    }

    const char* Router_Client_Last_Error_Message(const s_router_client* client) {
        const int error = Router_Client_Last_Error(client);
        return error == 0 ? "no error" : uv_strerror(error);
    }

    uint32_t Router_Client_Local_Network_Id(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return 0;
        }
        return client->state->local_network_id;
    }

    uint32_t Router_Client_Session_Id(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return 0;
        }
        return client->state->session_id;
    }

    const char* Router_Client_Router_Id(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return "";
        }
        return client->state->router_id;
    }

    const std::vector<s_router_session_info>& Router_Client_Sessions(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return Empty_Sessions();
        }
        return client->state->sessions;
    }

    const std::vector<s_router_member_info>& Router_Client_Members(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return Empty_Members();
        }
        return client->state->members;
    }

    s_router_client_stats Router_Client_Stats(const s_router_client* client) {
        s_router_client_stats stats;
        if (client == nullptr || client->state == nullptr) {
            return stats;
        }
        stats.wire_bytes_in = client->state->wire_bytes_in;
        stats.wire_bytes_out = client->state->wire_bytes_out;
        stats.frames_in = client->state->frames_in;
        stats.frames_out = client->state->frames_out;
        stats.failed_write_bytes = client->state->failed_write_bytes;
        stats.failed_write_frames = client->state->failed_write_frames;
        stats.pending_write_bytes = client->state->pending_write_bytes;
        stats.compression = client->state->frame_codec.Stats();
        return stats;
    }

    size_t Router_Client_Event_Count(const s_router_client* client) {
        if (client == nullptr || client->state == nullptr) {
            return 0;
        }
        return client->state->events.size();
    }

    bool Router_Client_Pop_Event(s_router_client* client, s_router_client_event* event) {
        if (client == nullptr || client->state == nullptr || event == nullptr || client->state->events.empty()) {
            return false;
        }
        *event = std::move(client->state->events.front());
        client->state->events.pop_front();
        return true;
    }

    bool Router_Client_Send_Message(s_router_client* client, const s_router_message* message) {
        if (client == nullptr || client->state == nullptr || message == nullptr || !client->state->connected || client->state->closing) {
            return false;
        }

        s_router_client_state* state = client->state.get();
        const uint32_t max_encoded_size = Encoded_Frame_Max_Size(message);
        if (max_encoded_size > state->config.max_pending_write_bytes ||
            state->pending_write_bytes > state->config.max_pending_write_bytes - max_encoded_size) {
            return false;
        }

        s_router_client_write_request* write_request = new s_router_client_write_request{};
        Encode_Frame(message, &state->frame_codec, &write_request->data);
        write_request->byte_count = write_request->data.Size();

        state->pending_write_bytes += write_request->byte_count;
        write_request->buffer = uv_buf_init((char*)write_request->data.Data(), write_request->data.Size());
        write_request->request.data = write_request;
        write_request->client = state;

        const int result = uv_write(&write_request->request, (uv_stream_t*)&state->tcp, &write_request->buffer, 1, Router_Client_Write_Callback);
        if (result != 0) {
            state->pending_write_bytes -= write_request->byte_count;
            state->last_error = result;
            delete write_request;
            Router_Client_Begin_Close(state, true);
            return false;
        }
        return true;
    }

    bool Router_Client_Send_Hello(s_router_client* client, const char* client_name, uint32_t protocol_version, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_HELLO;
        message.header.request_id = request_id;
        Encode_Hello_Payload(client_name, protocol_version, &message.payload);
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Request_Session_List(s_router_client* client, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_LIST_SESSIONS;
        message.header.request_id = request_id;
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Register_Session(s_router_client* client, const s_router_session_registration_payload* registration, uint32_t request_id) {
        if (registration == nullptr) {
            return false;
        }
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_REGISTER_SESSION;
        message.header.request_id = request_id;
        Encode_Session_Registration_Payload(registration, &message.payload);
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Unregister_Session(s_router_client* client, uint32_t session_id, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_UNREGISTER_SESSION;
        message.header.session_id = session_id;
        message.header.request_id = request_id;
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Set_Session_Open(s_router_client* client, uint32_t session_id, bool open, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_SET_SESSION_OPEN;
        message.header.session_id = session_id;
        message.header.flags = open ? ROUTER_FLAG_SESSION_OPEN : ROUTER_FLAG_SESSION_CLOSED;
        message.header.request_id = request_id;
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Join_Session(s_router_client* client, uint32_t session_id, const char* build_string, uint32_t request_id) {
        s_router_join_session_payload join;
        Router_Copy_String(join.build_string, sizeof(join.build_string), build_string);

        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_JOIN_SESSION;
        message.header.session_id = session_id;
        message.header.request_id = request_id;
        Encode_Join_Session_Payload(&join, &message.payload);
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Leave_Session(s_router_client* client, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_LEAVE_SESSION;
        message.header.request_id = request_id;
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Remove_Member(s_router_client* client,
        uint32_t target_network_id,
        const char* reason,
        uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_REMOVE_MEMBER;
        message.header.target_network_id = target_network_id;
        message.header.request_id = request_id;
        Encode_Text_Payload(reason, &message.payload);
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Send_To(s_router_client* client, uint32_t target_network_id, const uint8_t* payload, uint32_t payload_size, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_SEND_TO;
        message.header.target_network_id = target_network_id;
        message.header.request_id = request_id;
        message.payload.Write_Bytes(payload, payload_size);
        return Router_Client_Send_Message(client, &message);
    }

    bool Router_Client_Send_Broadcast(s_router_client* client, const uint8_t* payload, uint32_t payload_size, uint32_t flags, uint32_t request_id) {
        s_router_message message;
        message.header.kind = ROUTER_MESSAGE_SEND_BROADCAST;
        message.header.flags = flags;
        message.header.request_id = request_id;
        message.payload.Write_Bytes(payload, payload_size);
        return Router_Client_Send_Message(client, &message);
    }

} // namespace network::router