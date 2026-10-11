#include "pch.h"

#include <cstring>

#include "config/cfg_parser.h"
#include "doctest/doctest.h"

namespace {
    bool Parse_(const char* source, config::s_cfg_document* document,
                config::s_cfg_diagnostic* diagnostic) {
        return config::Parse_Cfg("fixture.cfg", source, std::strlen(source), document, diagnostic);
    }
}

DOCTEST_TEST_SUITE("CFG lexical parser") {

DOCTEST_TEST_CASE("assignments preserve raw paths and classify value syntax") {
    const char* source =
        "# Complete and partial assignments\n"
        "guardian weapon 1 = plasma_cannon all heavy,eccm 0x12 0b101 -5 1/3 -;\n"
        "guardian weapon 1 arc = all continuous;\n";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 2);

    const config::s_cfg_statement& complete = document.statements[0];
    DOCTEST_CHECK(complete.kind == config::CFG_STATEMENT_ASSIGNMENT);
    DOCTEST_REQUIRE(complete.path.size() == 3);
    DOCTEST_CHECK(complete.path[0].raw == "guardian");
    DOCTEST_CHECK(complete.path[1].raw == "weapon");
    DOCTEST_CHECK(complete.path[2].raw == "1");
    DOCTEST_CHECK(complete.path[2].kind == config::CFG_TOKEN_INTEGER);
    DOCTEST_REQUIRE(complete.values.size() == 8);
    DOCTEST_CHECK(complete.values[0].kind == config::CFG_TOKEN_IDENTIFIER);
    DOCTEST_CHECK(complete.values[1].kind == config::CFG_TOKEN_IDENTIFIER);
    DOCTEST_CHECK(complete.values[2].kind == config::CFG_TOKEN_MASK);
    DOCTEST_CHECK(complete.values[3].kind == config::CFG_TOKEN_INTEGER);
    DOCTEST_CHECK(complete.values[4].kind == config::CFG_TOKEN_INTEGER);
    DOCTEST_CHECK(complete.values[5].kind == config::CFG_TOKEN_INTEGER);
    DOCTEST_CHECK(complete.values[6].kind == config::CFG_TOKEN_RATIO);
    DOCTEST_CHECK(complete.values[7].kind == config::CFG_TOKEN_SKIP);

    const config::s_cfg_statement& partial = document.statements[1];
    DOCTEST_REQUIRE(partial.path.size() == 4);
    DOCTEST_CHECK(partial.path[3].raw == "arc");
    DOCTEST_REQUIRE(partial.values.size() == 2);
    DOCTEST_CHECK(partial.values[0].raw == "all");
    DOCTEST_CHECK(partial.values[1].raw == "continuous");
}

DOCTEST_TEST_CASE("strings use JSON escapes and preserve source spelling") {
    const char* source = "captain name = \"Gr\\u00fc\\u00dfe\\nAce \\\"One\\\"\";";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 1);
    DOCTEST_REQUIRE(document.statements[0].values.size() == 1);
    const config::s_cfg_token& value = document.statements[0].values[0];
    DOCTEST_CHECK(value.kind == config::CFG_TOKEN_STRING);
    DOCTEST_CHECK(value.raw == "\"Gr\\u00fc\\u00dfe\\nAce \\\"One\\\"\"");
    DOCTEST_CHECK(value.value == "Grüße\nAce \"One\"");
}

DOCTEST_TEST_CASE("labeled assignment groups preserve selectors and values") {
    const char* source =
        "guardian weapon 1 =\n"
        "  mods: heavy,eccm 0\n"
        "  amount: 4;\n";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 1);
    const config::s_cfg_statement& statement = document.statements[0];
    DOCTEST_CHECK(statement.values.empty());
    DOCTEST_REQUIRE(statement.groups.size() == 2);
    DOCTEST_CHECK(statement.groups[0].selector.value == "mods");
    DOCTEST_REQUIRE(statement.groups[0].values.size() == 2);
    DOCTEST_CHECK(statement.groups[0].values[0].kind == config::CFG_TOKEN_MASK);
    DOCTEST_CHECK(statement.groups[0].values[1].kind == config::CFG_TOKEN_INTEGER);
    DOCTEST_CHECK(statement.groups[1].selector.value == "amount");
    DOCTEST_REQUIRE(statement.groups[1].values.size() == 1);
    DOCTEST_CHECK(statement.groups[1].values[0].value == "4");
}

DOCTEST_TEST_CASE("bare values escape structural colons and backslashes") {
    const char* source = "symbols = first: foo\\: package\\\\action \"quoted:\";";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 1);
    DOCTEST_REQUIRE(document.statements[0].groups.size() == 1);
    const std::vector<config::s_cfg_token>& values =
        document.statements[0].groups[0].values;
    DOCTEST_REQUIRE(values.size() == 3);
    DOCTEST_CHECK(values[0].raw == "foo\\:");
    DOCTEST_CHECK(values[0].value == "foo:");
    DOCTEST_CHECK(values[0].kind == config::CFG_TOKEN_IDENTIFIER);
    DOCTEST_CHECK(values[1].raw == "package\\\\action");
    DOCTEST_CHECK(values[1].value == "package\\action");
    DOCTEST_CHECK(values[2].value == "quoted:");
}

DOCTEST_TEST_CASE("include enable and stop statements retain arguments") {
    const char* source =
        "include balance/weapons.cfg;\n"
        "include \"local files/interface.cfg\";\n"
        "enable example-balance.mod;\n"
        "enable v150;\n"
        "stop;\n";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 5);
    DOCTEST_CHECK(document.statements[0].kind == config::CFG_STATEMENT_INCLUDE);
    DOCTEST_CHECK(document.statements[0].argument.kind == config::CFG_TOKEN_PATH);
    DOCTEST_CHECK(document.statements[0].argument.value == "balance/weapons.cfg");
    DOCTEST_CHECK(document.statements[1].kind == config::CFG_STATEMENT_INCLUDE);
    DOCTEST_CHECK(document.statements[1].argument.kind == config::CFG_TOKEN_STRING);
    DOCTEST_CHECK(document.statements[1].argument.value == "local files/interface.cfg");
    DOCTEST_CHECK(document.statements[2].kind == config::CFG_STATEMENT_ENABLE);
    DOCTEST_CHECK(document.statements[2].argument.value == "example-balance.mod");
    DOCTEST_CHECK(document.statements[3].kind == config::CFG_STATEMENT_ENABLE);
    DOCTEST_CHECK(document.statements[3].argument.value == "v150");
    DOCTEST_CHECK(document.statements[4].kind == config::CFG_STATEMENT_STOP);
}

DOCTEST_TEST_CASE("comments and multiline assignments preserve one-based locations") {
    const char* source =
        "# heading\r\n"
        "guardian weapon =\r\n"
        "  plasma_cannon # first value\r\n"
        "  all;\r\n";
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_REQUIRE(Parse_(source, &document, &diagnostic));
    DOCTEST_REQUIRE(document.statements.size() == 1);
    const config::s_cfg_statement& statement = document.statements[0];
    DOCTEST_CHECK(statement.location.source == "fixture.cfg");
    DOCTEST_CHECK(statement.location.line == 2);
    DOCTEST_CHECK(statement.location.column == 1);
    DOCTEST_REQUIRE(statement.values.size() == 2);
    DOCTEST_CHECK(statement.values[0].location.line == 3);
    DOCTEST_CHECK(statement.values[0].location.column == 3);
    DOCTEST_CHECK(statement.values[1].location.line == 4);
    DOCTEST_CHECK(statement.values[1].location.column == 3);
}

DOCTEST_TEST_CASE("syntax failures report their phase and clear partial output") {
    struct s_case {
        const char* source;
        const char* message;
        uint32_t line;
        uint32_t column;
    };
    const s_case cases[] = {
        {"music_enabled = ;", "assignment requires at least one value", 1, 17},
        {"music_enabled = 0x;", "malformed numeric value '0x'", 1, 17},
        {"music_enabled = 0b102;", "malformed numeric value '0b102'", 1, 17},
        {"music_enabled = 1.2;", "malformed numeric value '1.2'", 1, 17},
        {"music_enabled = +1;", "malformed numeric value '+1'", 1, 17},
        {"captain = \"bad\\q\";", "invalid string escape", 1, 16},
        {"captain = \"unterminated", "unterminated string", 1, 11},
        {"first = 1;\nsecond = 2", "unterminated assignment; expected ';'", 2, 1},
        {"enable;", "enable requires a mod ID", 1, 1},
        {"enable 150;", "enable requires a symbolic package ID", 1, 8},
        {"include;", "include requires a path", 1, 1},
        {"stop now;", "expected ';'", 1, 6},
        {"guardian = type:plasma_cannon;", "expected whitespace after labeled assignment selector", 1, 17},
        {"guardian = type: ;", "labeled assignment group requires at least one value", 1, 12},
        {"guardian = plasma_cannon type: 1;", "labeled assignment must begin with a selector", 1, 26},
        {"guardian = type: plasma_cannon type: 1;", "duplicate labeled assignment selector 'type'", 1, 32},
        {"value = foo\\q;", "invalid bare-token escape", 1, 12},
    };

    for (const s_case& test_case : cases) {
        config::s_cfg_document document;
        config::s_cfg_diagnostic diagnostic;
        DOCTEST_CHECK_FALSE(Parse_(test_case.source, &document, &diagnostic));
        DOCTEST_CHECK_MESSAGE(diagnostic.message == test_case.message,
                              test_case.source, ": ", diagnostic.message);
        DOCTEST_CHECK(diagnostic.location.line == test_case.line);
        DOCTEST_CHECK(diagnostic.location.column == test_case.column);
        DOCTEST_CHECK(document.statements.empty());
    }
}

DOCTEST_TEST_CASE("invalid UTF-8 is rejected before statement parsing") {
    const char source[] = {'n', 'a', 'm', 'e', ' ', '=', ' ', static_cast<char>(0xC3), ';'};
    config::s_cfg_document document;
    config::s_cfg_diagnostic diagnostic;

    DOCTEST_CHECK_FALSE(config::Parse_Cfg("invalid.cfg", source, sizeof(source),
                                         &document, &diagnostic));
    DOCTEST_CHECK(diagnostic.message == "source is not valid UTF-8");
    DOCTEST_CHECK(diagnostic.location.line == 1);
    DOCTEST_CHECK(diagnostic.location.column == 8);
}

}