#include "pch.h"

#include "doctest/doctest.h"
#include "game/techdata.h"

namespace MOX {
    int16_t first_field_in_group[10] = {0, 18, 55, 57, 29, 7, 22, 28, 10, 74};
}

namespace {
    struct s_expected_strategic_damage {
        int16_t weapon_id;
        uint8_t minimum;
        uint8_t maximum;
    };

    constexpr s_expected_strategic_damage kExpectedDamage[] = {
        { WEAPON_GAUSS_CANNON, 18, 18 },
        { WEAPON_PARTICLE_BEAM, 10, 30 },
        { WEAPON_GRAVITON_BEAM, 3, 15 },
        { WEAPON_DISRUPTER, 40, 40 },
        { WEAPON_DEATH_RAY, 50, 100 },
        { WEAPON_PLASMA_CANNON, 6, 30 },
        { WEAPON_PLASMA_TORPEDO, 120, 120 },
        { WEAPON_MAULER_DEVICE, 100, 100 },
        { WEAPON_HEAVY_FIGHTER, 8, 32 },
        { WEAPON_BOMBER, 5, 20 },
        { WEAPON_PLASMA_WEB, 5, 25 },
        { WEAPON_DRAGON_BREATH, 250, 250 },
        { WEAPON_CRYSTAL_RAY, 40, 80 },
        { WEAPON_PLASMA_BREATH, 60, 60 },
        { WEAPON_PLASMA_FLUX, 10, 40 },
        { WEAPON_CAUSTIC_SLIME, 25, 50 },
    };

    constexpr int16_t kAlwaysHitWeaponsWithoutDoubleRangePenalty[] = {
        WEAPON_MAULER_DEVICE,
        WEAPON_PLASMA_BREATH,
    };

    struct s_expected_special_exclusions {
        int16_t special_id;
        uint16_t excluded[6];
    };

    constexpr s_expected_special_exclusions kExpectedCloakExclusions[] = {
        { SPECIAL_CLOAKING_DEVICE,
          { SPECIAL_PHASING_CLOAK, SPECIAL_CLOAKING_DEVICE,
            SPECIAL_STEALTH_FIELD, 0, 0, 0 } },
        { SPECIAL_PHASING_CLOAK,
          { SPECIAL_CLOAKING_DEVICE, SPECIAL_PHASING_CLOAK,
            SPECIAL_STEALTH_FIELD, 0, 0, 0 } },
        { SPECIAL_STEALTH_FIELD,
          { SPECIAL_STEALTH_FIELD, SPECIAL_CLOAKING_DEVICE,
            SPECIAL_PHASING_CLOAK, 0, 0, 0 } },
    };
}

DOCTEST_TEST_CASE("strategic weapon damage uses corrected estimates") {
    for (const s_expected_strategic_damage& expected : kExpectedDamage) {
        const s_tech_weapon_data& weapon =
            TECHDATA::weapons[expected.weapon_id];
        DOCTEST_CHECK(weapon.strat_min_damage == expected.minimum);
        DOCTEST_CHECK(weapon.strat_max_damage == expected.maximum);
    }
}

DOCTEST_TEST_CASE("orbital defenses compensate for corrected weapon space costs") {
    DOCTEST_CHECK(TECHDATA::hull_data[HULL_STAR_BASE].size == 400);
    DOCTEST_CHECK(TECHDATA::hull_data[HULL_BATTLESTATION].size == 800);
    DOCTEST_CHECK(TECHDATA::hull_data[HULL_STAR_FORTRESS].size == 1600);
}

DOCTEST_TEST_CASE("always-hit weapons do not use double range penalty") {
    for (int16_t weapon_id : kAlwaysHitWeaponsWithoutDoubleRangePenalty) {
        const uint16_t natural_mods = TECHDATA::weapons[weapon_id].natural_mods;
        DOCTEST_CHECK((natural_mods & WEAPON_NATURAL_FLAG_ALWAYS_HITS) != 0);
        DOCTEST_CHECK((natural_mods & WEAPON_NATURAL_FLAG_DOUBLE_RANGE_PENALTY) == 0);
    }
}

DOCTEST_TEST_CASE("stealth field and cloaks are mutually exclusive") {
    for (const s_expected_special_exclusions& expected :
         kExpectedCloakExclusions) {
        const s_tech_specials_data& special =
            TECHDATA::specials[expected.special_id];
        for (int32_t i = 0; i < 6; ++i) {
            DOCTEST_CHECK(special.exclude[i] == expected.excluded[i]);
        }
    }
}