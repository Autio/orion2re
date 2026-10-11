#include "pch.h"

#include <cstring>

#include "doctest/doctest.h"

namespace {
    void Check_Weapon_Damage_String_(int16_t weapon_id, const char* expected) {
        char buffer[32];
        DESIGN::Design_Weapon_Damage_String(weapon_id, buffer, sizeof(buffer), 0, 0);
        DOCTEST_CHECK(std::strcmp(buffer, expected) == 0);
    }
}

DOCTEST_TEST_CASE("Death Spore and Bio Terminator damage displays are independently configurable") {
    TECHDATA::weapons[WEAPON_DEATH_SPORE].min_damage = 4;
    TECHDATA::weapons[WEAPON_DEATH_SPORE].max_damage = 6;
    TECHDATA::weapons[WEAPON_BIO_TERMINATOR].min_damage = 8;
    TECHDATA::weapons[WEAPON_BIO_TERMINATOR].max_damage = 8;

    DESIGN::death_spore_list_damage = 0;
    DESIGN::bio_terminator_list_damage = 0;
    Check_Weapon_Damage_String_(WEAPON_DEATH_SPORE, "special");
    Check_Weapon_Damage_String_(WEAPON_BIO_TERMINATOR, "special");
    Check_Weapon_Damage_String_(WEAPON_STASIS_FIELD, "special");

    DESIGN::death_spore_list_damage = 1;
    Check_Weapon_Damage_String_(WEAPON_DEATH_SPORE, "4-6");
    Check_Weapon_Damage_String_(WEAPON_BIO_TERMINATOR, "special");

    DESIGN::bio_terminator_list_damage = 1;
    Check_Weapon_Damage_String_(WEAPON_BIO_TERMINATOR, "8");
    Check_Weapon_Damage_String_(WEAPON_STASIS_FIELD, "special");
}