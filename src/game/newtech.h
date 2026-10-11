#pragma once
#include <cstdint>

struct s_player;

namespace newtech {
    extern int16_t best_category[12];

    extern int32_t tech_research_level_values[30];

    struct s_ai_tech_priority {
        int16_t double_field_1;
        int16_t double_field_2;
        uint16_t double_application;
        uint16_t quintuple_application;
    };

    extern s_ai_tech_priority tech_priority;

    /**
     * Returns the highest field AI group any other player has fully researched for the specified application AI group.
     * @param target_ai_group Technology application AI group to compare.
     * @param exclude_player_idx Player index to skip during the comparison.
     */
    int16_t __cdecl Get_Competition_Tech_Values(int16_t target_ai_group, int16_t exclude_player_idx);

    /**
     * Caches the best competitor field levels used by the tech chooser for the specified player.
     * @param player_idx Player whose competition baseline is being prepared.
     */
    void __cdecl Set_Competition_Tech_Values(int16_t player_idx);

    /**
     * Returns the highest field AI group the player has fully researched within the specified application AI group.
     * @param player_idx Player to inspect.
     * @param target_ai_group Technology application AI group to compare.
     */
    int32_t __cdecl Get_Competition_Player_Tech_Value(int16_t player_idx, uint16_t target_ai_group);

    /**
     * Calculates the AI desirability score for researching a specific technology application.
     * @param player_idx Player whose research priorities are being evaluated.
     * @param tech_app_idx Technology application index to score.
     */
    int32_t __cdecl Calc_Tech_Value(int16_t player_idx, int16_t tech_app_idx);

    /**
     * Checks whether the player's required gray-box tech fields are all fully researched.
     * @param player Player state to inspect.
     */
    int8_t __cdecl All_Gray_Boxes_Researched(s_player* player);

    /** Randomly selects a fallback hyper-advanced technology application. */
    int32_t __cdecl Choose_Hyper_Advanced_Tech();

    /**
     * Chooses the next technology application for the player to research.
     * @param player_idx Player whose available research choices are being evaluated.
     */
    int16_t __cdecl Choose_Tech_Application(int16_t player_idx);
} // namespace newtech