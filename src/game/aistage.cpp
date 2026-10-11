#include "pch.h"

namespace AISTAGE {

    void __cdecl Compute_Defensive_Stage_Points(int16_t player_idx) {
        uint8_t processed_stars[MAX_STARS];
        int16_t selected_stagepoint_count = 0;
        bool finished;
        uint32_t best_weight;
        int16_t best_star;

        if (MOX::player[player_idx].eliminated != 0) {
            return;
        }

        memset(processed_stars, 0, sizeof(processed_stars));
        MOX::player[player_idx].offensive_stagepoint = -1;

        for (int16_t i = 0; i < MOX::NUM_STARS; ++i) {
            MOX::star[i].is_stagepoint &= ~(1 << (player_idx & 0x1F));
        }

        do {
            finished = false;
            best_star = -1;
            best_weight = 0;

            for (int16_t i = 0; i < MOX::NUM_STARS; ++i) {
                if (processed_stars[i] == 0) {
                    uint32_t weight = (*AIDATA::star_stage_weights)[i][player_idx];
                    if (best_weight < weight) {
                        best_weight = weight;
                        best_star = i;
                    }
                }
            }

            if (best_star == -1) {
                finished = true;
            } else {
                MOX::star[best_star].is_stagepoint |= (1 << (player_idx & 0x1F));

                if (selected_stagepoint_count == 0) {
                    MOX::player[player_idx].offensive_stagepoint = best_star;
                }

                ++selected_stagepoint_count;
                processed_stars[best_star] = 1;

                for (int16_t i = 0; i < MOX::NUM_STARS; ++i) {
                    if (i != best_star) {
                        if ((*AIDATA::star_stage_weights)[i][player_idx] != 0 &&
                            (MOX::star[i].is_stagepoint & (1 << (player_idx & 0x1F))) == 0) {
                            if (AIDATA::N_Turns_From_To(best_star, i, player_idx) <= 1) {
                                (*AIDATA::star_stage_weights)[i][player_idx] = 0;
                                processed_stars[i] = 1;
                            }
                        }
                    }
                }
            }
        } while (!finished);

        if (selected_stagepoint_count == 0) {
            uint32_t max_weight = 0;
            int16_t candidate_star = -1;
            int32_t search_speed = (uint8_t)MOX::player[player_idx].ship_speed;

            do {
                for (int16_t i = 0; i < MOX::NUM_STARS; ++i) {
                    if (((MOX::star[i].has_colony >> (player_idx & 0x1F)) & 1) != 0) {
                        uint8_t in_range_mask = (*AIDATA::star_in_range_of_player)[i];
                        if ((in_range_mask & ~(1 << (player_idx & 0x1F))) != 0) {
                            for (int16_t other_player_idx = 0; other_player_idx < MOX::NUM_PLAYERS; ++other_player_idx) {
                                if (other_player_idx != player_idx && ((in_range_mask >> (other_player_idx & 0x1F)) & 1) != 0) {
                                    if (SHIPMOVE::Star_In_Range_Of_Player(&MOX::star[i], other_player_idx, (int16_t)search_speed)) {
                                        uint32_t weight = (*AIDATA::base_star_stage_weights)[i][player_idx];
                                        if (max_weight < weight) {
                                            max_weight = weight;
                                            candidate_star = i;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (candidate_star != -1) break;
                search_speed += 3;
            } while (search_speed < 50);

            if (candidate_star == -1) {
                int16_t ship_range = MOX::player[player_idx].ship_range;
                for (int16_t owned_star_idx = 0; owned_star_idx < MOX::NUM_STARS; ++owned_star_idx) {
                    if (((MOX::star[owned_star_idx].has_colony >> (player_idx & 0x1F)) & 1) != 0) {
                        for (int16_t enemy_colony_star_idx = 0; enemy_colony_star_idx < MOX::NUM_STARS; ++enemy_colony_star_idx) {
                            if (((MOX::star[enemy_colony_star_idx].has_colony & ~(1 << (player_idx & 0x1F))) != 0)) {
                                if (GEO::Parsecs_Between_Stars(owned_star_idx, enemy_colony_star_idx) <= ship_range) {
                                    uint32_t weight = (*AIDATA::base_star_stage_weights)[owned_star_idx][player_idx];
                                    if (max_weight < weight) {
                                        max_weight = weight;
                                        candidate_star = owned_star_idx;
                                    }
                                }
                            }
                        }
                    }
                }

                if (candidate_star == -1) {
                    for (int16_t i = 0; i < MOX::NUM_STARS; ++i) {
                        if (((MOX::star[i].has_colony >> (player_idx & 0x1F)) & 1) != 0) {
                            uint32_t weight = (*AIDATA::base_star_stage_weights)[i][player_idx];
                            if (max_weight <= weight) {
                                max_weight = weight;
                                candidate_star = i;
                            }
                        }
                    }
                }
            }

            MOX::player[player_idx].offensive_stagepoint = candidate_star;
            MOX::star[candidate_star].is_stagepoint |= (1 << (player_idx & 0x1F));
        }
    }

} // namespace AISTAGE