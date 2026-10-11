#include "pch.h"

namespace AIBUILD {
    uint8_t difficulty_ship_design[12] = {2, 10, 35, 50, 115, 0, 0, 0, 45, 0, 0, 0};
    int8_t difficulty_intel_bias[8] = {1, 1, 2, 3, 4, 0, 0, 0};
    uint8_t player_ship_size_table[HULL_CLASS_COUNT] = {1, 2, 3, 5, 8, 13, 0, 0, 0};
    s_player_ship_sizes player_ship_sizes[MAX_PLAYERS];
    int32_t player_total_strength_vs_colonies[MAX_PLAYERS];
    uint8_t player_n_transports[MAX_PLAYERS][2];
    int8_t player_n_freighter_groups_wanted[MAX_PLAYERS];
    uint8_t player_n_spies[MAX_PLAYERS][2];
    uint8_t player_n_colony_ships[MAX_PLAYERS][3];
    int32_t player_total_strength_vs_ships[MAX_PLAYERS][3];

    int16_t __cdecl Mean_Ship_Strength(s_ship_data* ship) {
        int16_t ship_idx = (int16_t)(ship - MOX::ship);
        int32_t owner = (int32_t)ship->owner;
        int16_t result_strength = 0;

        if ((*AIDATA::ai_player_info)[owner].n_reachable_players == 0) {
            uint32_t total_str = 0;
            uint32_t count = 0;

            for (int16_t i = 0; i < (int16_t)MOX::NUM_PLAYERS; ++i) {
                if (i != owner && !MOX::player[i].eliminated && MOX::player[i].objectives == PLAYER_OBJECTIVE_HUMAN) {
                    count++;
                    total_str += (uint16_t)(*AIDATA::ship_strengths)[ship_idx][i];
                }
            }

            if (count != 0) {
                result_strength = (int16_t)(total_str / count);
            }
        } else {
            uint32_t count_human = 0;
            uint32_t total_human = 0;
            uint32_t min_human = 0xffffffff;

            uint32_t count_other = 0;
            uint32_t total_other = 0;
            uint32_t min_other = 0xffffffff;

            for (int16_t i = 0; i < (int16_t)MOX::NUM_PLAYERS; ++i) {
                if (i != owner && !MOX::player[i].eliminated && (*AIDATA::ai_player_info)[owner].reachable_players[i]) {
                    uint32_t ship_strength = (uint16_t)(*AIDATA::ship_strengths)[ship_idx][i];

                    if (MOX::player[i].objectives == PLAYER_OBJECTIVE_HUMAN) {
                        total_human += ship_strength;
                        if (ship_strength < min_human) {
                            min_human = ship_strength;
                        }
                        count_human++;
                    } else {
                        total_other += ship_strength;
                        if (ship_strength < min_other) {
                            min_other = ship_strength;
                        }
                        count_other++;
                    }
                }
            }

            if (count_human == 0) {
                if (count_other != 0) {
                    result_strength = (int16_t)(total_other / count_other);
                }
            } else if (count_other == 0) {
                result_strength = (int16_t)(total_human / count_human);
            } else {
                if (min_other < min_human) {
                    result_strength = (int16_t)min_other;
                } else {
                    uint32_t bias = (uint8_t)AIBUILD::difficulty_intel_bias[MOX::settings.game_difficulty];
                    result_strength = (int16_t)((min_other + bias * min_human) / (bias + 1));
                }
            }
        }

        return result_strength;
    }

    int16_t __cdecl Refit_Cost(int16_t player_idx, s_ship_design* old_design, s_ship_design* new_design) {
        int32_t min_cost =
            (uint16_t)TECHDATA::hull_data[(int16_t)new_design->size].cost / 4;
        int8_t government =
            MOX::player[player_idx].traits[TRAIT_CURRENT_GOVERNMENT];
        if (government == GOVERNMENT_FEUDAL ||
            government == GOVERNMENT_CONFEDERATION) {
            DESIGN::Cost_Reduction_For_Govt_Type(player_idx, &min_cost);
        }

        int32_t delta_cost =
            (int32_t)new_design->cost - (int32_t)old_design->cost;
        if (DESIGN::alternative_refit != 0) {
            if (delta_cost < 0) {
                delta_cost = 0;
            }
            return (int16_t)(delta_cost + min_cost);
        }

        int32_t refit_cost = delta_cost * 2;
        return (int16_t)(refit_cost < min_cost ? min_cost : refit_cost);
    }

    int16_t __cdecl Player_War_Level(int16_t player_idx) {
        int16_t max_war_level = 0;

        for (int16_t i = 0; i < MOX::NUM_PLAYERS; ++i) {
            if (player_idx != i && MOX::player[player_idx].contact[i] != 0) {
                int8_t treaty_type = MOX::player[player_idx].treaty[i];

                if (treaty_type == 4) {
                    if (max_war_level < 1) {
                        max_war_level = 1;
                    }
                } else if (treaty_type == 5) {
                    if (max_war_level < 2) {
                        max_war_level = 2;
                    }
                } else if (treaty_type == 6) {
                    if (max_war_level < 3) {
                        max_war_level = 3;
                    }
                }
            }
        }

        return max_war_level;
    }

    void __cdecl Compute_Empire_Building_Needs() {
        RUSS::Mox_Update();

        memset(player_total_strength_vs_ships, 0, sizeof(player_total_strength_vs_ships));
        memset(player_ship_sizes, 0, sizeof(player_ship_sizes));
        memset(player_total_strength_vs_colonies, 0, sizeof(player_total_strength_vs_colonies));
        memset(player_n_colony_ships, 0, sizeof(player_n_colony_ships));
        memset(player_n_transports, 0, sizeof(player_n_transports));

        for (int16_t i = 0; i < MOX::NUM_SHIPS; ++i) {
            s_ship_data* ship = &MOX::ship[i];

            if ((ship->status < 5 || ship->status == 6) && ship->owner < MAX_PLAYERS) {
                int8_t owner = (int8_t)ship->owner;

                if (ship->d.ship_type == SHIP_TYPE_COMBAT) {
                    int32_t strength = (int32_t)Mean_Ship_Strength(ship);
                    player_total_strength_vs_ships[owner][0] += strength;

                    if (ship->status != 6) {
                        player_total_strength_vs_ships[owner][1] += (int32_t)Mean_Ship_Strength(ship);
                    }

                    player_ship_sizes[owner].ship_counts[(int32_t)ship->d.size]++;
                    player_ship_sizes[owner].weighted_ship_size += (int32_t)player_ship_size_table[(int32_t)ship->d.size];
                    player_ship_sizes[owner].combat_command_points_used += ((int32_t)ship->d.size + 1);
                    player_ship_sizes[owner].combat_ship_count++;

                    if (ship->status != 6) {
                        player_ship_sizes[owner].active_combat_command_points_used += ((int32_t)ship->d.size + 1);
                    }
                } else if (ship->d.ship_type == SHIP_TYPE_COLONY) {
                    player_n_colony_ships[owner][0]++;
                    if (ship->status == 1 || ship->status == 2) {
                        player_n_colony_ships[owner][2]++;
                    }
                } else if (ship->d.ship_type == SHIP_TYPE_TRANSPORT) {
                    player_n_transports[owner][0]++;
                }
            }
        }

        RUSS::Mox_Update();
        memset(player_n_spies, 0, sizeof(player_n_spies));

        if (MOX::NUM_PLAYERS > 0) {
            for (int16_t owner = 0; owner < MOX::NUM_PLAYERS; ++owner) {
                for (int16_t target = 0; target < MOX::NUM_PLAYERS; ++target) {
                    int16_t spy_count = BILL::Get_Their_Spy_Number(owner, target);
                    player_n_spies[owner][0] += (int8_t)spy_count;
                }
            }
        }

        RUSS::Mox_Update();
        for (int16_t player = 0; player < MOX::NUM_PLAYERS; ++player) {
            Compute_Needs(player);
        }
    }

    void __cdecl Compute_Colony_Ship_Need(int16_t player_idx) {
        if (player_n_colony_ships[player_idx][0] == 0) {
            player_n_colony_ships[player_idx][1] = 1;
        }
    }

    void __cdecl Compute_Freighter_Need(int16_t player_idx) {
        int16_t surplus = MOX::player[player_idx].surplus_freighters;

        int16_t colony_ship_need = player_n_colony_ships[player_idx][2];

        player_n_freighter_groups_wanted[player_idx] = 0;

        int16_t freighter_shortfall = surplus - colony_ship_need;

        if (freighter_shortfall < 0) {
            player_n_freighter_groups_wanted[player_idx] = (int8_t)((4 - (int32_t)freighter_shortfall) / 5);
        }
    }

    void __cdecl Compute_Needs(int16_t player_idx) {
        bool is_militarist;
        int32_t highest_ship_size_idx;
        int32_t total_contacts;
        int32_t total_transports;
        int32_t max_transports;
        int32_t sum_strength_vs_ships;
        int32_t max_strength_vs_ships;
        int32_t max_strength_vs_colonies;
        int32_t max_spies;
        int32_t max_ship_size_13;
        int32_t strength_target;

        is_militarist = (MOX::player[player_idx].objectives == PLAYER_OBJECTIVE_MILITARIST);
        highest_ship_size_idx = 0;
        total_contacts = 0;
        max_transports = 0;
        total_transports = 0;
        max_strength_vs_ships = 0;
        sum_strength_vs_ships = 0;
        max_strength_vs_colonies = 0;
        max_spies = 0;
        max_ship_size_13 = 0;

        for (int16_t i = 0; i < MOX::NUM_PLAYERS; ++i) {
            if (player_idx != i) {
                bool aware = ((*AIDATA::ai_player_info)[player_idx].reachable_players[i] != 0);
                if (!aware) {
                    if ((*AIDATA::ai_player_info)[player_idx].n_reachable_players == 0 && MOX::player[i].objectives == PLAYER_OBJECTIVE_HUMAN) {
                        aware = true;
                    }
                }

                if (aware) {
                    total_contacts++;
                    int32_t transports = AIBUILD::player_n_transports[i][0];
                    total_transports += transports;
                    if (max_transports < transports) {
                        max_transports = transports;
                    }

                    int32_t ship_strength = AIBUILD::player_total_strength_vs_ships[i][0];
                    sum_strength_vs_ships += ship_strength;
                    if (max_strength_vs_ships < ship_strength) {
                        max_strength_vs_ships = ship_strength;
                    }

                    if (max_strength_vs_colonies < AIBUILD::player_total_strength_vs_colonies[i]) {
                        max_strength_vs_colonies = AIBUILD::player_total_strength_vs_colonies[i];
                    }

                    int32_t spies = AIBUILD::player_n_spies[i][0];
                    if (max_spies < spies) {
                        max_spies = spies;
                    }

                    if (max_ship_size_13 < AIBUILD::player_ship_sizes[i].weighted_ship_size) {
                        max_ship_size_13 = AIBUILD::player_ship_sizes[i].weighted_ship_size;
                    }

                    for (int16_t j = 0; j < HULL_CLASS_COUNT; ++j) {
                        if (AIBUILD::player_ship_sizes[i].ship_counts[j] != 0 && highest_ship_size_idx < j) {
                            highest_ship_size_idx = j;
                        }
                    }
                }
            }
        }

        AIBUILD::player_ship_sizes[player_idx].target_ship_size = highest_ship_size_idx + 1;
        if (max_strength_vs_ships < AIBUILD::player_total_strength_vs_ships[player_idx][0] ||
            (*AIDATA::ai_player_info)[player_idx].n_reachable_players == 0) {
            int32_t rand_val = game_random::Random(2);
            AIBUILD::player_ship_sizes[player_idx].target_ship_size = highest_ship_size_idx + 1 + rand_val + (is_militarist ? 1 : 0) - 1;
        }

        if (total_contacts == 0) {
            total_contacts = 1;
        }

        AIBUILD::player_n_transports[player_idx][1] = 0;
        if (MOX::player[player_idx].tech_applications[TECH_APP_TRANSPORT] == TECH_RESEARCH_STATUS_RESEARCHED && AIBUILD::player_ship_sizes[player_idx].weighted_ship_size > 1) {
            int32_t colony_limit = ((*AIDATA::ai_player_info)[player_idx].colony_count + 2) / 3;
            int32_t transport_avg = (max_transports + 1 + total_transports / total_contacts) / 2;

            if (AIBUILD::player_n_transports[player_idx][0] < transport_avg) {
                AIBUILD::player_n_transports[player_idx][1] = (int8_t)(transport_avg - AIBUILD::player_n_transports[player_idx][0]);
            }

            if ((*AIDATA::ai_player_info)[player_idx].n_reachable_players > 0) {
                int32_t missing_need = (is_militarist ? 1 : 0) + (*AIDATA::ai_player_info)[player_idx].n_reachable_players + (int32_t)special::isqrt((*AIDATA::ai_player_info)[player_idx].colony_count);
                if (AIBUILD::player_n_transports[player_idx][0] < missing_need) {
                    int32_t transport_shortfall = missing_need - AIBUILD::player_n_transports[player_idx][0];
                    if (AIBUILD::player_n_transports[player_idx][1] < transport_shortfall) {
                        AIBUILD::player_n_transports[player_idx][1] = (int8_t)transport_shortfall;
                    }
                }
            }

            if (AIBUILD::player_n_transports[player_idx][0] < 20) {
                if (colony_limit < AIBUILD::player_n_transports[player_idx][1]) {
                    AIBUILD::player_n_transports[player_idx][1] = (int8_t)colony_limit;
                    if ((*AIDATA::ai_player_info)[player_idx].n_reachable_players == 0) {
                        AIBUILD::player_n_transports[player_idx][1] >>= 2;
                    }
                }
            } else {
                AIBUILD::player_n_transports[player_idx][1] = 0;
            }

            if (AIBUILD::player_n_transports[player_idx][1] > 1 && MOX::player[player_idx].command_points < MOX::player[player_idx].command_points_used) {
                AIBUILD::player_n_transports[player_idx][1] = 1;
            }
        }

        if (is_militarist) {
            strength_target = max_strength_vs_ships;
        } else {
            strength_target = (max_strength_vs_ships + 1 + sum_strength_vs_ships / total_contacts) / 2;
        }

        AIBUILD::player_total_strength_vs_ships[player_idx][2] = 0;
        if (AIBUILD::player_total_strength_vs_ships[player_idx][0] < strength_target) {
            int32_t strength_shortfall = strength_target - AIBUILD::player_total_strength_vs_ships[player_idx][0];
            AIBUILD::player_total_strength_vs_ships[player_idx][2] = strength_shortfall;
            if ((*AIDATA::ai_player_info)[player_idx].n_reachable_players == 0) {
                AIBUILD::player_total_strength_vs_ships[player_idx][2] = strength_shortfall / 2;
            }
        }

        int32_t current_need = AIBUILD::player_total_strength_vs_ships[player_idx][2];
        if (current_need == 0 || MOX::player[player_idx].command_points_used < MOX::player[player_idx].command_points) {
            if ((*AIDATA::ai_player_info)[player_idx].n_reachable_players > 0) {
                if (game_random::Random(10 - (is_militarist ? 1 : 0)) == 1) {
                    AIBUILD::player_total_strength_vs_ships[player_idx][2] += (is_militarist ? 4 : 0) + 1;
                }
            }
        } else if (MOX::player[player_idx].bc < 50 || MOX::player[player_idx].surplus_bc < 50) {
            AIBUILD::player_total_strength_vs_ships[player_idx][2] = 0;
        } else if (MOX::player[player_idx].surplus_bc < 1024) {
            int32_t scaled_need = (special::isqrt(MOX::player[player_idx].surplus_bc) * current_need) >> 5;
            AIBUILD::player_total_strength_vs_ships[player_idx][2] = scaled_need;
            if (Player_War_Level(player_idx) == 0) {
                AIBUILD::player_total_strength_vs_ships[player_idx][2] = special::isqrt(scaled_need) >> 4;
            }
        }

        if (MOX::player[player_idx].surplus_bc < 1024) {
            int32_t divisor = AIBUILD::player_ship_sizes[player_idx].weighted_ship_size;
            if (max_ship_size_13 < divisor) {
                AIBUILD::player_total_strength_vs_ships[player_idx][2] = (AIBUILD::player_total_strength_vs_ships[player_idx][2] * max_ship_size_13) / divisor;
            }
        } else {
            if (AIBUILD::player_total_strength_vs_ships[player_idx][2] <= 9 && AIBUILD::player_ship_sizes[player_idx].active_combat_command_points_used != 0) {
                int32_t ratio = AIBUILD::player_total_strength_vs_ships[player_idx][1] / AIBUILD::player_ship_sizes[player_idx].active_combat_command_points_used;
                if (AIBUILD::player_ship_sizes[player_idx].combat_command_points_used < MOX::player[player_idx].command_points) {
                    AIBUILD::player_total_strength_vs_ships[player_idx][2] = (MOX::player[player_idx].command_points - AIBUILD::player_ship_sizes[player_idx].combat_command_points_used) * ratio;
                } else {
                    AIBUILD::player_total_strength_vs_ships[player_idx][2] = (MOX::player[player_idx].surplus_bc * ratio) / 15;
                }
            }
        }

        AIBUILD::player_n_spies[player_idx][1] = 0;
        if (AIBUILD::player_total_strength_vs_ships[player_idx][0] > 0) {
            int32_t spy_limit = 0x3F - BILL::Get_Their_Spy_Number(player_idx, player_idx);
            if ((*AIDATA::ai_player_info)[player_idx].n_contacts > 0 && spy_limit > 0) {
                int32_t current_spies = AIBUILD::player_n_spies[player_idx][0];
                if (current_spies < max_spies) {
                    AIBUILD::player_n_spies[player_idx][1] = (uint8_t)(max_spies - current_spies);
                }

                int32_t spy_trait = MOX::player[player_idx].traits[TRAIT_SPYING_BONUS];
                if (current_spies < max_spies + 2 && (int32_t)game_random::Random(100) <= spy_trait) {
                    AIBUILD::player_n_spies[player_idx][1]++;
                }

                if (MOX::player[player_idx].objectives == PLAYER_OBJECTIVE_DIPLOMATIC && game_random::Random(2) == 1) {
                    AIBUILD::player_n_spies[player_idx][1]++;
                }

                uint8_t spy_need = AIBUILD::player_n_spies[player_idx][1];
                if (spy_need != 0) {
                    int32_t cap = (*AIDATA::ai_player_info)[player_idx].n_contacts * ((MOX::player[player_idx].traits[TRAIT_SPYING_BONUS] > 0 ? 1 : 0) + 1) * 2;
                    if (cap < current_spies) {
                        if (spy_need < 2) {
                            if (MOX::player[player_idx].traits[TRAIT_SPYING_BONUS] < 1 && game_random::Random(4) != 1) {
                                AIBUILD::player_n_spies[player_idx][1] = 0;
                            } else {
                                AIBUILD::player_n_spies[player_idx][1] = 1;
                            }
                        } else {
                            AIBUILD::player_n_spies[player_idx][1] = (uint8_t)special::isqrt(spy_need);
                        }
                    }
                }

                if (spy_limit < AIBUILD::player_n_spies[player_idx][1]) {
                    AIBUILD::player_n_spies[player_idx][1] = (uint8_t)spy_limit;
                }
            }
        }

        Compute_Colony_Ship_Need(player_idx);
        Compute_Freighter_Need(player_idx);
    }

    void __cdecl Empire_Buy_Outright(int16_t player_idx) {
        s_player* player = &MOX::player[player_idx];
        int16_t total_maintenance = (int16_t)(uint16_t)player->total_maintenance;

        int32_t limit = (player->bc * 3) / 4;
        if (limit > 1500) {
            limit = 1500;
        } else if (limit < 50) {
            limit = 50;
        }

        int16_t chosen_colony;
        do {
            bool has_threatened = false;
            uint16_t max_weight_threatened = 0;
            uint16_t max_weight_safe = 0;
            uint16_t weights[MAX_COLONIES];
            memset(weights, 0, sizeof(weights));

            for (int16_t i = 0; i < MOX::NUM_COLONIES; ++i) {
                s_colony* colony = &MOX::colony[i];

                if (colony->owner != (int8_t)player_idx) continue;
                if (colony->bought_outright != 0) continue;

                int16_t planet_idx = colony->planet;
                int16_t star_idx = MOX::planet[planet_idx].star_index;
                if ((MOX::star[star_idx].blockaded >> (player_idx & 0x1F)) & 1) continue;

                int16_t producing = colony->producing[0];
                if (producing == COLONY_PRODUCTION_NONE ||
                    producing == COLONY_PRODUCTION_TRADE_GOODS ||
                    producing == COLONY_PRODUCTION_HOUSING) continue;

                int32_t cost = COLCALC::Colony_Cost_To_Buy_Producing_Product(colony);
                if (cost <= 0) continue;

                if ((int32_t)total_maintenance + cost >= player->bc) continue;

                s_ai_colony_info* ai_info = &(*AIDATA::ai_colony_info)[i];
                if (ai_info->threatened == 0) {
                    if (player->bc - (int32_t)total_maintenance - cost < limit) continue;
                }

                int32_t turns = (int32_t)COLCALC::Colony_N_Turns_To_Produce(colony);
                if (turns < 0 || turns > 50) {
                    turns = 50;
                }

                uint32_t weight = ((turns * turns * 1000) / (cost * cost)) + 1;
                if (weight > 0xFFFF) {
                    weight = 0xFFFF;
                }
                weights[i] = (uint16_t)weight;

                if (ai_info->threatened != 0) {
                    has_threatened = true;
                    if (weights[i] > max_weight_threatened) {
                        max_weight_threatened = weights[i];
                    }
                } else {
                    if (weights[i] > max_weight_safe) {
                        max_weight_safe = weights[i];
                    }
                }
            }

            if (max_weight_safe != 0 || max_weight_threatened != 0) {
                for (int16_t i = 0; i < MOX::NUM_COLONIES; ++i) {
                    if (weights[i] == 0) continue;

                    if (has_threatened) {
                        s_ai_colony_info* ai_info = &(*AIDATA::ai_colony_info)[i];
                        if (ai_info->threatened == 0 || (uint32_t)weights[i] * 4 < (uint32_t)max_weight_threatened * 3) {
                            weights[i] = 0;
                        }
                    } else {
                        if ((uint32_t)weights[i] * 4 < (uint32_t)max_weight_safe * 3) {
                            weights[i] = 0;
                        }
                    }
                }
            }

            chosen_colony = RUSS::Get_Weighted_Choice_Short(weights, (int16_t)MOX::NUM_COLONIES);
            if (chosen_colony != -1) {
                COLCALC::Colony_Buy_Outright(chosen_colony);
            }

            RUSS::Mox_Update();

        } while (chosen_colony != -1);
    }

    void __cdecl Assign_Colony_Building(s_colony* colony) {
        if (colony->outpost_flag == 0 && colony->n_pops != 0) {
            int16_t current_production = colony->producing[0];

            if (current_production == COLONY_PRODUCTION_AI_UNASSIGNED ||
                current_production == COLONY_PRODUCTION_NONE ||
                current_production == COLONY_PRODUCTION_TRADE_GOODS ||
                current_production == COLONY_PRODUCTION_HOUSING) {
                Assign_Colony_New_Building(colony);
            }
        }
    }

    void __cdecl Find_Buildable_Buildings(s_colony* colony, uint16_t* buildable_flags) {
        for (int16_t product_id = BUILDING_NO_BUILDING + 1; product_id < BUILDING_COUNT; ++product_id) {
            if (COLCALC::Colony_Can_Build_Product(colony, product_id) == 1) {
                buildable_flags[product_id] = 1;
            }
        }
    }

    void Build_Production_Context(const s_colony* colony, int32_t war_level, production_rules::s_context* context) {
        memset(context, 0, sizeof(*context));
        int32_t owner = (uint8_t)colony->owner;
        int16_t colony_idx = (int16_t)(colony - MOX::colony);
        const s_player* player = &MOX::player[owner];
        const s_planet_data* planet = &MOX::planet[colony->planet];
        const s_ai_colony_info* ai_colony = &(*AIDATA::ai_colony_info)[colony_idx];
        const s_ai_star_player_block* star_info =
            &(*AIDATA::ai_star_info)[planet->star_index].per_player[owner];

        int32_t population = (uint8_t)colony->n_pops;
        int32_t population_limit = (uint8_t)ai_colony->pop_limit;
        int32_t colony_food = colony->production[ECON_FOOD] - colony->n_food_replicated;
        int32_t food2_demand = (uint8_t)colony->food2_needed_for_our_empire +
            (uint8_t)colony->food2_needed_for_assimilated +
            (uint8_t)colony->food2_needed_for_conquered +
            (uint8_t)colony->food2_needed_for_natives;
        if (war_level < 0) war_level = Player_War_Level(owner);

        context->metrics[production_rules::METRIC_POPULATION] = population;
        context->metrics[production_rules::METRIC_POPULATION_LIMIT] = population_limit;
        context->metrics[production_rules::METRIC_FREE_POPULATION_CAPACITY] = population_limit - population;
        context->metrics[production_rules::METRIC_COLONY_FOOD] = colony_food;
        context->metrics[production_rules::METRIC_COLONY_FOOD_DEMAND] = (food2_demand + 1) / 2;
        context->metrics[production_rules::METRIC_PRODUCTION] = colony->production[ECON_INDUSTRY];
        context->metrics[production_rules::METRIC_INDUSTRY] = Industry_Worth((s_colony*)colony);
        context->metrics[production_rules::METRIC_POLLUTION] = colony->pollution;
        context->metrics[production_rules::METRIC_FOOD_PER_FARMER] = colony->food2_per_farmer;
        int32_t preferred_gravity = PLANET_GRAVITY_NORMAL_G;
        if (player->traits[TRAIT_LOW_G_WORLD] != 0) preferred_gravity = PLANET_GRAVITY_LOW_G;
        if (player->traits[TRAIT_HEAVY_G_WORLD] != 0) preferred_gravity = PLANET_GRAVITY_HEAVY_G;
        int32_t gravity_mismatch = (int32_t)planet->gravity_class - preferred_gravity;
        context->metrics[production_rules::METRIC_GRAVITY_MISMATCH] =
            gravity_mismatch < 0 ? -gravity_mismatch : gravity_mismatch;
        context->metrics[production_rules::METRIC_CLIMATE] = planet->climate;
        context->metrics[production_rules::METRIC_MINERAL_CLASS] = planet->mineral_class;
        context->metrics[production_rules::METRIC_INCOMING_HUMAN_SHIPS] =
            (*AIDATA::enroute_human_ships)[planet->star_index][9];
        context->metrics[production_rules::METRIC_ENEMY_PRESENCE] = (uint8_t)star_info->enemy_count;
        context->metrics[production_rules::METRIC_NEUTRAL_PRESENCE] = (uint8_t)star_info->neutral_count;
        context->metrics[production_rules::METRIC_ALLIED_PRESENCE] = (uint8_t)star_info->alliance_count;
        context->metrics[production_rules::METRIC_OUT_OF_RANGE_PRESENCE] = (uint8_t)star_info->out_of_range_count;
        context->metrics[production_rules::METRIC_EMPIRE_FOOD_SURPLUS] = player->surplus_food;
        context->metrics[production_rules::METRIC_EMPIRE_TREASURY] = player->bc;
        context->metrics[production_rules::METRIC_EMPIRE_BC_SURPLUS] = player->surplus_bc;
        context->metrics[production_rules::METRIC_COMMAND_POINT_DEFICIT] =
            player->command_points_used > player->command_points ? player->command_points_used - player->command_points : 0;
        context->metrics[production_rules::METRIC_WAR_LEVEL] = war_level;
        context->metrics[production_rules::METRIC_CURRENT_STRENGTH] =
            player_total_strength_vs_ships[owner][0];
        context->metrics[production_rules::METRIC_COMBAT_NEED] =
            player_total_strength_vs_ships[owner][2];
        context->metrics[production_rules::METRIC_DESIRED_STRENGTH] =
            context->metrics[production_rules::METRIC_CURRENT_STRENGTH] +
            context->metrics[production_rules::METRIC_COMBAT_NEED];
        context->metrics[production_rules::METRIC_FREIGHTER_NEED] =
            player_n_freighter_groups_wanted[owner];
        context->metrics[production_rules::METRIC_SPY_NEED] = player_n_spies[owner][1];
        context->metrics[production_rules::METRIC_TRANSPORT_NEED] = player_n_transports[owner][1];
        context->metrics[production_rules::METRIC_EXISTING_COLONY_SHIPS] = player_n_colony_ships[owner][0];

        int32_t treasury_score = 0;
        if (player->bc >= 1500) {
            int16_t surplus_bc = player->surplus_bc;
            treasury_score = special::isqrt((int16_t)((surplus_bc + ((surplus_bc >> 15) & 0x3f)) >> 6));
            if (treasury_score > 10) treasury_score = 10;
        }
        context->metrics[production_rules::METRIC_TREASURY_SCORE] = treasury_score;
        context->metrics[production_rules::METRIC_LOCAL_THREAT] =
            (uint8_t)ai_colony->threatened * 5 + (uint8_t)star_info->alliance_count * 6 +
            (uint8_t)star_info->out_of_range_count * 2 + (uint8_t)star_info->neutral_count * 3 +
            (uint8_t)star_info->enemy_count;

        int8_t government_family = player->traits[TRAIT_CURRENT_GOVERNMENT] / 2;
        bool industrial_focus = colony->buildings[BUILDING_AUTOMATED_FACTORY] == 0 &&
            player->tech_applications[TECH_APP_AUTOMATED_FACTORIES] == TECH_RESEARCH_STATUS_RESEARCHED &&
            planet->mineral_class <= MINERAL_RESOURCE_ABUNDANT;
        bool military_focus = government_family >= 0 && government_family <= 1 &&
            ((colony->buildings[BUILDING_MARINE_BARRACKS] == 0 &&
                 player->tech_applications[TECH_APP_MARINE_BARRACKS] == TECH_RESEARCH_STATUS_RESEARCHED) ||
                (colony->buildings[BUILDING_ARMOR_BARRACKS] == 0 &&
                    player->tech_applications[TECH_APP_ARMOR_BARRACKS] == TECH_RESEARCH_STATUS_RESEARCHED));

        context->flags[production_rules::FLAG_CONQUERED] = Colony_Is_Conquered((s_colony*)colony);
        context->flags[production_rules::FLAG_BUILDING_FOCUS] = industrial_focus || military_focus;
        context->flags[production_rules::FLAG_RESEARCH_ENABLED] = player->do_not_research == 0;
        context->flags[production_rules::FLAG_THREATENED] = ai_colony->threatened != 0;
        context->flags[production_rules::FLAG_AT_WAR] = war_level > 0;
        context->flags[production_rules::FLAG_IS_HOMEWORLD] = player->home_planet_id == colony->planet;
        context->flags[production_rules::FLAG_EMPIRE_FOOD_DEFICIT] = player->surplus_food < 0;
        context->flags[production_rules::FLAG_EMPIRE_LOW_MONEY] =
            player->bc + player->surplus_bc < player->total_maintenance;
        context->flags[production_rules::FLAG_OBJECTIVE_ECOLOGIST] =
            player->objectives == PLAYER_OBJECTIVE_ECOLOGIST;
        context->flags[production_rules::FLAG_OBJECTIVE_INDUSTRIALIST] =
            player->objectives == PLAYER_OBJECTIVE_INDUSTRIALIST;
        context->flags[production_rules::FLAG_OBJECTIVE_MILITARIST] =
            player->objectives == PLAYER_OBJECTIVE_MILITARIST;
        context->flags[production_rules::FLAG_OBJECTIVE_TECHNOLOGIST] =
            player->objectives == PLAYER_OBJECTIVE_TECHNOLOGIST;
        context->flags[production_rules::FLAG_FOOD_BUILDINGS_APPLICABLE] =
            ai_colony->food_buildings_applicable != 0;
        context->flags[production_rules::FLAG_POLLUTION_CONTROLS_APPLICABLE] =
            ai_colony->pollution_controls_applicable != 0;
        context->flags[production_rules::FLAG_POPULATION_OR_WEALTH_GATE] = population > 1 || treasury_score != 0;
        context->flags[production_rules::FLAG_MISSING_ARMOR_BARRACKS_FOR_GOVERNMENT] =
            colony->buildings[BUILDING_ARMOR_BARRACKS] == 0 && government_family >= 0 && government_family <= 1;
        context->flags[production_rules::FLAG_MISSING_MARINE_BARRACKS_FOR_GOVERNMENT] =
            colony->buildings[BUILDING_MARINE_BARRACKS] == 0 && government_family >= 0 && government_family <= 1;
        context->flags[production_rules::FLAG_COMBAT_SHIP_AVAILABLE] =
            player->ship_range != 0 && player->ship_speed != 0;
        context->flags[production_rules::FLAG_TRANSPORT_DESIGN_AVAILABLE] =
            context->flags[production_rules::FLAG_COMBAT_SHIP_AVAILABLE] &&
            player->tech_applications[TECH_APP_TRANSPORT] == TECH_RESEARCH_STATUS_RESEARCHED;
        context->government_family = government_family;
        context->climate = planet->climate;
        context->mineral_class = planet->mineral_class;
    }

    int16_t __cdecl Colony_Building_Score(s_colony* colony, int16_t building_id) {
        uint8_t owner_u8 = (uint8_t)colony->owner;
        int32_t owner = owner_u8;
        s_player* player = &MOX::player[owner];
        int16_t colony_idx = (int16_t)(colony - MOX::colony);
        s_ai_colony_info* ai_colony = &(*AIDATA::ai_colony_info)[colony_idx];

        int32_t ecologist_objective_bonus = (player->objectives == PLAYER_OBJECTIVE_ECOLOGIST);
        int32_t industrialist_objective_bonus = (player->objectives == PLAYER_OBJECTIVE_INDUSTRIALIST);
        int32_t militarist_objective_bonus = (player->objectives == PLAYER_OBJECTIVE_MILITARIST);
        int32_t technologist_objective_bonus = (player->objectives == PLAYER_OBJECTIVE_TECHNOLOGIST);

        int32_t bc_score = 0;
        if (player->bc >= 1500) {
            int16_t surplus_bc = player->surplus_bc;
            bc_score = (int32_t)special::isqrt((int16_t)((surplus_bc + ((surplus_bc >> 15) & 0x3f)) >> 6));
        }
        if (bc_score > 10) bc_score = 10;

        s_planet_data* planet = &MOX::planet[colony->planet];
        int16_t star_idx = planet->star_index;
        s_ai_star_info* ai_star = &(*AIDATA::ai_star_info)[star_idx];
        s_ai_star_player_block* star_player_info = &ai_star->per_player[owner];

        uint8_t pollution_controls_applicable = (uint8_t)ai_colony->pollution_controls_applicable;
        uint8_t enemy_count = (uint8_t)star_player_info->enemy_count;
        uint8_t neutral_count = (uint8_t)star_player_info->neutral_count;
        uint8_t alliance_count = (uint8_t)star_player_info->alliance_count;
        uint8_t out_of_range_count = (uint8_t)star_player_info->out_of_range_count;
        uint8_t threatened = (uint8_t)ai_colony->threatened;
        uint8_t food_buildings_applicable = (uint8_t)ai_colony->food_buildings_applicable;
        int8_t climate = planet->climate;
        int8_t gravity = planet->gravity_class;

        bool is_industrial_focus = false;
        if (colony->buildings[BUILDING_AUTOMATED_FACTORY] == 0 && player->tech_applications[TECH_APP_AUTOMATED_FACTORIES] == TECH_RESEARCH_STATUS_RESEARCHED && planet->mineral_class <= MINERAL_RESOURCE_ABUNDANT) {
            is_industrial_focus = true;
        }

        bool is_military_focus = false;
        int8_t government_family = player->traits[TRAIT_CURRENT_GOVERNMENT] / 2;
        if (government_family >= 0 && government_family <= 1) {
            if ((colony->buildings[BUILDING_MARINE_BARRACKS] == 0 && player->tech_applications[TECH_APP_MARINE_BARRACKS] == TECH_RESEARCH_STATUS_RESEARCHED) ||
                (colony->buildings[BUILDING_ARMOR_BARRACKS] == 0 && player->tech_applications[TECH_APP_ARMOR_BARRACKS] == TECH_RESEARCH_STATUS_RESEARCHED)) {
                is_military_focus = true;
            }
        }

        bool building_focus = is_industrial_focus || is_military_focus;
        int32_t score = 0;

        switch (building_id) {
            case BUILDING_ARMOR_BARRACKS: {
                int32_t threatened_score = alliance_count * 3 + threatened * 2 +
                    out_of_range_count + neutral_count + enemy_count;
                if (colony->n_pops > 2 || bc_score != 0) {
                    if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                }
                if (government_family >= 0 && government_family <= 1 && colony->buildings[BUILDING_MARINE_BARRACKS] == 0) {
                    score += 6;
                }
                if (Colony_Is_Conquered(colony)) score += 1;
                bc_score /= 2;
                score += bc_score;
            } break;

            case BUILDING_ARTEMIS_SYSTEM_NET:
                if (building_focus && threatened == 0) break;
                if (((MOX::star[star_idx].has_warp_field_interdictor >> (owner_u8 & 0x1F)) & 1) == 0) {
                    int threatened_score = alliance_count * 3 + threatened * 10 +
                        out_of_range_count + neutral_count + enemy_count;
                    if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                }
                bc_score /= 2;
                score += bc_score;
                break;

            case BUILDING_ASTRO_UNIVERSITY:
                score = 5;
                break;

            case BUILDING_ATMOSPHERE_RENEWER:
                if (pollution_controls_applicable != 0 && colony->pollution > 5) {
                    score = ecologist_objective_bonus;
                    if (colony->pollution >= 11) {
                        score = ecologist_objective_bonus + (int32_t)special::isqrt((uint32_t)colony->pollution);
                    }
                }
                break;

            case BUILDING_AUTOLAB:
                if (!building_focus && player->do_not_research == 0) {
                    score = technologist_objective_bonus * 4 + 11;
                }
                break;

            case BUILDING_AUTOMATED_FACTORY:
                score = (uint8_t)colony->n_pops + 13 + industrialist_objective_bonus * 2;
                break;

            case BUILDING_BATTLESTATION: {
                if (building_focus && threatened == 0) break;
                int32_t base_threat = (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 3 + threatened * 10;
                if (player->command_points <= player->command_points_used) {
                    base_threat += (player->command_points_used - player->command_points) + 1;
                }
                if (base_threat != 0) score = base_threat + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_CAPITOL:
                if (government_family != 3 && player->home_planet_id == colony->planet) {
                    score = 100;
                }
                break;

            case BUILDING_CLONING_CENTER:
                if (!building_focus) {
                    if (player->traits[TRAIT_POP_GROWTH_BONUS] < 0) score = ecologist_objective_bonus;
                    bc_score /= 2;
                    score += bc_score;
                }
                break;

            case BUILDING_DEEP_CORE_MINE:
                score = (uint8_t)colony->n_pops + 12 + industrialist_objective_bonus * 4;
                break;

            case BUILDING_CORE_WASTE_DUMP:
                if (!building_focus && pollution_controls_applicable != 0 && colony->pollution > 5) {
                    score = ecologist_objective_bonus;
                    if (colony->pollution >= 11) {
                        score = ecologist_objective_bonus + (int32_t)special::isqrt((uint32_t)colony->pollution);
                    }
                }
                break;

            case BUILDING_BIOSPHERES:
                if (!building_focus) score = ecologist_objective_bonus + 18;
                break;

            case BUILDING_FOOD_REPLICATORS:
                if (food_buildings_applicable != 0) {
                    int32_t base_score = 8;
                    if (player->surplus_food >= 0) {
                        base_score = 4;
                    }
                    score = base_score + ecologist_objective_bonus;
                }
                break;

            case BUILDING_GAIA_TRANSFORMATION:
                score = bc_score + ecologist_objective_bonus;
                break;

            case BUILDING_GALACTIC_CYBERNET:
                if (!building_focus && player->do_not_research == 0) {
                    score = technologist_objective_bonus * 4 + 11;
                }
                break;

            case BUILDING_HOLO_SIMULATOR:
                if (government_family != 3 && (colony->n_pops > 2 || (bc_score != 0 && colony->n_pops > 1))) {
                    score = 10;
                }
                break;

            case BUILDING_HYDROPONIC_FARM:
                if (food_buildings_applicable != 0) {
                    int32_t base_val = 6;
                    if (colony->food2_per_farmer == 0)
                        base_val = 12;
                    else if (colony->food2_per_farmer == 1)
                        base_val = 11;
                    else if (colony->food2_per_farmer == 2)
                        base_val = 10;

                    if (player->surplus_food < 0) base_val -= (int32_t)player->surplus_food;
                    score = base_val + ecologist_objective_bonus * 4;
                }
                break;

            case BUILDING_MARINE_BARRACKS: {
                int32_t threatened_score = threatened * 5 + (alliance_count * 3 + out_of_range_count) * 2 + neutral_count * 3 + enemy_count;
                if (colony->n_pops > 1 || bc_score != 0) {
                    if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                }
                if (colony->buildings[BUILDING_ARMOR_BARRACKS] == 0 && government_family >= 0 && government_family <= 1) {
                    score += 12;
                }
                if (Colony_Is_Conquered(colony)) score += 3;
                score += bc_score;
            } break;

            case BUILDING_BARRIER_SHIELD:
            case BUILDING_FLUX_SHIELD: {
                if (building_focus && threatened == 0) break;
                int32_t threatened_score = enemy_count + (threatened * 5 + (alliance_count * 3 + neutral_count) * 2 + out_of_range_count) * 2;
                if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                if (climate == PLANET_CLIMATE_RADIATED) score += ecologist_objective_bonus * 2;
                score += bc_score;
            } break;

            case BUILDING_GRAVITY_GENERATOR: {
                if (player->traits[TRAIT_HEAVY_G_WORLD] != 0) {
                    if (gravity == PLANET_GRAVITY_LOW_G) score = ecologist_objective_bonus + 3;
                } else if (player->traits[TRAIT_LOW_G_WORLD] != 0) {
                    if (gravity == PLANET_GRAVITY_NORMAL_G)
                        score = ecologist_objective_bonus + 3;
                    else if (gravity == PLANET_GRAVITY_HEAVY_G)
                        score = ecologist_objective_bonus + 6;
                } else {
                    if (gravity == PLANET_GRAVITY_LOW_G)
                        score = ecologist_objective_bonus + 3;
                    else if (gravity == PLANET_GRAVITY_HEAVY_G)
                        score = ecologist_objective_bonus + 6;
                }
            } break;

            case BUILDING_MISSILE_BASE: {
                if (building_focus && threatened == 0) break;
                int32_t base_threat = threatened * 10 + (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 4;
                if (base_threat != 0) score = base_threat + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_GROUND_BATTERIES: {
                if (building_focus && threatened == 0) break;
                int32_t base_threat = threatened * 10 + (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 4;
                if (base_threat != 0) score = base_threat + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_RADIATION_SHIELD: {
                if (building_focus && threatened == 0) break;
                int32_t threatened_score = threatened * 10 + (alliance_count * 3 + neutral_count * 2 + out_of_range_count + enemy_count) * 4;
                if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                if (climate == PLANET_CLIMATE_RADIATED) score += ecologist_objective_bonus * 2;
                score += bc_score;
            } break;

            case BUILDING_STOCK_EXCHANGE:
                if (!building_focus && colony->n_pops > 3) {
                    score = (ai_colony->pop_limit + (uint8_t)colony->n_pops + industrialist_objective_bonus) / 3;
                }
                break;

            case BUILDING_SUPERCOMPUTER:
                if (!building_focus && player->do_not_research == 0) {
                    score = technologist_objective_bonus * 3 + 8;
                }
                break;

            case BUILDING_PLEASURE_DOME:
                if (government_family != 3 && (colony->n_pops > 2 || (bc_score != 0 && colony->n_pops > 1))) {
                    score = 16;
                }
                break;

            case BUILDING_POLLUTION_PROCESSOR:
                if (pollution_controls_applicable != 0 && colony->pollution > 5) {
                    score = ecologist_objective_bonus;
                    if (colony->pollution >= 11) {
                        score = ecologist_objective_bonus + (int32_t)special::isqrt((uint32_t)colony->pollution);
                    }
                }
                break;

            case BUILDING_RECYCLOTRON:
                score = (ai_colony->pop_limit + (uint8_t)colony->n_pops * 2) / 3 + (pollution_controls_applicable + industrialist_objective_bonus + ecologist_objective_bonus) * 2;
                break;

            case BUILDING_ROBOTIC_FACTORY:
                score = industrialist_objective_bonus * 2 + 12;
                break;

            case BUILDING_RESEARCH_LAB:
                if (!building_focus && player->do_not_research == 0) {
                    score = technologist_objective_bonus * 2 + 5;
                }
                break;

            case BUILDING_ROBO_MINER_PLANT:
                score = (uint8_t)colony->n_pops + 5 + industrialist_objective_bonus * 2;
                break;

            case BUILDING_SOIL_ENRICHMENT:
                if (!building_focus && food_buildings_applicable != 0 && colony->food2_per_farmer != 0) {
                    int32_t base_val = (player->surplus_food >= 0) ? 3 : 5;
                    score = base_val + ecologist_objective_bonus * 2;
                }
                break;

            case BUILDING_SPACE_ACADEMY:
                if (!building_focus && (colony->production[1] > 16 || colony->n_pops > 4 || bc_score != 0)) {
                    score = (int32_t)special::isqrt((int32_t)colony->production[1] - 15);
                }
                break;

            case BUILDING_SPACEPORT:
                if (!building_focus && colony->n_pops > 3) {
                    score = (ai_colony->pop_limit + (uint8_t)colony->n_pops + industrialist_objective_bonus) / 3;
                }
                break;

            case BUILDING_STAR_BASE: {
                if (building_focus && threatened == 0) break;
                int32_t base_threat = out_of_range_count * 3 + (enemy_count + (neutral_count + alliance_count * 2) * 2) * 4 + threatened * 10;
                if (player->command_points <= player->command_points_used) {
                    base_threat += (player->command_points_used - player->command_points) + 1;
                }
                if (base_threat != 0) score = base_threat + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_STAR_FORTRESS: {
                if (building_focus && threatened == 0) break;
                int32_t base_threat = (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 3 + threatened * 10;
                if (player->command_points <= player->command_points_used) {
                    base_threat += (player->command_points_used - player->command_points) + 1;
                }
                if (base_threat != 0) score = base_threat + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_STELLAR_CONVERTER: {
                if (building_focus && threatened == 0) break;
                int32_t threatened_score = threatened * 10 + (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 4;
                if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                score += bc_score;
            } break;

            case BUILDING_SUBTERRANEAN_FARMS:
                if (!building_focus && food_buildings_applicable != 0) {
                    int32_t base_val = 7;
                    if (colony->food2_per_farmer == 0)
                        base_val = 13;
                    else if (colony->food2_per_farmer == 1)
                        base_val = 12;
                    else if (colony->food2_per_farmer == 2)
                        base_val = 10;

                    if (player->surplus_food < 0) base_val -= (int32_t)player->surplus_food;
                    score = base_val + ecologist_objective_bonus * 3;
                }
                break;

            case BUILDING_TERRAFORMING:
                if (!building_focus) {
                    int32_t base_climate_score = 0;
                    bool valid_climate = true;
                    bool is_aquatic = (player->traits[TRAIT_AQUATIC] != 0);
                    switch (climate) {
                        case PLANET_CLIMATE_BARREN: base_climate_score = 2; break;
                        case PLANET_CLIMATE_DESERT: base_climate_score = 1; break;
                        case PLANET_CLIMATE_TUNDRA:
                            if (is_aquatic) {
                                base_climate_score = 1;
                            }
                            break;
                        case PLANET_CLIMATE_OCEAN:
                            if (!is_aquatic) {
                                base_climate_score = 4;
                            }
                            break;
                        case PLANET_CLIMATE_SWAMP:
                            if (!is_aquatic) {
                                base_climate_score = 6;
                            }
                            break;
                        case PLANET_CLIMATE_ARID: base_climate_score = 1; break;
                        default: valid_climate = false; break;
                    }
                    score = bc_score;
                    if (valid_climate) score += base_climate_score + ecologist_objective_bonus * 3;
                }
                break;

            case BUILDING_WARP_INTERDICTOR: {
                if (building_focus && threatened == 0) break;
                if (((MOX::star[star_idx].has_warp_field_interdictor >> (owner_u8 & 0x1F)) & 1) == 0) {
                    int32_t threatened_score = threatened * 5 + (enemy_count + alliance_count * 2) * 2 + neutral_count * 3 + out_of_range_count;
                    if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                }
                bc_score /= 2;
                score += bc_score;
            } break;

            case BUILDING_WEATHER_CONTROLLER:
                if (!building_focus && food_buildings_applicable != 0 && colony->food2_per_farmer != 0) {
                    int base_val = (player->surplus_food >= 0) ? 5 : 10;
                    score = base_val + ecologist_objective_bonus * 2;
                }
                break;

            case BUILDING_FIGHTER_GARRISON: {
                if (building_focus && threatened == 0) break;
                int32_t threatened_score = threatened * 10 + (out_of_range_count + (neutral_count + alliance_count * 2) * 2 + enemy_count) * 4;
                if (threatened_score != 0) score = threatened_score + militarist_objective_bonus;
                score += bc_score;
            } break;
        }

        if (score > 1000) score = 1000;
        if (score <= 0) return 0;
        return (int16_t)score;
    }

    bool __cdecl Colony_Is_Conquered(s_colony* colony) {
        for (uint8_t i = 0; i < (uint8_t)colony->n_pops; ++i) {
            uint32_t pop_data = colony->pop[i];
            uint32_t pop_owner = POP::Get_Race(pop_data);

            if (pop_owner < MAX_PLAYERS && (int8_t)pop_owner != colony->owner) {
                if (MOX::player[pop_owner].eliminated == 0) {
                    int8_t treaty_status = MOX::player[pop_owner].treaty[(int32_t)colony->owner];

                    if (treaty_status >= 4 && treaty_status <= 6) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void __cdecl Assign_Colony_New_Building(s_colony* colony) {
        uint16_t weights[52];
        int32_t evaluated_weights[AUTOBUILD_CANDIDATE_COUNT];
        production_rules::s_context context;

        memset(weights, 0, sizeof(weights));
        memset(evaluated_weights, 0, sizeof(evaluated_weights));

        Find_Buildable_Buildings(colony, weights);
        Build_Production_Context(colony, -1, &context);
        context.flags[production_rules::FLAG_ORBITAL_DEFENSE_APPLICABLE] =
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_STAR_BASE) == 1 ||
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_BATTLESTATION) == 1 ||
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_STAR_FORTRESS) == 1;
        context.flags[production_rules::FLAG_PLANETARY_DEFENSE_APPLICABLE] =
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_MISSILE_BASE) == 1 ||
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_GROUND_BATTERIES) == 1 ||
            COLCALC::Colony_Can_Build_Product(colony, BUILDING_FIGHTER_GARRISON) == 1;

        for (int16_t i = BUILDING_NO_BUILDING; i < BUILDING_COUNT; ++i) {
            production_rules::s_candidate* candidate = &autobuild_rules.candidates[i];
            if (weights[i] > 0 && candidate->enabled &&
                production_rules::Evaluate_Eligibility(&context, &candidate->eligibility)) {
                evaluated_weights[i] = production_rules::Evaluate_Weight(
                    &context, &candidate->weight, Colony_Building_Score(colony, i));
            }
        }

        for (int32_t i = BUILDING_COUNT; i < AUTOBUILD_CANDIDATE_COUNT; ++i) {
            production_rules::s_candidate* candidate = &autobuild_rules.candidates[i];
            int16_t native_product = i == BUILDING_COUNT ? COLONY_PRODUCTION_HOUSING : COLONY_PRODUCTION_TRADE_GOODS;
            if (COLCALC::Colony_Can_Build_Product(colony, native_product) == 1 &&
                candidate->enabled && production_rules::Evaluate_Eligibility(&context, &candidate->eligibility)) {
                evaluated_weights[i] = production_rules::Evaluate_Weight(&context, &candidate->weight, 0);
            }
        }

        int32_t difficulty = MOX::settings.game_difficulty;
        if (MOX::player[(int32_t)colony->owner].objectives == PLAYER_OBJECTIVE_HUMAN) {
            difficulty = 5;
        }

        production_rules::Apply_Cutoff(&autobuild_rules.cutoff, difficulty, autobuild_rules.candidates, evaluated_weights, AUTOBUILD_CANDIDATE_COUNT);
        for (int32_t i = 0; i < AUTOBUILD_CANDIDATE_COUNT; ++i) {
            weights[i] = (uint16_t)evaluated_weights[i];
        }

        int16_t colony_idx = (int16_t)(colony - MOX::colony);
        int16_t choice = RUSS::Get_Weighted_Choice_Short(weights, BUILDING_COUNT + 2);

        if (choice < BUILDING_COUNT) {
            COLBLDG::Delete_Prod_From_Colony_Queue(colony_idx, choice);
            colony->producing[0] = choice;
        } else if (choice == BUILDING_COUNT) {
            colony->producing[0] = COLONY_PRODUCTION_HOUSING;
        } else if (choice == BUILDING_COUNT + 1) {
            colony->producing[0] = COLONY_PRODUCTION_TRADE_GOODS;
        }
    }

    void __cdecl Force_Produce(int16_t* id_list, int16_t list_index, int32_t* list_count, int16_t production_id) {
        int16_t colony_idx = id_list[list_index];
        MOX::colony[colony_idx].producing[0] = production_id;
        *list_count -= 1;
        int32_t items_to_move = *list_count - list_index;
        memmove(&id_list[list_index], &id_list[list_index + 1], items_to_move * sizeof(int16_t));
    }

    int16_t __cdecl Industry_Worth(s_colony* colony) {
        return (int16_t)(((uint8_t)colony->n_pops >> 3) - (uint8_t)colony->maintenance[1] + colony->production[1]);
    }

    char* __cdecl Product_Name(int16_t product_id) {
        if (product_id == COLONY_PRODUCTION_AI_TRADE_GOODS) {
            return (char*)"AI trade goods";
        }

        return COLBLDG::Selection_Name(product_id);
    }

    void __cdecl Player_Colony_Autobuild(s_colony* colony) {
        AIBUILD::Assign_Colony_New_Building(colony);
    }
} // namespace AIBUILD
