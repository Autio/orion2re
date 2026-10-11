#include "pch.h"

namespace spy {
    int8_t lookup_table[GOVERNMENT_COUNT] = {0, 0, 10, 15, -10, -10, 15, 15};

    s_spy_tech_bonus spy_tech_bonus = {
        10,
        0,
        5,
        10,
        10,
        10,
    };

    int16_t __cdecl Tech_Spy_Bonus(s_player* player) {
        int16_t bonus = 0;

        if (player->tech_applications[TECH_APP_NEURAL_SCANNER] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.neural_scanner;
        }

        if (player->tech_applications[TECH_APP_XENO_PSYCHOLOGY] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.xeno_psychology;
        }

        if (player->tech_applications[TECH_APP_TELEPATHIC_TRAINING] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.telepathic_training;
        }

        if (player->tech_applications[TECH_APP_CYBER_SECURITY_LINK] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.cyber_security_link;
        }

        if (player->tech_applications[TECH_APP_STEALTH_SUIT] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.stealth_suit;
        }

        if (player->tech_applications[TECH_APP_PSIONICS] == TECH_RESEARCH_STATUS_RESEARCHED) {
            bonus += spy_tech_bonus.psionics;
        }

        return bonus;
    }
} // namespace spy