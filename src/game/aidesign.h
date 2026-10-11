#pragma once
#include <cstdint>

namespace aidesign {
    extern bool allow_stealth_field;
    extern uint8_t ship_design_bhg_chance;
    extern uint8_t ship_design_stellar_chance;

    enum e_ship_design_allocation_profile {
        SHIP_DESIGN_ALLOCATION_STANDARD_BEAM_MISSILE_X2,
        SHIP_DESIGN_ALLOCATION_HYBRID_BEAM_CARRIER,
        SHIP_DESIGN_ALLOCATION_HYBRID_BEAM_MISSILE_X5,
        SHIP_DESIGN_ALLOCATION_STANDARD_MISSILE_X10,
        SHIP_DESIGN_ALLOCATION_STANDARD_CARRIER,
        SHIP_DESIGN_ALLOCATION_SPECIAL_DEVICE,
        SHIP_DESIGN_ALLOCATION_SPECIAL_WEAPON,
        SHIP_DESIGN_ALLOCATION_SPECIAL_WEAPON_2,
        SHIP_DESIGN_ALLOCATION_PROFILE_COUNT,
    };

    struct s_ship_design_allocations {
        s_aidesign_ship_design profiles
            [SHIP_DESIGN_ALLOCATION_PROFILE_COUNT][SHIP_SIZE_COUNT];
    };

    struct s_ship_defense_priority {
        s_spcl human_non_beams[19];
        s_spcl human_beams[19];
    };

    struct s_ship_special_priority {
        s_spcl bio_weapon[5];
        s_spcl capture[5];
        s_spcl cloaking[3];
        s_spcl beam_defense[4];
        s_spcl missile_defense[8];
        s_spcl armor[3];
        s_spcl shield[3];
        s_spcl beam_offense[7];
        s_spcl missile_offense[2];
        s_spcl special_weapon[5];
    };

    struct s_ship_design_config {
        s_ship_special_priority special_priority;
        s_spcl special_device_priority[12];
        s_ship_defense_priority defense_priority;
    };

    extern int16_t min_weapon_count_table[8];

    extern s_ship_design_config ship_design;

    extern const s_ship_design_allocations default_ship_design_allocations;
    extern s_ship_design_allocations ship_design_allocations;

    /**
     * Adds legal learned specials or special weapons to a ship design until the available space is exhausted.
     * @param list Special-item table to scan.
     * @param count Number of entries in the table.
     * @param player_idx Player whose unlocked tech is used.
     * @param design Ship design being populated.
     * @param space Available space budget for additions.
     * @param weapon_index_ptr In/out weapon slot index for added weapon entries.
     */
    int32_t __cdecl Add_Specials_To_Design(struct s_spcl* list, int16_t count, int16_t player_idx, struct s_ship_design* design, int32_t space, int16_t* weapon_index_ptr);

    /**
     * Adds specials that match the player's current ship-special theme.
     * @param player_idx Player whose theme is applied.
     * @param design Ship design being populated.
     * @param space Available space budget for additions.
     * @param weapon_index_ptr In/out weapon slot index for added weapon entries.
     */
    int32_t __cdecl Add_Theme_Special_To_Design(int16_t player_idx, struct s_ship_design* design, int32_t space, int16_t* weapon_index_ptr);

    /**
     * Chooses an AI ship-class template for the player's next auto-designed hull.
     * @param player_idx Player requesting the design.
     */
    int16_t __cdecl Get_Ship_Class(int16_t player_idx);

    /**
     * Reports whether the player has access to any fighter-bay weapon.
     * @param player_idx Player to inspect.
     */
    int16_t __cdecl Fighters_Available(int16_t player_idx);

    /**
     * Reports whether the player has access to any special offensive weapon.
     * @param player_idx Player to inspect.
     */
    int16_t __cdecl Special_Weapons_Available(int16_t player_idx);

    /**
     * Reports whether the player has access to any themed special device entry.
     * @param player_idx Player to inspect.
     */
    int16_t __cdecl Special_Devices_Available(int16_t player_idx);

    /**
     * Returns the best AI-group value the player has reached for a technology category.
     * @param player_idx Player whose tech is inspected.
     * @param tech_type AI-group category to compare.
     */
    int16_t __cdecl Get_Competition_Player_Tech_Values(int16_t player_idx, int16_t tech_type);

    /**
     * Compares the leading non-eliminated human player's beam tech against competing weapon tech.
     */
    bool __cdecl Human_Player_Best_Weapons_Are_Beams();

    /**
     * Fills weapon slots with the best available fighter-bay weapon for the allotted space.
     * @param player_idx Player whose tech is used.
     * @param design Ship design being populated.
     * @param weapon_index_ptr In/out weapon slot index for added entries.
     * @param space Available space budget for fighters.
     */
    int32_t __cdecl Add_Fighters_To_Design(int16_t player_idx, struct s_ship_design* design, int16_t* weapon_index_ptr, int32_t space);

    /**
     * Adds one class of beam weapon using the selected beam modifications for the hull.
     * @param player_idx Player whose tech is used.
     * @param design Ship design being populated.
     * @param weapon_index_ptr In/out weapon slot index for added entries.
     * @param space Available space budget for beams.
     * @param weapon_class Beam role selector for point defense, standard, or heavy beams.
     */
    int32_t __cdecl Add_Beam_Weapon_To_Design(int16_t player_idx, struct s_ship_design* design, int16_t* weapon_index_ptr, int32_t space, int16_t weapon_class);

    /**
     * Chooses up to three beam weapon configurations and firing arcs for the hull.
     * @param player_idx Player whose tech is used.
     * @param hull_size Hull size being configured.
     * @param type0 Out point-defense beam weapon id.
     * @param spec0 Out point-defense beam modifiers.
     * @param arc0 Out point-defense firing arc.
     * @param type1 Out main beam weapon id.
     * @param spec1 Out main beam modifiers.
     * @param arc1 Out main beam firing arc.
     * @param type2 Out heavy beam weapon id.
     * @param spec2 Out heavy beam modifiers.
     * @param arc2 Out heavy beam firing arc.
     */
    void __cdecl Determine_Beam_Weapon_Modifications(int16_t player_idx, int16_t hull_size, int16_t* type0, int16_t* spec0, int16_t* arc0, int16_t* type1, int16_t* spec1, int16_t* arc1, int16_t* type2, int16_t* spec2, int16_t* arc2);

    /**
     * Chooses a missile weapon, its modifiers, and an ammo count that fit the current budget.
     * @param player_idx Player whose tech is used.
     * @param out_weapon_id Out selected missile weapon id.
     * @param out_weapon_mods Out selected missile modifiers.
     * @param weapon_count_table_idx Hull-size index for minimum missile counts.
     * @param available_space Space budget for the missile weapon.
     * @param out_weapon_count In/out ammo count for the selected missile.
     */
    void __cdecl Determine_Missile_Weapon_Modifications(
        int16_t player_idx,
        uint16_t* out_weapon_id,
        uint16_t* out_weapon_mods,
        int16_t weapon_count_table_idx,
        int32_t available_space,
        int16_t* out_weapon_count);

    /**
     * Adds a missile weapon loadout to the current design.
     * @param player_idx Player whose tech is used.
     * @param design Ship design being populated.
     * @param weapon_index_ptr In/out weapon slot index for added entries.
     * @param space Available space budget for missiles.
     * @param ammo Requested ammo setting before budget adjustments.
     */
    int32_t __cdecl Add_Missile_Weapon_To_Design(int16_t player_idx, struct s_ship_design* design, int16_t* weapon_index_ptr, int32_t space, int16_t ammo);

    /**
     * Chooses the bomb weapon that best matches the player's tech and the current opposition.
     * @param player_idx Player whose tech is used.
     * @param bomb_type Out selected bomb weapon id.
     */
    void __cdecl Determine_Bomb_Weapon(int16_t player_idx, int16_t* bomb_type);

    /**
     * Adds bomb weapons to the current design until the allotted space is consumed.
     * @param player_idx Player whose tech is used.
     * @param design Ship design being populated.
     * @param weapon_slot_idx In/out weapon slot index for added entries.
     * @param space_available Available space budget for bombs.
     */
    int32_t __cdecl Add_Bomb_Weapon_To_Design(int16_t player_idx, struct s_ship_design* design, int16_t* weapon_slot_idx, int32_t space_available);

    /**
     * Builds an automated ship design for the selected player and hull.
     * @param player_idx Player receiving the design.
     * @param design Output ship design structure.
     * @param hull_idx Hull size/type index to build.
     */
    void __cdecl Auto_Design_Ship(int16_t player_idx, s_ship_design* design, int16_t hull_idx);

    /**
     * Checks whether a special can be added without violating exclusion rules on the design.
     * @param design Ship design being inspected.
     * @param tech_id Special id to validate.
     */
    int16_t __cdecl Special_Legal_For_Design(s_ship_design* design, int16_t tech_id);
} // namespace aidesign