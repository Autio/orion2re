#include "pch.h"

#include <cstdint>
#include <cstring>

#include "doctest/doctest.h"
#include "game/arena.h"

DOCTEST_TEST_CASE("arena grows in fixed chunks and leaves old tail space unused") {
    arena::s_arena* target = arena::Create(32);
    void* first = arena::Allocate_Aligned(target, 24, 8);
    void* second = arena::Allocate_Aligned(target, 16, 8);

    DOCTEST_CHECK(first != nullptr);
    DOCTEST_CHECK(second != nullptr);
    const arena::s_stats stats = arena::Get_Stats(target);
    DOCTEST_CHECK(stats.chunk_count == 2);
    DOCTEST_CHECK(stats.used_bytes == 40);
    DOCTEST_CHECK(stats.reserved_bytes == 64);
    DOCTEST_CHECK(arena::Release(target) == 0);
}

DOCTEST_TEST_CASE("arena supports aligned zeroed blocks and oversized chunks") {
    arena::s_arena* target = arena::Create(32);
    auto* bytes = static_cast<uint8_t*>(
        arena::Allocate_Zeroed_Aligned(target, 80, alignof(uint64_t)));

    DOCTEST_CHECK(reinterpret_cast<uintptr_t>(bytes) % alignof(uint64_t) == 0);
    for (size_t index = 0; index < 80; ++index) {
        DOCTEST_CHECK(bytes[index] == 0);
    }
    const arena::s_stats stats = arena::Get_Stats(target);
    DOCTEST_CHECK(stats.chunk_count == 1);
    DOCTEST_CHECK(stats.reserved_bytes >= 80);
    arena::Release(target);
}

DOCTEST_TEST_CASE("arena containment includes used allocation space only") {
    arena::s_arena* target = arena::Create(32);
    char* text = arena::Copy_String(target, "owned");
    const char unrelated[] = "unrelated";

    DOCTEST_CHECK(arena::Contains(target, text));
    DOCTEST_CHECK(arena::Contains(target, text + 5));
    DOCTEST_CHECK_FALSE(arena::Contains(target, unrelated));
    DOCTEST_CHECK_FALSE(arena::Contains(target, nullptr));
    DOCTEST_CHECK_FALSE(arena::Contains(nullptr, text));
    arena::Release(target);
}

DOCTEST_TEST_CASE("arena string helpers terminate bounded and empty copies") {
    arena::s_arena* target = arena::Create();
    char* bounded = arena::Copy_String_N(target, "abcdef", 3);
    char* empty = arena::Copy_String(target, "");

    DOCTEST_CHECK(std::strcmp(bounded, "abc") == 0);
    DOCTEST_CHECK(empty[0] == '\0');
    DOCTEST_CHECK(bounded != empty);
    arena::Release(target);
}

DOCTEST_TEST_CASE("arena remains alive until its final reference is released") {
    arena::s_arena* target = arena::Create();
    char* text = arena::Copy_String(target, "retained");
    arena::Add_Ref(target);

    DOCTEST_CHECK(arena::Get_Stats(target).reference_count == 2);
    DOCTEST_CHECK(arena::Release(target) == 1);
    DOCTEST_CHECK(std::strcmp(text, "retained") == 0);
    DOCTEST_CHECK(arena::Release(target) == 0);
}