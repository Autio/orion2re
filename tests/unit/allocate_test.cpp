#include "pch.h"

#include <string>

#include "doctest/doctest.h"
#include "game/allocate.h"

namespace {
    struct fatal_exit {};
    std::string fatal_message;
}

namespace exitm {
    [[noreturn]] void __cdecl Exit_With_Message(const char* message) {
        fatal_message = message;
        throw fatal_exit{};
    }
}

DOCTEST_TEST_CASE("global screen space covers expanded non-AI screen structures") {
    DOCTEST_CHECK(allocate::Global_Screen_Space() == 0x123D668u);
}

DOCTEST_TEST_CASE("mark stack accepts forty pushes and rejects push forty-one") {
    allocate::s_alloc_header header = {};
    void* block = &header + 1;
    allocate::mark_stack_pos[0] = 0;
    fatal_message.clear();

    for (int32_t i = 0; i < MARK_STACK_COUNT; ++i) {
        header.current = static_cast<uint32_t>(i);
        allocate::Push_Block(block);

        DOCTEST_CHECK(allocate::mark_stack_pos[0] == i + 1);
        DOCTEST_CHECK(allocate::mark_stack[i].block == block);
        DOCTEST_CHECK(
            allocate::mark_stack[i].current == static_cast<uint32_t>(i));
    }

    DOCTEST_CHECK_THROWS_AS(allocate::Push_Block(block), fatal_exit);
    DOCTEST_CHECK(fatal_message == "Mark Block Stack Size Exceeded");
    DOCTEST_CHECK(allocate::mark_stack_pos[0] == MARK_STACK_COUNT);

    for (int32_t i = MARK_STACK_COUNT - 1; i >= 0; --i) {
        header.current = UINT32_MAX;
        allocate::Pop_Block(nullptr);

        DOCTEST_CHECK(header.current == static_cast<uint32_t>(i));
    }
}