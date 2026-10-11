#include "doctest/doctest.h"

#include "network/router_protocol.h"

#include <cstring>
#include <string>
#include <vector>

using namespace network::router;

namespace {

bool Stream_Equals_(const Bit_Stream& stream, const uint8_t* data, uint32_t size) {
    return stream.Size() == size && (size == 0 || std::memcmp(stream.Data(), data, size) == 0);
}

uint32_t Read_Uint32_(const uint8_t* data) {
    return (uint32_t)data[0] |
        ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[3] << 24);
}

uint16_t Read_Uint16_(const uint8_t* data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

}

DOCTEST_TEST_SUITE("router protocol") {

DOCTEST_TEST_CASE("bit stream writes length-prefixed strings and reads into caller buffer") {
    Bit_Stream stream;
    stream.Write_String("foobar");
    DOCTEST_REQUIRE(stream.Size() == 10);

    const uint8_t* data = stream.Data();
    DOCTEST_CHECK(data[0] == 6);
    DOCTEST_CHECK(data[1] == 0);
    DOCTEST_CHECK(data[2] == 0);
    DOCTEST_CHECK(data[3] == 0);
    DOCTEST_CHECK(std::memcmp(data + 4, "foobar", 6) == 0);

    uint32_t required_size = 0;
    DOCTEST_CHECK_FALSE(stream.Read_String(&required_size, 0, nullptr));
    DOCTEST_CHECK(required_size == 6);

    char text[7]{};
    DOCTEST_REQUIRE(stream.Read_String(&required_size, sizeof(text), text));
    DOCTEST_CHECK(std::strcmp(text, "foobar") == 0);
    DOCTEST_CHECK(stream.At_End());
}

DOCTEST_TEST_CASE("bit stream reads ASCIIZ strings into caller buffer") {
    const uint8_t bytes[] = {'h', 'i', 0, 'x'};
    Bit_Stream stream(bytes, sizeof(bytes));

    uint32_t length = 0;
    char text[3]{};
    DOCTEST_REQUIRE(stream.Read_ASCIIZ_String(&length, sizeof(text), text));
    DOCTEST_CHECK(length == 2);
    DOCTEST_CHECK(std::strcmp(text, "hi") == 0);
    DOCTEST_CHECK(stream.Tell() == 3);
}

DOCTEST_TEST_CASE("framed messages round trip with fixed routing header and payload") {
    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_SEND_TO;
    message.header.flags = 7;
    message.header.session_id = 11;
    message.header.sender_network_id = 22;
    message.header.target_network_id = 33;
    message.header.request_id = 44;
    const uint8_t payload[] = {1, 2, 3, 4};
    message.payload.Write_Bytes(payload, sizeof(payload));

    Bit_Stream frame;
    Encode_Frame(&message, &frame);
    DOCTEST_REQUIRE(frame.Size() > 4 + ROUTER_PACKET_HEADER_SIZE + ROUTER_MESSAGE_HEADER_SIZE);
    DOCTEST_CHECK(Read_Uint32_(frame.Data()) == frame.Size() - 4);
    DOCTEST_CHECK((Read_Uint16_(frame.Data() + 4) & ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED) != 0);
    DOCTEST_CHECK(Read_Uint32_(frame.Data() + 4 + ROUTER_PACKET_HEADER_SIZE) == ROUTER_PROTOCOL_MAGIC);

    Bit_Stream frame_buffer(frame.Data(), frame.Size());
    s_router_message decoded;
    std::string error;
    DOCTEST_CHECK(Try_Read_Frame(&frame_buffer, ROUTER_DEFAULT_MAX_FRAME_SIZE, &decoded, &error) == FRAME_READ_MESSAGE_READY);
    DOCTEST_CHECK(frame_buffer.Size() == 0);
    DOCTEST_CHECK(decoded.header.kind == ROUTER_MESSAGE_SEND_TO);
    DOCTEST_CHECK(decoded.header.flags == 7);
    DOCTEST_CHECK(decoded.header.session_id == 11);
    DOCTEST_CHECK(decoded.header.sender_network_id == 22);
    DOCTEST_CHECK(decoded.header.target_network_id == 33);
    DOCTEST_CHECK(decoded.header.request_id == 44);
    DOCTEST_CHECK(Stream_Equals_(decoded.payload, payload, sizeof(payload)));
}

DOCTEST_TEST_CASE("framed messages are compressed on the wire") {
    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_SEND_BROADCAST;
    Bit_Stream payload;
    payload.Resize(4096);
    std::memset(payload.Data(), 0x41, payload.Size());
    message.payload.Write_Bytes(payload.Data(), payload.Size());

    Bit_Stream body;
    Encode_Message_Body(&message, &body);

    Bit_Stream frame;
    Encode_Frame(&message, &frame);
    DOCTEST_REQUIRE(frame.Size() > 4 + ROUTER_PACKET_HEADER_SIZE + ROUTER_MESSAGE_HEADER_SIZE);
    DOCTEST_CHECK((Read_Uint16_(frame.Data() + 4) & ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED) != 0);
    DOCTEST_CHECK(frame.Size() < 4 + ROUTER_PACKET_HEADER_SIZE + body.Size());

    Bit_Stream frame_buffer(frame.Data(), frame.Size());
    s_router_message decoded;
    std::string error;
    DOCTEST_REQUIRE(Try_Read_Frame(&frame_buffer, ROUTER_DEFAULT_MAX_FRAME_SIZE, &decoded, &error) == FRAME_READ_MESSAGE_READY);
    DOCTEST_CHECK(frame_buffer.Size() == 0);
    DOCTEST_CHECK(decoded.header.kind == ROUTER_MESSAGE_SEND_BROADCAST);
    DOCTEST_REQUIRE(decoded.payload.Size() == payload.Size());
    DOCTEST_CHECK(std::memcmp(decoded.payload.Data(), payload.Data(), payload.Size()) == 0);
}

DOCTEST_TEST_CASE("frame reader waits for complete message before consuming bytes") {
    s_router_message message;
    message.header.kind = ROUTER_MESSAGE_HELLO;
    Encode_Hello_Payload("alice", ROUTER_VERSION, &message.payload);

    Bit_Stream encoded_frame;
    Encode_Frame(&message, &encoded_frame);
    Bit_Stream partial(encoded_frame.Data(), encoded_frame.Size() - 1);

    s_router_message decoded;
    std::string error;
    DOCTEST_CHECK(Try_Read_Frame(&partial, ROUTER_DEFAULT_MAX_FRAME_SIZE, &decoded, &error) == FRAME_READ_NEED_MORE_DATA);
    DOCTEST_CHECK(partial.Size() == encoded_frame.Size() - 1);

    partial.Write_Bytes(encoded_frame.Data() + encoded_frame.Size() - 1, 1);
    DOCTEST_CHECK(Try_Read_Frame(&partial, ROUTER_DEFAULT_MAX_FRAME_SIZE, &decoded, &error) == FRAME_READ_MESSAGE_READY);
    DOCTEST_CHECK(partial.Size() == 0);

    s_router_hello_payload hello;
    DOCTEST_REQUIRE(Decode_Hello_Payload(&decoded.payload, &hello, &error));
    DOCTEST_CHECK(std::strcmp(hello.client_name, "alice") == 0);
    DOCTEST_CHECK(hello.protocol_version == ROUTER_VERSION);
}

DOCTEST_TEST_CASE("join session payload preserves client build string") {
    s_router_join_session_payload join;
    Router_Copy_String(join.build_string, sizeof(join.build_string), "test-build");

    Bit_Stream payload;
    Encode_Join_Session_Payload(&join, &payload);

    s_router_join_session_payload decoded;
    std::string error;
    DOCTEST_REQUIRE(Decode_Join_Session_Payload(&payload, &decoded, &error));
    DOCTEST_CHECK(std::strcmp(decoded.build_string, "test-build") == 0);
}

DOCTEST_TEST_CASE("router copy string truncates and null terminates destination") {
    char text[5]{};

    Router_Copy_String(text, sizeof(text), "toolong");

    DOCTEST_CHECK(std::strcmp(text, "tool") == 0);
    DOCTEST_CHECK(text[sizeof(text) - 1] == '\0');
}

DOCTEST_TEST_CASE("session list payload preserves browser-visible metadata") {
    s_router_session_info session;
    session.session_id = 9;
    Router_Copy_String(session.router_id, sizeof(session.router_id), "router-a");
    Router_Copy_String(session.session_name, sizeof(session.session_name), "new game");
    Router_Copy_String(session.host_name, sizeof(session.host_name), "host");
    session.current_players = 2;
    session.max_players = 6;
    Router_Copy_String(session.build_string, sizeof(session.build_string), "1.0");
    session.open = true;

    std::vector<s_router_session_info> source{session};
    Bit_Stream payload;
    Encode_Session_List_Payload(&source, &payload);
    std::vector<s_router_session_info> decoded;
    std::string error;
    DOCTEST_REQUIRE(Decode_Session_List_Payload(&payload, &decoded, &error));
    DOCTEST_REQUIRE(decoded.size() == 1);
    DOCTEST_CHECK(decoded[0].session_id == 9);
    DOCTEST_CHECK(std::strcmp(decoded[0].router_id, "router-a") == 0);
    DOCTEST_CHECK(std::strcmp(decoded[0].session_name, "new game") == 0);
    DOCTEST_CHECK(std::strcmp(decoded[0].host_name, "host") == 0);
    DOCTEST_CHECK(decoded[0].current_players == 2);
    DOCTEST_CHECK(decoded[0].max_players == 6);
    DOCTEST_CHECK(std::strcmp(decoded[0].build_string, "1.0") == 0);
    DOCTEST_CHECK(decoded[0].open);
}

DOCTEST_TEST_CASE("binary discovery reply preserves router metadata") {
    Bit_Stream probe;
    Encode_Discovery_Probe(&probe);
    DOCTEST_CHECK(Is_Discovery_Probe(probe.Data(), probe.Size()));

    Bit_Stream reply_payload;
    Encode_Discovery_Reply(49000, "router-a", "LAN Router", &reply_payload);
    DOCTEST_CHECK(Read_Uint32_(reply_payload.Data()) == ROUTER_DISCOVERY_MAGIC);

    s_router_discovery_reply reply;
    std::string error;
    DOCTEST_REQUIRE(Decode_Discovery_Reply(&reply_payload, &reply, &error));
    DOCTEST_CHECK(reply.tcp_port == 49000);
    DOCTEST_CHECK(reply.protocol_version == ROUTER_PROTOCOL_VERSION);
    DOCTEST_CHECK(std::strcmp(reply.router_id, "router-a") == 0);
    DOCTEST_CHECK(std::strcmp(reply.router_name, "LAN Router") == 0);
}

DOCTEST_TEST_CASE("member list and member event payloads preserve identity metadata") {
    s_router_member_info member;
    member.network_id = 7;
    Router_Copy_String(member.player_name, sizeof(member.player_name), "guest");
    Router_Copy_String(member.build_string, sizeof(member.build_string), "test-build");
    member.owner = true;

    std::vector<s_router_member_info> source{member};
    Bit_Stream list_payload;
    Encode_Member_List_Payload(&source, &list_payload);

    std::vector<s_router_member_info> decoded;
    std::string error;
    DOCTEST_REQUIRE(Decode_Member_List_Payload(&list_payload, &decoded, &error));
    DOCTEST_REQUIRE(decoded.size() == 1);
    DOCTEST_CHECK(decoded[0].network_id == 7);
    DOCTEST_CHECK(std::strcmp(decoded[0].player_name, "guest") == 0);
    DOCTEST_CHECK(std::strcmp(decoded[0].build_string, "test-build") == 0);
    DOCTEST_CHECK(decoded[0].owner);

    Bit_Stream event_payload;
    Encode_Member_Event_Payload(ROUTER_MEMBER_JOINED, &member, &event_payload);
    s_router_member_event_payload event;
    DOCTEST_REQUIRE(Decode_Member_Event_Payload(&event_payload, &event, &error));
    DOCTEST_CHECK(event.event == ROUTER_MEMBER_JOINED);
    DOCTEST_CHECK(event.member.network_id == 7);
    DOCTEST_CHECK(event.member.owner);
}

DOCTEST_TEST_CASE("oversized frames are rejected without consuming buffered bytes") {
    const uint8_t frame_bytes[] = {0xff, 0xff, 0xff, 0x7f};
    Bit_Stream frame(frame_bytes, sizeof(frame_bytes));
    s_router_message decoded;
    std::string error;
    DOCTEST_CHECK(Try_Read_Frame(&frame, 1024, &decoded, &error) == FRAME_READ_PROTOCOL_ERROR);
    DOCTEST_CHECK(frame.Size() == 4);
}

}