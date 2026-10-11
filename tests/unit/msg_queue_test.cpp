#include "pch.h"

#include "doctest/doctest.h"

DOCTEST_TEST_SUITE("message queue") {

DOCTEST_TEST_CASE("empty message span has no minimum") {
    DOCTEST_CHECK(MSG_QUEUE::Lowest_Priority_Index(nullptr, 0) == -1);

    s_MSG message = {};
    DOCTEST_CHECK(MSG_QUEUE::Lowest_Priority_Index(&message, 0) == -1);
}

DOCTEST_TEST_CASE("minimum search stays within the final full message partition") {
    s_MSG messages[MAX_MESSAGES + 1] = {};
    for (int16_t idx = 0; idx < MAX_MESSAGES; ++idx) {
        messages[idx].priority = 5;
    }
    messages[MAX_MESSAGES - 1].priority = 1;
    messages[MAX_MESSAGES].priority = 0;

    DOCTEST_CHECK(MSG_QUEUE::Lowest_Priority_Index(messages, MAX_MESSAGES) == MAX_MESSAGES - 1);
}

DOCTEST_TEST_CASE("full final partition replaces its minimum without changing the canary") {
    s_MSG messages[MAX_MESSAGES + 1] = {};
    for (int16_t idx = 0; idx < MAX_MESSAGES; ++idx) {
        messages[idx].priority = 5;
    }
    messages[MAX_MESSAGES - 1].priority = 1;
    messages[MAX_MESSAGES].priority = 0xEE;

    uint8_t count = MAX_MESSAGES;
    s_MSG replacement = {};
    replacement.priority = 6;
    replacement.msg_type = MSG_TYPE_DIPLOMACY;
    MSG_QUEUE::Add(messages, MAX_MESSAGES, &count, replacement);

    DOCTEST_CHECK(count == MAX_MESSAGES);
    DOCTEST_CHECK(messages[MAX_MESSAGES - 1].priority == 6);
    DOCTEST_CHECK(messages[MAX_MESSAGES - 1].msg_type == MSG_TYPE_DIPLOMACY);
    DOCTEST_CHECK(messages[MAX_MESSAGES].priority == 0xEE);
}

DOCTEST_TEST_CASE("minimum search endpoint does not move when a lower priority is found") {
    s_MSG messages[6] = {};
    messages[0].priority = 9;
    messages[1].priority = 8;
    messages[2].priority = 7;
    messages[3].priority = 6;
    messages[4].priority = 0;
    messages[5].priority = 0;

    DOCTEST_CHECK(MSG_QUEUE::Lowest_Priority_Index(messages, 4) == 3);
}

DOCTEST_TEST_CASE("equal minimum priorities retain the first message") {
    s_MSG messages[4] = {};
    messages[0].priority = 3;
    messages[1].priority = 5;
    messages[2].priority = 3;
    messages[3].priority = 4;

    DOCTEST_CHECK(MSG_QUEUE::Lowest_Priority_Index(messages, 4) == 0);
}

DOCTEST_TEST_CASE("full partition rejects a message that does not exceed its minimum") {
    s_MSG messages[3] = {};
    messages[0].priority = 3;
    messages[1].priority = 5;
    messages[2].priority = 4;
    uint8_t count = 3;

    s_MSG rejected = {};
    rejected.priority = 3;
    rejected.msg_type = MSG_TYPE_DIPLOMACY;
    MSG_QUEUE::Add(messages, 3, &count, rejected);

    DOCTEST_CHECK(count == 3);
    DOCTEST_CHECK(messages[0].msg_type != MSG_TYPE_DIPLOMACY);
}

}