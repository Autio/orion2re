#include "doctest/doctest.h"

#include "network/router_client.h"
#include "network/router_service.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

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

std::vector<uint8_t> Copy_Stream_(const Bit_Stream& stream) {
    return std::vector<uint8_t>(stream.Data(), stream.Data() + stream.Size());
}

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

    return Router_Service_Is_Running(service) && Router_Service_Bound_Tcp_Port(service) != 0;
}

bool Wait_For_Client_Events_(s_router_client* client, size_t event_count) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (Router_Client_Event_Count(client) < event_count && std::chrono::steady_clock::now() < deadline) {
        Router_Client_Pump(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return Router_Client_Event_Count(client) >= event_count;
}

s_router_client_config Make_Client_Config_(uint16_t port) {
    s_router_client_config config;
    Router_Copy_String(config.host, sizeof(config.host), "127.0.0.1");
    config.tcp_port = port;
    return config;
}

s_router_session_registration_payload Make_Registration_() {
    s_router_session_registration_payload registration;
    Router_Copy_String(registration.session_name, sizeof(registration.session_name), "Client Game");
    Router_Copy_String(registration.host_name, sizeof(registration.host_name), "Host");
    Router_Copy_String(registration.build_string, sizeof(registration.build_string), "test-build");
    registration.max_players = 4;
    return registration;
}

}

DOCTEST_TEST_SUITE("router client") {

DOCTEST_TEST_CASE("client connects, joins sessions, routes game data, and receives member-left events") {
    s_router_config service_config;
    Router_Copy_String(service_config.listen_host, sizeof(service_config.listen_host), "127.0.0.1");
    service_config.tcp_port = 0;
    service_config.enable_discovery = false;
    service_config.enable_signal_handlers = false;
    Router_Copy_String(service_config.router_id, sizeof(service_config.router_id), "client-test-router");

    s_router_service service;
    Router_Service_Init(&service, &service_config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    const uint16_t port = Router_Service_Bound_Tcp_Port(&service);
    s_router_client_config host_config = Make_Client_Config_(port);
    s_router_client_config guest_config = Make_Client_Config_(port);

    s_router_client host;
    s_router_client guest;
    Router_Client_Init(&host, &host_config);
    Router_Client_Init(&guest, &guest_config);

    DOCTEST_REQUIRE(Router_Client_Connect(&host));
    DOCTEST_REQUIRE(Router_Client_Send_Hello(&host, "host", ROUTER_VERSION, 1));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 1));

    s_router_client_event event;
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_WELCOME);
    DOCTEST_CHECK(Router_Client_Local_Network_Id(&host) == 1);
    DOCTEST_CHECK(std::strcmp(Router_Client_Router_Id(&host), "client-test-router") == 0);

    s_router_session_registration_payload registration = Make_Registration_();
    DOCTEST_REQUIRE(Router_Client_Register_Session(&host, &registration, 2));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 2));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_SESSION_REGISTERED);
    DOCTEST_CHECK(event.session.session_id == 1);
    DOCTEST_CHECK(Router_Client_Session_Id(&host) == 1);
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_MEMBER_LIST);
    DOCTEST_REQUIRE(Router_Client_Members(&host).size() == 1);
    DOCTEST_CHECK(Router_Client_Members(&host)[0].network_id == 1);

    DOCTEST_REQUIRE(Router_Client_Connect(&guest));
    DOCTEST_REQUIRE(Router_Client_Send_Hello(&guest, "guest", ROUTER_VERSION, 3));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&guest, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&guest, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_WELCOME);
    DOCTEST_CHECK(Router_Client_Local_Network_Id(&guest) == 2);

    DOCTEST_REQUIRE(Router_Client_Request_Session_List(&guest, 4));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&guest, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&guest, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_SESSION_LIST);
    DOCTEST_REQUIRE(event.sessions.size() == 1);
    DOCTEST_CHECK(std::strcmp(event.sessions[0].session_name, "Client Game") == 0);
    DOCTEST_REQUIRE(Router_Client_Sessions(&guest).size() == 1);

    DOCTEST_REQUIRE(Router_Client_Join_Session(&guest, 1, "test-build", 5));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&guest, 2));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&guest, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_JOINED_SESSION);
    DOCTEST_CHECK(Router_Client_Session_Id(&guest) == 1);
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&guest, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_MEMBER_LIST);
    DOCTEST_REQUIRE(Router_Client_Members(&guest).size() == 2);

    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_MEMBER_JOINED);
    DOCTEST_CHECK(event.member_event.member.network_id == 2);
    DOCTEST_REQUIRE(Router_Client_Members(&host).size() == 2);

    const uint8_t direct_payload[] = {42, 43};
    DOCTEST_REQUIRE(Router_Client_Send_To(&guest, Router_Client_Local_Network_Id(&host), direct_payload, sizeof(direct_payload), 6));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_GAME_DATA);
    DOCTEST_CHECK(event.message.header.sender_network_id == Router_Client_Local_Network_Id(&guest));
    DOCTEST_CHECK(event.message.header.target_network_id == Router_Client_Local_Network_Id(&host));
    DOCTEST_CHECK(Copy_Stream_(event.message.payload) == std::vector<uint8_t>{42, 43});

    const uint8_t broadcast_payload[] = {7, 8, 9};
    DOCTEST_REQUIRE(Router_Client_Send_Broadcast(&host, broadcast_payload, sizeof(broadcast_payload), ROUTER_FLAG_BROADCAST_INCLUDE_SENDER, 7));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 1));
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&guest, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_GAME_DATA);
    DOCTEST_CHECK(Copy_Stream_(event.message.payload) == std::vector<uint8_t>{7, 8, 9});
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&guest, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_GAME_DATA);
    DOCTEST_CHECK(Copy_Stream_(event.message.payload) == std::vector<uint8_t>{7, 8, 9});

    Router_Client_Disconnect(&guest);
    DOCTEST_REQUIRE(Wait_For_Client_Events_(&host, 1));
    DOCTEST_REQUIRE(Router_Client_Pop_Event(&host, &event));
    DOCTEST_CHECK(event.kind == ROUTER_CLIENT_EVENT_MEMBER_LEFT);
    DOCTEST_CHECK(event.member_event.member.network_id == 2);
    DOCTEST_REQUIRE(Router_Client_Members(&host).size() == 1);

    Router_Client_Disconnect(&host);
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

}