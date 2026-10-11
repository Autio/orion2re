#pragma once

namespace lostship {
    /**
     * Removes ships that end the turn outside their allowed travel range.
     */
    void __cdecl Lose_Out_Of_Range_Ships();

    /**
     * Marks which stars are within a player's normal and extended ship range.
     * @param basic_range_array Output array for stars reachable with normal range.
     * @param extended_range_array Output array for stars reachable with extended range.
     * @param player_idx Player index to evaluate.
     */
    void __cdecl Compute_Stars_In_Range_Of_Player(int8_t* basic_range_array, int8_t* extended_range_array, int16_t player_idx);

    /**
     * Applies out-of-range ship loss rules for a single player.
     * @param basic_range_array Stars reachable with normal ship range.
     * @param extended_range_array Stars reachable with extended ship range.
     * @param player_idx Player index to process.
     */
    void __cdecl Player_Lose_Out_Of_Range_Ships(int8_t* basic_range_array, int8_t* extended_range_array, int16_t player_idx);
} // namespace lostship