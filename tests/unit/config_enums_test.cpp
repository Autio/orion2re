#include "config/generated/config_enums.h"

#include <doctest/doctest.h>

#include <cstring>

TEST_CASE("generated configuration enums provide canonical bidirectional lookup") {
    const config::s_generated_enum* applications =
        config::Find_Generated_Enum("technology_application");
    REQUIRE(applications != nullptr);
    CHECK(applications->value_count == 212);

    const config::s_generated_enum_symbol* plasma =
        config::Find_Generated_Enum_Symbol(applications, "plasma_cannon");
    REQUIRE(plasma != nullptr);
    CHECK(plasma->value == 137);
    CHECK(config::Find_Generated_Enum_Value(applications, 137) == plasma);

    const config::s_generated_enum* fields =
        config::Find_Generated_Enum("technology_field_index");
    REQUIRE(fields != nullptr);
    CHECK(fields->value_count == 83);
    REQUIRE(config::Find_Generated_Enum_Value(fields, 59) != nullptr);
    CHECK(std::strcmp(
              config::Find_Generated_Enum_Value(fields, 59)->name,
              "plasma_physics") == 0);

    const config::s_generated_enum* statuses =
        config::Find_Generated_Enum("technology_status");
    REQUIRE(statuses != nullptr);
    CHECK(statuses->value_count == 3);
    REQUIRE(config::Find_Generated_Enum_Value(statuses, 2) != nullptr);
    CHECK(std::strcmp(
              config::Find_Generated_Enum_Value(statuses, 2)->name,
              "researched") == 0);

    CHECK(config::Find_Generated_Enum(nullptr) == nullptr);
    CHECK(config::Find_Generated_Enum("missing") == nullptr);
    CHECK(config::Find_Generated_Enum_Symbol(nullptr, "missing") == nullptr);
    CHECK(config::Find_Generated_Enum_Symbol(fields, nullptr) == nullptr);
    CHECK(config::Find_Generated_Enum_Value(nullptr, 0) == nullptr);
    CHECK(config::Find_Generated_Enum_Value(fields, -1) == nullptr);

    const config::s_generated_enum missing_values = {
        "malformed",
        nullptr,
        1,
    };
    CHECK(config::Find_Generated_Enum_Symbol(
              &missing_values, "missing") == nullptr);
    CHECK(config::Find_Generated_Enum_Value(&missing_values, 0) == nullptr);

    const config::s_generated_enum_symbol unnamed[] = {
        {nullptr, 0},
    };
    const config::s_generated_enum unnamed_symbol = {
        "malformed",
        unnamed,
        std::size(unnamed),
    };
    CHECK(config::Find_Generated_Enum_Symbol(
              &unnamed_symbol, "missing") == nullptr);
}