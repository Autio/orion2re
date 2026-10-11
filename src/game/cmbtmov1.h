#pragma once
#include <cstdint>

struct s_net_tactical_combat_message;

namespace CMBTMOV1 {
    extern uint8_t round_teleporter;
    extern uint8_t sub_space_teleporter_range;

    /** Returns whether a destination is within configured teleporter reach. */
    bool __cdecl Teleporter_Destination_In_Range(
        int16_t ship_idx, int16_t target_x_px, int16_t target_y_px);

    /**
     * @brief Returns whether the combat ship is currently held in stasis.
     * @param ship_idx Combat ship index to inspect.
     */
    int8_t __cdecl Ship_Is_In_Stasis(int16_t ship_idx);

    /**
     * @brief Rotates a ship toward the requested facing while updating incoming missiles.
     * @param ship_idx Combat ship index to rotate.
     * @param target_facing Desired facing direction.
     * @param screen_x Current on-screen X position for drawing.
     * @param screen_y Current on-screen Y position for drawing.
     */
    void __cdecl Rotate_Ship(int16_t ship_idx, int16_t target_facing, int16_t screen_x, int16_t screen_y);

    /**
     * @brief Waits for the remote tactical-combat participant to finish their action.
     * @param show_wait_message Nonzero to draw the waiting prompt while polling.
     */
    void __cdecl Wait_Until_Net_Opponent_Finished(int16_t show_wait_message);

    /**
     * @brief Returns the movement cost to reach a destination grid square.
     * @param ship_idx Combat ship index to evaluate.
     * @param target_x Destination grid X coordinate.
     * @param target_y Destination grid Y coordinate.
     */
    int16_t __cdecl Move_Cost(int16_t ship_idx, int16_t target_x, int16_t target_y);

    /**
     * @brief Returns the per-step turning cost for a combat ship.
     * @param ship_idx Combat ship index to inspect.
     */
    int16_t __cdecl Turn_Cost(int16_t ship_idx);

    /**
     * @brief Moves a ship toward a destination and resolves movement-side combat effects.
     * @param ship_idx Combat ship index to move.
     * @param dest_x Destination grid X coordinate.
     * @param dest_y Destination grid Y coordinate.
     * @param move_cost_unused Preserved unused caller-supplied move-cost slot.
     */
    int16_t __cdecl Move_Ship(int16_t ship_idx, int16_t dest_x, int16_t dest_y, int16_t move_cost_unused);

    /**
     * @brief Turns a ship toward a target square while consuming available movement.
     * @param ship_idx Combat ship index to rotate.
     * @param target_x Target grid X coordinate.
     * @param target_y Target grid Y coordinate.
     */
    void __cdecl Turn_To_Face(int16_t ship_idx, int16_t target_x, int16_t target_y);

    /**
     * @brief Returns whether the combat ship is currently trapped in a black hole effect.
     * @param ship_idx Combat ship index to inspect.
     */
    bool __cdecl Ship_In_Black_Hole(int16_t ship_idx);

    /**
     * @brief Computes the discrete combat facing from one point to another.
     * @param x1 Source X coordinate.
     * @param y1 Source Y coordinate.
     * @param x2 Destination X coordinate.
     * @param y2 Destination Y coordinate.
     */
    int16_t __cdecl Get_Facing(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

    /**
     * @brief Plays the teleport animation while moving a ship to a destination square.
     * @param ship_idx Combat ship index to animate.
     * @param target_x Destination grid X coordinate.
     * @param target_y Destination grid Y coordinate.
     */
    void __cdecl Draw_Teleporting_Ship(int16_t ship_idx, int16_t target_x, int16_t target_y);

    /**
     * @brief Tests whether rotating toward a facing would trigger a damaging missile impact.
     * @param ship_idx Combat ship index to simulate.
     * @param target_facing In/out facing updated to the last safe value.
     */
    int16_t __cdecl Rotate_Ship_Until_Impact(int16_t ship_idx, int16_t* target_facing);

    /**
     * @brief Returns whether the current interpolated position maps to a legal destination square.
     * @param ship_idx Combat ship index that is moving.
     * @param x Current interpolated X position in pixels.
     * @param y Current interpolated Y position in pixels.
     * @param target_x Destination X position in pixels.
     * @param target_y Destination Y position in pixels.
     */
    int16_t __cdecl Ship_In_Legal_Square(int16_t ship_idx, int16_t x, int16_t y, int16_t target_x, int16_t target_y);

    /**
     * @brief Simulates movement until impact to clamp a path to the last legal safe position.
     * @param ship_idx Combat ship index to simulate.
     * @param dest_x In/out destination X position in pixels.
     * @param dest_y In/out destination Y position in pixels.
     * @param dest_facing Out facing reached by the simulated move.
     */
    int16_t __cdecl Test_Move_Until_Impact(int16_t ship_idx, int16_t* dest_x, int16_t* dest_y, int16_t* dest_facing);

    /**
     * @brief Handles incoming tactical-combat network packets for the current client.
     * @param sender_net_id Network sender identifier.
     * @param message_id Tactical-combat message type.
     * @param message_size Size of the packet payload in bytes.
     * @param message Decoded tactical-combat packet payload.
     */
    void __cdecl Tactical_Combat_Receive_Message(int32_t sender_net_id, uint16_t message_id, int32_t message_size, s_net_tactical_combat_message* message);
} // namespace CMBTMOV1