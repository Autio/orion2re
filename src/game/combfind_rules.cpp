#include "pch.h"

namespace COMBFIND {
    bool Is_Star_Or_Antares_Location(int16_t location, int16_t star_count) {
        return location >= 0 && location <= star_count;
    }

    bool Wandering_Monster_Retires(
        int8_t monster_type, bool amoeba_toxicized_colony) {
        if (monsters_stay) {
            return false;
        }

        if (monster_type == NONPLAYER_SHIP_TYPE_AMOEBA) {
            return amoeba_toxicized_colony;
        }

        if (monster_type == NONPLAYER_SHIP_TYPE_CRYSTAL ||
            monster_type == NONPLAYER_SHIP_TYPE_DRAGON ||
            monster_type == NONPLAYER_SHIP_TYPE_HYDRA) {
            return game_random::Random(monsters_stay_chance) == 1;
        }

        return false;
    }
} // namespace COMBFIND