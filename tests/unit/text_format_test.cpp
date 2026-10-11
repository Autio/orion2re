#include "pch.h"

#include <cstring>

#include "doctest/doctest.h"
#include "game/text_format.h"

namespace test_text_format_runtime {
    void Reset_();
    int Print_Kind_();
    int16_t Print_X_();
    int16_t Print_Y_();
    const char* Print_Text_();
    int16_t Font_Style_();
}

DOCTEST_TEST_CASE("named formatter replaces supplied values and preserves unknown placeholders") {
    char output[128];
    const STRBUILD::s_named_argument arguments[] = {
        STRBUILD::Named_Argument("FOO", 42),
        STRBUILD::Named_Argument("BAR", "value"),
    };

    STRBUILD::Named_Snprintf(
        output,
        sizeof(output),
        "$FOO$ $BAR$ $FOO$ $MISSING$ $$ $OPEN",
        arguments,
        sizeof(arguments) / sizeof(arguments[0])
    );

    DOCTEST_CHECK(std::strcmp(
        output, "42 value 42 $MISSING$ $ $OPEN") == 0);
}

DOCTEST_TEST_CASE("named formatter uses exact case-sensitive names") {
    char output[64];
    const STRBUILD::s_named_argument argument =
        STRBUILD::Named_Argument("FOO", 7u);

    STRBUILD::Named_Snprintf(
        output, sizeof(output), "$FOO$ $foo$", &argument, 1);

    DOCTEST_CHECK(std::strcmp(output, "7 $foo$") == 0);
}

DOCTEST_TEST_CASE("named formatter truncates with a terminator") {
    char output[8];
    const STRBUILD::s_named_argument argument =
        STRBUILD::Named_Argument("VALUE", "abcdefghijk");

    STRBUILD::Named_Snprintf(
        output, sizeof(output), "$VALUE$", &argument, 1);

    DOCTEST_CHECK(std::strcmp(output, "abcdefg") == 0);
    DOCTEST_CHECK(output[sizeof(output) - 1] == '\0');
}

DOCTEST_TEST_CASE("text format expands before drawing with alignment and offset") {
    test_text_format_runtime::Reset_();
    TEXT_FORMAT::s_format row = {
        "$COUNT$ / $UNKNOWN$", 2, TEXT_FORMAT::ALIGN_RIGHT, 100, 20
    };
    const STRBUILD::s_named_argument argument =
        STRBUILD::Named_Argument("COUNT", 9);

    TEXT_FORMAT::Print(row, &argument, 1, 4, 6);

    DOCTEST_CHECK(test_text_format_runtime::Print_Kind_() == 2);
    DOCTEST_CHECK(test_text_format_runtime::Print_X_() == 106);
    DOCTEST_CHECK(test_text_format_runtime::Print_Y_() == 24);
    DOCTEST_CHECK(std::strcmp(
        test_text_format_runtime::Print_Text_(), "9 / $UNKNOWN$") == 0);
    DOCTEST_CHECK(test_text_format_runtime::Font_Style_() == 2);
}