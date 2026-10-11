#include "doctest/doctest.h"

#include "network/router_protocol.h"
#include "network/router_service.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <uv.h>

using namespace network::router;

namespace {

struct Service_Thread_Guard_ {
    s_router_service* service = nullptr;
    std::thread* service_thread = nullptr;

    ~Service_Thread_Guard_() {
        if (service != nullptr && service_thread != nullptr && service_thread->joinable()) {
            Router_Service_Request_Stop(service);
            service_thread->join();
        }
    }
};

struct Test_Log_ {
    std::atomic<int> backpressure_drops{0};
    std::atomic<int> broadcast_data_sizes{0};
    std::atomic<int> compression_stats{0};
    std::atomic<int> direct_data_sizes{0};
    std::atomic<int> protocol_errors{0};
};

void Test_Log_Callback_(const char* message, void* user_data) {
    Test_Log_* log = (Test_Log_*)user_data;
    if (log == nullptr || message == nullptr) {
        return;
    }
    if (std::strstr(message, "dropping outbound router message") != nullptr) {
        log->backpressure_drops.fetch_add(1);
    }
    if (std::strstr(message, "game data broadcast") != nullptr &&
        std::strstr(message, "bytes=") != nullptr &&
        std::strstr(message, "compressed_bytes=") != nullptr &&
        std::strstr(message, "compression_saved=") != nullptr) {
        log->broadcast_data_sizes.fetch_add(1);
    }
    if (std::strstr(message, "game data direct") != nullptr &&
        std::strstr(message, "bytes=") != nullptr &&
        std::strstr(message, "compressed_bytes=") != nullptr &&
        std::strstr(message, "compression_saved=") != nullptr) {
        log->direct_data_sizes.fetch_add(1);
    }
    if (std::strstr(message, "traffic peer=") != nullptr && std::strstr(message, "compressed_payload_in=") != nullptr) {
        log->compression_stats.fetch_add(1);
    }
    if (std::strstr(message, "protocol") != nullptr || std::strstr(message, "frame") != nullptr) {
        log->protocol_errors.fetch_add(1);
    }
}

s_router_message Make_Hello_(const char* name, uint32_t request_id) {
    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_HELLO;
    message.header.request_id = request_id;
    Encode_Hello_Payload(name, ROUTER_VERSION, &message.payload);
    return message;
}

s_router_message Make_Register_(uint32_t request_id) {
    s_router_session_registration_payload registration;
    Router_Copy_String(registration.session_name, sizeof(registration.session_name), "TCP Game");
    Router_Copy_String(registration.host_name, sizeof(registration.host_name), "Host");
    Router_Copy_String(registration.build_string, sizeof(registration.build_string), "test-build");
    registration.max_players = 4;

    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_REGISTER_SESSION;
    message.header.request_id = request_id;
    Encode_Session_Registration_Payload(&registration, &message.payload);
    return message;
}

s_router_message Make_Join_(uint32_t session_id, uint32_t request_id) {
    s_router_join_session_payload join;
    Router_Copy_String(join.build_string, sizeof(join.build_string), "test-build");

    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_JOIN_SESSION;
    message.header.session_id = session_id;
    message.header.request_id = request_id;
    Encode_Join_Session_Payload(&join, &message.payload);
    return message;
}

bool Stream_Equals_(const Bit_Stream& stream, const uint8_t* data, uint32_t size) {
    return stream.Size() == size && (size == 0 || std::memcmp(stream.Data(), data, size) == 0);
}

struct Uv_Test_Client_ {
    uv_loop_t loop{};
    uv_tcp_t tcp{};
    Bit_Stream read_buffer;
    s_router_frame_codec frame_codec;
    std::vector<s_router_message> received_messages;
    bool loop_initialized = false;
    bool tcp_initialized = false;
    bool connect_done = false;
    bool connected = false;
    bool write_done = false;
    bool closed = false;
    bool remote_closed = false;
    int error = 0;

    bool Connect(uint16_t port) {
        int result = uv_loop_init(&loop);
        if (result != 0) {
            error = result;
            return false;
        }
        loop_initialized = true;

        result = uv_tcp_init(&loop, &tcp);
        if (result != 0) {
            error = result;
            return false;
        }
        tcp.data = this;
        tcp_initialized = true;

        sockaddr_in address{};
        result = uv_ip4_addr("127.0.0.1", port, &address);
        if (result != 0) {
            error = result;
            return false;
        }

        uv_connect_t connect_request{};
        connect_request.data = this;
        result = uv_tcp_connect(&connect_request, &tcp, (const sockaddr*)&address, Connect_Callback_);
        if (result != 0) {
            error = result;
            return false;
        }

        if (!Pump_Until_([&]() { return connect_done; })) {
            return false;
        }
        if (!connected) {
            return false;
        }

        result = uv_read_start((uv_stream_t*)&tcp, Alloc_Callback_, Read_Callback_);
        if (result != 0) {
            error = result;
            return false;
        }
        return true;
    }

    bool Send(const s_router_message& message) {
        Bit_Stream frame;
        Encode_Frame(&message, &frame_codec, &frame);
        return Send_Raw(frame.Data(), frame.Size());
    }

    bool Send_Raw(const uint8_t* data, uint32_t size) {
        if (data == nullptr || size == 0) {
            return false;
        }

        uv_write_t write_request{};
        write_request.data = this;
        uv_buf_t buffer = uv_buf_init((char*)data, size);
        write_done = false;
        const int result = uv_write(&write_request, (uv_stream_t*)&tcp, &buffer, 1, Write_Callback_);
        if (result != 0) {
            error = result;
            return false;
        }

        return Pump_Until_([&]() { return write_done; });
    }

    bool Send_Raw_Allow_Close(const uint8_t* data, uint32_t size) {
        const int saved_error = error;
        error = 0;
        const bool sent = Send_Raw(data, size);
        if (!sent && error != 0) {
            return false;
        }
        error = saved_error;
        return sent;
    }

    bool Wait_For_Messages(size_t message_count) {
        return Pump_Until_([&]() { return received_messages.size() >= message_count; });
    }

    bool Wait_For_Messages_Allow_Close(size_t message_count) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (received_messages.size() < message_count && std::chrono::steady_clock::now() < deadline) {
            uv_run(&loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return received_messages.size() >= message_count;
    }

    bool Wait_For_Remote_Close() {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!remote_closed && error == 0 && std::chrono::steady_clock::now() < deadline) {
            uv_run(&loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return remote_closed && error == 0;
    }

    void Pump_For(std::chrono::milliseconds duration) {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (error == 0 && std::chrono::steady_clock::now() < deadline) {
            uv_run(&loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void Close() {
        if (tcp_initialized && !closed && !uv_is_closing((uv_handle_t*)&tcp)) {
            uv_close((uv_handle_t*)&tcp, Close_Callback_);
            Pump_Until_([&]() { return closed; });
        }
        if (loop_initialized) {
            while (uv_loop_close(&loop) == UV_EBUSY) {
                uv_run(&loop, UV_RUN_NOWAIT);
            }
            loop_initialized = false;
        }
    }

    ~Uv_Test_Client_() {
        Close();
    }

    template <typename Predicate>
    bool Pump_Until_(Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!predicate() && error == 0 && std::chrono::steady_clock::now() < deadline) {
            uv_run(&loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return predicate() && error == 0;
    }

    static void Alloc_Callback_(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer) {
        (void)handle;
        const size_t allocation_size = suggested_size == 0 ? 4096 : suggested_size;
        buffer->base = (char*)std::malloc(allocation_size);
        buffer->len = (unsigned int)allocation_size;
    }

    static void Connect_Callback_(uv_connect_t* request, int status) {
        Uv_Test_Client_* client = (Uv_Test_Client_*)request->data;
        client->connect_done = true;
        if (status == 0) {
            client->connected = true;
        } else {
            client->error = status;
        }
    }

    static void Write_Callback_(uv_write_t* request, int status) {
        Uv_Test_Client_* client = (Uv_Test_Client_*)request->data;
        client->write_done = true;
        if (status != 0) {
            client->error = status;
        }
    }

    static void Read_Callback_(uv_stream_t* stream, ssize_t bytes_read, const uv_buf_t* buffer) {
        Uv_Test_Client_* client = (Uv_Test_Client_*)stream->data;
        if (bytes_read > 0) {
            const uint8_t* bytes = (const uint8_t*)buffer->base;
            client->read_buffer.Write_Bytes(bytes, (uint32_t)bytes_read);
            for (;;) {
                s_router_message message;
                std::string error_message;
                const e_frame_read_status status = Try_Read_Frame(&client->read_buffer, ROUTER_DEFAULT_MAX_FRAME_SIZE, &client->frame_codec, &message, &error_message);
                if (status == FRAME_READ_NEED_MORE_DATA) {
                    break;
                }
                if (status == FRAME_READ_PROTOCOL_ERROR) {
                    client->error = UV_EPROTO;
                    break;
                }
                client->received_messages.push_back(std::move(message));
            }
        } else if (bytes_read < 0) {
            if (bytes_read == UV_EOF) {
                client->remote_closed = true;
            } else {
                client->error = (int)bytes_read;
            }
        }

        std::free(buffer->base);
    }

    static void Close_Callback_(uv_handle_t* handle) {
        Uv_Test_Client_* client = (Uv_Test_Client_*)handle->data;
        client->closed = true;
    }
};

struct Uv_Udp_Client_ {
    uv_loop_t loop{};
    uv_udp_t udp{};
    Bit_Stream reply;
    bool loop_initialized = false;
    bool udp_initialized = false;
    bool send_done = false;
    int error = 0;

    bool Init() {
        int result = uv_loop_init(&loop);
        if (result != 0) {
            error = result;
            return false;
        }
        loop_initialized = true;

        result = uv_udp_init(&loop, &udp);
        if (result != 0) {
            error = result;
            return false;
        }
        udp.data = this;
        udp_initialized = true;

        sockaddr_in address{};
        result = uv_ip4_addr("127.0.0.1", 0, &address);
        if (result != 0) {
            error = result;
            return false;
        }
        result = uv_udp_bind(&udp, (const sockaddr*)&address, 0);
        if (result != 0) {
            error = result;
            return false;
        }
        result = uv_udp_recv_start(&udp, Alloc_Callback_, Recv_Callback_);
        if (result != 0) {
            error = result;
            return false;
        }
        return true;
    }

    bool Send_Probe(uint16_t port) {
        Bit_Stream probe;
        Encode_Discovery_Probe(&probe);

        sockaddr_in target{};
        int result = uv_ip4_addr("127.0.0.1", port, &target);
        if (result != 0) {
            error = result;
            return false;
        }

        uv_udp_send_t send_request{};
        send_request.data = this;
        uv_buf_t buffer = uv_buf_init((char*)probe.Data(), probe.Size());
        result = uv_udp_send(&send_request, &udp, &buffer, 1, (const sockaddr*)&target, Send_Callback_);
        if (result != 0) {
            error = result;
            return false;
        }
        return Pump_Until_([&]() { return send_done; });
    }

    bool Wait_For_Reply() {
        return Pump_Until_([&]() { return reply.Size() != 0; });
    }

    void Close() {
        if (udp_initialized && !uv_is_closing((uv_handle_t*)&udp)) {
            uv_close((uv_handle_t*)&udp, nullptr);
        }
        if (loop_initialized) {
            while (uv_loop_close(&loop) == UV_EBUSY) {
                uv_run(&loop, UV_RUN_NOWAIT);
            }
            loop_initialized = false;
        }
    }

    ~Uv_Udp_Client_() {
        Close();
    }

    template <typename Predicate>
    bool Pump_Until_(Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!predicate() && error == 0 && std::chrono::steady_clock::now() < deadline) {
            uv_run(&loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return predicate() && error == 0;
    }

    static void Alloc_Callback_(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buffer) {
        (void)handle;
        const size_t allocation_size = suggested_size == 0 ? 4096 : suggested_size;
        buffer->base = (char*)std::malloc(allocation_size);
        buffer->len = (unsigned int)allocation_size;
    }

    static void Send_Callback_(uv_udp_send_t* request, int status) {
        Uv_Udp_Client_* client = (Uv_Udp_Client_*)request->data;
        client->send_done = true;
        if (status != 0) {
            client->error = status;
        }
    }

    static void Recv_Callback_(uv_udp_t* handle, ssize_t bytes_read, const uv_buf_t* buffer, const sockaddr* address, unsigned flags) {
        (void)address;
        (void)flags;
        Uv_Udp_Client_* client = (Uv_Udp_Client_*)handle->data;
        if (bytes_read > 0) {
            const uint8_t* bytes = (const uint8_t*)buffer->base;
            client->reply.Clear();
            client->reply.Write_Bytes(bytes, (uint32_t)bytes_read);
        } else if (bytes_read < 0 && bytes_read != UV_EOF) {
            client->error = (int)bytes_read;
        }
        std::free(buffer->base);
    }
};

bool Start_Test_Service_(s_router_service* service, std::thread* service_thread, int* run_result) {
    *run_result = -1;
    *service_thread = std::thread([service, run_result]() {
        *run_result = Router_Service_Run(service);
    });

    for (int i = 0; i < 100 && !Router_Service_Is_Running(service); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (int i = 0; i < 100 && Router_Service_Bound_Tcp_Port(service) == 0 && Router_Service_Is_Running(service); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (Router_Service_Config(service) != nullptr && Router_Service_Config(service)->enable_discovery) {
        for (int i = 0; i < 100 && Router_Service_Bound_Discovery_Port(service) == 0 && Router_Service_Is_Running(service); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return Router_Service_Is_Running(service) && Router_Service_Bound_Tcp_Port(service) != 0;
}

}

DOCTEST_TEST_SUITE("router service") {

DOCTEST_TEST_CASE("uv service starts on an ephemeral TCP port and stops cleanly") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;
    Test_Log_ log;
    config.log_callback = Test_Log_Callback_;
    config.log_user_data = &log;
    Router_Copy_String(config.router_id, sizeof(config.router_id), "test-router");

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;

    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
    DOCTEST_CHECK_FALSE(Router_Service_Is_Running(&service));
}

DOCTEST_TEST_CASE("discovery reply advertises router identity and bound TCP port") {
    s_router_config config;
    Router_Copy_String(config.router_id, sizeof(config.router_id), "router-a");
    Router_Copy_String(config.router_name, sizeof(config.router_name), "LAN Router");

    Bit_Stream payload;
    Build_Discovery_Reply(&config, 49000, &payload);

    s_router_discovery_reply reply;
    std::string error;
    DOCTEST_REQUIRE(Decode_Discovery_Reply(&payload, &reply, &error));
    DOCTEST_CHECK(reply.tcp_port == 49000);
    DOCTEST_CHECK(reply.protocol_version == ROUTER_PROTOCOL_VERSION);
    DOCTEST_CHECK(std::strcmp(reply.router_id, "router-a") == 0);
    DOCTEST_CHECK(std::strcmp(reply.router_name, "LAN Router") == 0);
}

DOCTEST_TEST_CASE("uv TCP service performs hello, session registration, join, and data routing") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;
    config.verbose_logging = true;
    Test_Log_ log;
    config.log_callback = Test_Log_Callback_;
    config.log_user_data = &log;
    Router_Copy_String(config.router_id, sizeof(config.router_id), "tcp-router");

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));
    const uint16_t port = Router_Service_Bound_Tcp_Port(&service);

    Uv_Test_Client_ host;
    Uv_Test_Client_ guest;
    std::string error;

    DOCTEST_REQUIRE(host.Connect(port));
    DOCTEST_REQUIRE(host.Send(Make_Hello_("host", 1)));
    DOCTEST_REQUIRE(host.Wait_For_Messages(1));

    s_router_welcome_payload host_welcome;
    DOCTEST_REQUIRE(Decode_Welcome_Payload(&host.received_messages[0].payload, &host_welcome, &error));
    DOCTEST_CHECK(host_welcome.network_id == 1);

    DOCTEST_REQUIRE(host.Send(Make_Register_(2)));
    DOCTEST_REQUIRE(host.Wait_For_Messages(2));
    DOCTEST_CHECK(host.received_messages[1].header.kind == ROUTER_MESSAGE_SESSION_REGISTERED);
    DOCTEST_REQUIRE(host.Wait_For_Messages(3));
    DOCTEST_CHECK(host.received_messages[2].header.kind == ROUTER_MESSAGE_MEMBER_LIST);

    s_router_session_info registered_session;
    DOCTEST_REQUIRE(Decode_Session_Info_Payload(&host.received_messages[1].payload, &registered_session, &error));
    DOCTEST_CHECK(registered_session.session_id == 1);
    DOCTEST_CHECK(std::strcmp(registered_session.session_name, "TCP Game") == 0);

    DOCTEST_REQUIRE(guest.Connect(port));
    DOCTEST_REQUIRE(guest.Send(Make_Hello_("guest", 3)));
    DOCTEST_REQUIRE(guest.Wait_For_Messages(1));

    s_router_welcome_payload guest_welcome;
    DOCTEST_REQUIRE(Decode_Welcome_Payload(&guest.received_messages[0].payload, &guest_welcome, &error));
    DOCTEST_CHECK(guest_welcome.network_id == 2);

    s_router_message list;
    list.header.kind = ROUTER_MESSAGE_LIST_SESSIONS;
    list.header.request_id = 4;
    DOCTEST_REQUIRE(guest.Send(list));
    DOCTEST_REQUIRE(guest.Wait_For_Messages(2));

    std::vector<s_router_session_info> sessions;
    DOCTEST_REQUIRE(Decode_Session_List_Payload(&guest.received_messages[1].payload, &sessions, &error));
    DOCTEST_REQUIRE(sessions.size() == 1);
    DOCTEST_CHECK(std::strcmp(sessions[0].session_name, "TCP Game") == 0);

    DOCTEST_REQUIRE(guest.Send(Make_Join_(registered_session.session_id, 5)));
    DOCTEST_REQUIRE(guest.Wait_For_Messages(4));
    DOCTEST_CHECK(guest.received_messages[2].header.kind == ROUTER_MESSAGE_JOINED_SESSION);
    DOCTEST_CHECK(guest.received_messages[3].header.kind == ROUTER_MESSAGE_MEMBER_LIST);
    DOCTEST_REQUIRE(host.Wait_For_Messages(4));
    DOCTEST_CHECK(host.received_messages[3].header.kind == ROUTER_MESSAGE_MEMBER_JOINED);

    s_router_message direct;
    direct.header.kind = ROUTER_MESSAGE_SEND_TO;
    direct.header.target_network_id = host_welcome.network_id;
    direct.header.request_id = 6;
    const uint8_t direct_payload[] = {42, 43};
    direct.payload.Write_Bytes(direct_payload, sizeof(direct_payload));
    DOCTEST_REQUIRE(guest.Send(direct));
    DOCTEST_REQUIRE(host.Wait_For_Messages(5));
    DOCTEST_CHECK(host.received_messages[4].header.kind == ROUTER_MESSAGE_GAME_DATA);
    DOCTEST_CHECK(host.received_messages[4].header.sender_network_id == guest_welcome.network_id);
    DOCTEST_CHECK(Stream_Equals_(host.received_messages[4].payload, direct_payload, sizeof(direct_payload)));

    s_router_message broadcast;
    broadcast.header.kind = ROUTER_MESSAGE_SEND_BROADCAST;
    broadcast.header.flags = ROUTER_FLAG_BROADCAST_INCLUDE_SENDER;
    broadcast.header.request_id = 7;
    const uint8_t broadcast_payload[] = {7, 8, 9};
    broadcast.payload.Write_Bytes(broadcast_payload, sizeof(broadcast_payload));
    DOCTEST_REQUIRE(host.Send(broadcast));
    DOCTEST_REQUIRE(host.Wait_For_Messages(6));
    DOCTEST_REQUIRE(guest.Wait_For_Messages(5));
    DOCTEST_CHECK(Stream_Equals_(host.received_messages[5].payload, broadcast_payload, sizeof(broadcast_payload)));
    DOCTEST_CHECK(Stream_Equals_(guest.received_messages[4].payload, broadcast_payload, sizeof(broadcast_payload)));

    guest.Close();
    host.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
    DOCTEST_CHECK(log.broadcast_data_sizes.load() == 1);
    DOCTEST_CHECK(log.compression_stats.load() >= 2);
    DOCTEST_CHECK(log.direct_data_sizes.load() == 1);
}

DOCTEST_TEST_CASE("uv TCP service returns protocol error before closing malformed frames") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;
    Test_Log_ log;
    config.log_callback = Test_Log_Callback_;
    config.log_user_data = &log;

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    Uv_Test_Client_ client;
    DOCTEST_REQUIRE(client.Connect(Router_Service_Bound_Tcp_Port(&service)));
    const uint8_t bad_frame[] = {1, 0, 0, 0};
    DOCTEST_REQUIRE(client.Send_Raw_Allow_Close(bad_frame, sizeof(bad_frame)));
    DOCTEST_REQUIRE(client.Wait_For_Messages_Allow_Close(1));
    DOCTEST_CHECK(client.received_messages[0].header.kind == ROUTER_MESSAGE_ERROR);
    DOCTEST_CHECK(log.protocol_errors.load() == 1);

    client.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("uv TCP service handles multiple framed messages in one write") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    Uv_Test_Client_ client;
    DOCTEST_REQUIRE(client.Connect(Router_Service_Bound_Tcp_Port(&service)));

    Bit_Stream hello_frame;
    s_router_frame_codec frame_codec;
    s_router_message hello = Make_Hello_("host", 1);
    Encode_Frame(&hello, &frame_codec, &hello_frame);

    Bit_Stream list_frame;
    s_router_message list;
    list.header.kind = ROUTER_MESSAGE_LIST_SESSIONS;
    list.header.request_id = 2;
    Encode_Frame(&list, &frame_codec, &list_frame);

    Bit_Stream combined;
    combined.Write_Bytes(hello_frame.Data(), hello_frame.Size());
    combined.Write_Bytes(list_frame.Data(), list_frame.Size());
    DOCTEST_REQUIRE(client.Send_Raw(combined.Data(), combined.Size()));
    DOCTEST_REQUIRE(client.Wait_For_Messages(2));
    DOCTEST_CHECK(client.received_messages[0].header.kind == ROUTER_MESSAGE_WELCOME);
    DOCTEST_CHECK(client.received_messages[1].header.kind == ROUTER_MESSAGE_SESSION_LIST);

    client.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("uv TCP service drops outbound messages that exceed pending write limit") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;
    config.max_pending_write_bytes = 1;
    Test_Log_ log;
    config.log_callback = Test_Log_Callback_;
    config.log_user_data = &log;

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    Uv_Test_Client_ client;
    DOCTEST_REQUIRE(client.Connect(Router_Service_Bound_Tcp_Port(&service)));
    DOCTEST_REQUIRE(client.Send(Make_Hello_("host", 1)));
    client.Pump_For(std::chrono::milliseconds(50));
    DOCTEST_CHECK_FALSE(client.remote_closed);
    DOCTEST_CHECK(client.received_messages.empty());
    DOCTEST_CHECK(log.backpressure_drops.load() == 1);

    client.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("uv TCP service rejects oversized frames at service level") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.enable_discovery = false;
    config.enable_signal_handlers = false;
    config.max_frame_size = ROUTER_MESSAGE_HEADER_SIZE;
    Test_Log_ log;
    config.log_callback = Test_Log_Callback_;
    config.log_user_data = &log;

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    Uv_Test_Client_ client;
    DOCTEST_REQUIRE(client.Connect(Router_Service_Bound_Tcp_Port(&service)));
    DOCTEST_REQUIRE(client.Send_Raw_Allow_Close((const uint8_t*)"\xff\xff\x00\x00", 4));
    DOCTEST_REQUIRE(client.Wait_For_Messages_Allow_Close(1));
    DOCTEST_CHECK(client.received_messages[0].header.kind == ROUTER_MESSAGE_ERROR);
    DOCTEST_CHECK(log.protocol_errors.load() == 1);

    client.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("uv UDP discovery socket replies to binary probe") {
    s_router_config config;
    Router_Copy_String(config.listen_host, sizeof(config.listen_host), "127.0.0.1");
    config.tcp_port = 0;
    config.discovery_port = 0;
    config.enable_discovery = true;
    config.enable_signal_handlers = false;
    Router_Copy_String(config.router_id, sizeof(config.router_id), "udp-router");
    Router_Copy_String(config.router_name, sizeof(config.router_name), "UDP Router");

    s_router_service service;
    Router_Service_Init(&service, &config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));
    DOCTEST_REQUIRE(Router_Service_Bound_Discovery_Port(&service) != 0);

    Uv_Udp_Client_ client;
    DOCTEST_REQUIRE(client.Init());
    DOCTEST_REQUIRE(client.Send_Probe(Router_Service_Bound_Discovery_Port(&service)));
    DOCTEST_REQUIRE(client.Wait_For_Reply());

    s_router_discovery_reply reply;
    std::string error;
    DOCTEST_REQUIRE(Decode_Discovery_Reply(&client.reply, &reply, &error));
    DOCTEST_CHECK(reply.tcp_port == Router_Service_Bound_Tcp_Port(&service));
    DOCTEST_CHECK(std::strcmp(reply.router_id, "udp-router") == 0);
    DOCTEST_CHECK(std::strcmp(reply.router_name, "UDP Router") == 0);

    client.Close();
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

}