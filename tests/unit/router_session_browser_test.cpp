#include "doctest/doctest.h"

#include "network/router_client.h"
#include "network/router_service.h"
#include "network/router_session_browser.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

using namespace network::router;

namespace {

struct Browser_Log_ {
    std::atomic<int> accepted_connections{0};
    std::atomic<int> closed_connections{0};
};

void Browser_Log_Callback_(const char* message, void* user_data) {
    Browser_Log_* log = static_cast<Browser_Log_*>(user_data);
    if (log == nullptr || message == nullptr) {
        return;
    }
    if (std::strstr(message, "connection accepted peer=") != nullptr) {
        log->accepted_connections.fetch_add(1);
    }
    if (std::strstr(message, "connection closed peer=") != nullptr) {
        log->closed_connections.fetch_add(1);
    }
}

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

bool Wait_For_Client_Events_(s_router_client* client, size_t event_count) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (Router_Client_Event_Count(client) < event_count && std::chrono::steady_clock::now() < deadline) {
        Router_Client_Pump(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return Router_Client_Event_Count(client) >= event_count;
}

bool Wait_For_Browser_Revision_(s_router_session_browser* browser, uint32_t previous_revision) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (Router_Session_Browser_Revision(browser) == previous_revision && std::chrono::steady_clock::now() < deadline) {
        Router_Session_Browser_Pump(browser);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return Router_Session_Browser_Revision(browser) != previous_revision;
}

bool Wait_For_Log_Value_(const std::atomic<int>* value, int expected_minimum) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (value->load() < expected_minimum && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return value->load() >= expected_minimum;
}

s_router_client_config Make_Client_Config_(uint16_t port) {
    s_router_client_config config;
    Router_Copy_String(config.host, sizeof(config.host), "127.0.0.1");
    config.tcp_port = port;
    return config;
}

bool Create_Service_Session_(s_router_client* client, uint16_t port, const char* client_name, const char* session_name) {
    s_router_client_config client_config = Make_Client_Config_(port);
    Router_Client_Init(client, &client_config);
    if (!Router_Client_Connect(client) || !Router_Client_Send_Hello(client, client_name, ROUTER_VERSION, 1)) {
        return false;
    }
    if (!Wait_For_Client_Events_(client, 1)) {
        return false;
    }
    s_router_client_event event;
    Router_Client_Pop_Event(client, &event);
    if (event.kind != ROUTER_CLIENT_EVENT_WELCOME) {
        return false;
    }

    s_router_session_registration_payload registration;
    Router_Copy_String(registration.session_name, sizeof(registration.session_name), session_name);
    Router_Copy_String(registration.host_name, sizeof(registration.host_name), client_name);
    Router_Copy_String(registration.build_string, sizeof(registration.build_string), "test-build");
    registration.max_players = 4;
    if (!Router_Client_Register_Session(client, &registration, 2)) {
        return false;
    }
    if (!Wait_For_Client_Events_(client, 1)) {
        return false;
    }
    Router_Client_Pop_Event(client, &event);
    return event.kind == ROUTER_CLIENT_EVENT_SESSION_REGISTERED;
}

}

DOCTEST_TEST_SUITE("router session browser") {

DOCTEST_TEST_CASE("direct endpoint refresh returns a session snapshot") {
    s_router_config service_config;
    Router_Copy_String(service_config.listen_host, sizeof(service_config.listen_host), "127.0.0.1");
    service_config.tcp_port = 0;
    service_config.enable_discovery = false;
    service_config.enable_signal_handlers = false;

    s_router_service service;
    Router_Service_Init(&service, &service_config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));
    s_router_client host;
    DOCTEST_REQUIRE(Create_Service_Session_(&host, Router_Service_Bound_Tcp_Port(&service), "host", "Direct Game"));

    s_router_session_browser browser;
    s_router_session_browser_config browser_config;
    browser_config.mode = ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT;
    Router_Copy_String(browser_config.host, sizeof(browser_config.host), "127.0.0.1");
    browser_config.tcp_port = Router_Service_Bound_Tcp_Port(&service);
    Router_Copy_String(browser_config.client_name, sizeof(browser_config.client_name), "browser");
    Router_Session_Browser_Init(&browser, &browser_config);

    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, 0));
    const std::vector<s_router_session_record>& sessions = Router_Session_Browser_Sessions(&browser);
    DOCTEST_REQUIRE(sessions.size() == 1);
    DOCTEST_CHECK(std::strcmp(sessions[0].session.session_name, "Direct Game") == 0);
    DOCTEST_CHECK(std::strcmp(sessions[0].router.host, "127.0.0.1") == 0);
    DOCTEST_CHECK(sessions[0].router.tcp_port == Router_Service_Bound_Tcp_Port(&service));

    Router_Session_Browser_Shutdown(&browser);
    Router_Client_Disconnect(&host);
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("direct endpoint refresh reuses one connection and shutdown closes it") {
    Browser_Log_ browser_log;

    s_router_config service_config;
    Router_Copy_String(service_config.listen_host, sizeof(service_config.listen_host), "127.0.0.1");
    service_config.tcp_port = 0;
    service_config.enable_discovery = false;
    service_config.enable_signal_handlers = false;
    service_config.verbose_logging = true;
    service_config.log_callback = Browser_Log_Callback_;
    service_config.log_user_data = &browser_log;

    s_router_service service;
    Router_Service_Init(&service, &service_config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));
    s_router_client host;
    DOCTEST_REQUIRE(Create_Service_Session_(&host, Router_Service_Bound_Tcp_Port(&service), "host", "Direct Game"));

    s_router_session_browser browser;
    s_router_session_browser_config browser_config;
    browser_config.mode = ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT;
    Router_Copy_String(browser_config.host, sizeof(browser_config.host), "127.0.0.1");
    browser_config.tcp_port = Router_Service_Bound_Tcp_Port(&service);
    Router_Copy_String(browser_config.client_name, sizeof(browser_config.client_name), "browser");
    Router_Session_Browser_Init(&browser, &browser_config);

    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, 0));
    DOCTEST_CHECK(Router_Session_Browser_Sessions(&browser).size() == 1);
    DOCTEST_REQUIRE(Wait_For_Log_Value_(&browser_log.accepted_connections, 2));

    const uint32_t revision = Router_Session_Browser_Revision(&browser);
    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, revision));
    DOCTEST_CHECK(Router_Session_Browser_Sessions(&browser).size() == 1);
    DOCTEST_CHECK(browser_log.accepted_connections.load() == 2);
    DOCTEST_CHECK(browser_log.closed_connections.load() == 0);

    Router_Session_Browser_Shutdown(&browser);
    DOCTEST_REQUIRE(Wait_For_Log_Value_(&browser_log.closed_connections, 1));

    Router_Client_Disconnect(&host);
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("LAN discovery refresh combines sessions from responding routers") {
    s_router_config service_config_a;
    Router_Copy_String(service_config_a.listen_host, sizeof(service_config_a.listen_host), "127.0.0.1");
    service_config_a.tcp_port = 0;
    service_config_a.enable_discovery = false;
    service_config_a.enable_signal_handlers = false;
    Router_Copy_String(service_config_a.router_id, sizeof(service_config_a.router_id), "router-a");

    s_router_config service_config_b = service_config_a;
    Router_Copy_String(service_config_b.router_id, sizeof(service_config_b.router_id), "router-b");

    s_router_service service_a;
    s_router_service service_b;
    Router_Service_Init(&service_a, &service_config_a);
    Router_Service_Init(&service_b, &service_config_b);
    int run_result_a = -1;
    int run_result_b = -1;
    std::thread service_thread_a;
    std::thread service_thread_b;
    Service_Thread_Guard_ service_guard_a{&service_a, &service_thread_a};
    Service_Thread_Guard_ service_guard_b{&service_b, &service_thread_b};
    DOCTEST_REQUIRE(Start_Test_Service_(&service_a, &service_thread_a, &run_result_a));
    DOCTEST_REQUIRE(Start_Test_Service_(&service_b, &service_thread_b, &run_result_b));
    s_router_client host_a;
    s_router_client host_b;
    DOCTEST_REQUIRE(Create_Service_Session_(&host_a, Router_Service_Bound_Tcp_Port(&service_a), "host-a", "LAN A"));
    DOCTEST_REQUIRE(Create_Service_Session_(&host_b, Router_Service_Bound_Tcp_Port(&service_b), "host-b", "LAN B"));

    s_router_session_browser browser;
    s_router_session_browser_config browser_config;
    browser_config.mode = ROUTER_SESSION_BROWSER_LAN_DISCOVERY;
    Router_Copy_String(browser_config.initial_endpoints[0].host, sizeof(browser_config.initial_endpoints[0].host), "127.0.0.1");
    browser_config.initial_endpoints[0].tcp_port = Router_Service_Bound_Tcp_Port(&service_a);
    Router_Copy_String(browser_config.initial_endpoints[1].host, sizeof(browser_config.initial_endpoints[1].host), "127.0.0.1");
    browser_config.initial_endpoints[1].tcp_port = Router_Service_Bound_Tcp_Port(&service_b);
    browser_config.initial_endpoint_count = 2;
    Router_Copy_String(browser_config.client_name, sizeof(browser_config.client_name), "browser");
    Router_Session_Browser_Init(&browser, &browser_config);

    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, 0));
    const std::vector<s_router_session_record>& sessions = Router_Session_Browser_Sessions(&browser);
    DOCTEST_REQUIRE(sessions.size() == 2);
    const bool found_a = std::any_of(sessions.begin(), sessions.end(), [](const s_router_session_record& record) {
        return std::strcmp(record.session.session_name, "LAN A") == 0;
    });
    const bool found_b = std::any_of(sessions.begin(), sessions.end(), [](const s_router_session_record& record) {
        return std::strcmp(record.session.session_name, "LAN B") == 0;
    });
    DOCTEST_CHECK(found_a);
    DOCTEST_CHECK(found_b);

    Router_Session_Browser_Shutdown(&browser);
    Router_Client_Disconnect(&host_a);
    Router_Client_Disconnect(&host_b);
    Router_Service_Request_Stop(&service_a);
    Router_Service_Request_Stop(&service_b);
    service_thread_a.join();
    service_thread_b.join();
    service_guard_a.service = nullptr;
    service_guard_a.service_thread = nullptr;
    service_guard_b.service = nullptr;
    service_guard_b.service_thread = nullptr;
    DOCTEST_CHECK(run_result_a == 0);
    DOCTEST_CHECK(run_result_b == 0);
}

DOCTEST_TEST_CASE("LAN discovery deduplicates exact duplicate endpoints") {
    s_router_config service_config;
    Router_Copy_String(service_config.listen_host, sizeof(service_config.listen_host), "0.0.0.0");
    service_config.tcp_port = 0;
    service_config.enable_discovery = false;
    service_config.enable_signal_handlers = false;
    Router_Copy_String(service_config.router_id, sizeof(service_config.router_id), "alias-router");

    s_router_service service;
    Router_Service_Init(&service, &service_config);
    int run_result = -1;
    std::thread service_thread;
    Service_Thread_Guard_ service_guard{&service, &service_thread};
    DOCTEST_REQUIRE(Start_Test_Service_(&service, &service_thread, &run_result));

    s_router_client host;
    DOCTEST_REQUIRE(Create_Service_Session_(&host, Router_Service_Bound_Tcp_Port(&service), "host", "Aliased LAN"));

    s_router_session_browser browser;
    s_router_session_browser_config browser_config;
    browser_config.mode = ROUTER_SESSION_BROWSER_LAN_DISCOVERY;
    Router_Copy_String(browser_config.initial_endpoints[0].host, sizeof(browser_config.initial_endpoints[0].host), "127.0.0.1");
    browser_config.initial_endpoints[0].tcp_port = Router_Service_Bound_Tcp_Port(&service);
    Router_Copy_String(browser_config.initial_endpoints[0].router_id, sizeof(browser_config.initial_endpoints[0].router_id), "alias-router");
    Router_Copy_String(browser_config.initial_endpoints[1].host, sizeof(browser_config.initial_endpoints[1].host), "127.0.0.1");
    browser_config.initial_endpoints[1].tcp_port = Router_Service_Bound_Tcp_Port(&service);
    Router_Copy_String(browser_config.initial_endpoints[1].router_id, sizeof(browser_config.initial_endpoints[1].router_id), "alias-router");
    browser_config.initial_endpoint_count = 2;
    Router_Copy_String(browser_config.client_name, sizeof(browser_config.client_name), "browser");
    Router_Session_Browser_Init(&browser, &browser_config);

    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, 0));
    const std::vector<s_router_session_record>& sessions = Router_Session_Browser_Sessions(&browser);
    DOCTEST_REQUIRE(sessions.size() == 1);
    DOCTEST_CHECK(std::strcmp(sessions[0].session.session_name, "Aliased LAN") == 0);
    DOCTEST_CHECK(std::strcmp(sessions[0].router.host, "127.0.0.1") == 0);

    Router_Session_Browser_Shutdown(&browser);
    Router_Client_Disconnect(&host);
    Router_Service_Request_Stop(&service);
    service_thread.join();
    service_guard.service = nullptr;
    service_guard.service_thread = nullptr;
    DOCTEST_CHECK(run_result == 0);
}

DOCTEST_TEST_CASE("LAN discovery refresh without routers completes with empty results") {
    s_router_session_browser browser;
    s_router_session_browser_config browser_config;
    browser_config.mode = ROUTER_SESSION_BROWSER_LAN_DISCOVERY;
    browser_config.discovery_port = 9;
    browser_config.discovery_timeout_ms = 50;
    Router_Session_Browser_Init(&browser, &browser_config);

    DOCTEST_REQUIRE(Router_Session_Browser_Refresh(&browser));
    DOCTEST_REQUIRE(Wait_For_Browser_Revision_(&browser, 0));
    DOCTEST_CHECK(Router_Session_Browser_Sessions(&browser).empty());

    Router_Session_Browser_Shutdown(&browser);
}

}