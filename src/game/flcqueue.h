#pragma once
#include <cstdint>

struct s_animation_header;

typedef struct s_flic_queue_entry s_flic_queue_entry, *Ps_flic_queue_entry;

struct s_flic_queue_entry {
    int16_t x;
    int16_t y;
    int16_t current_frame;
    struct s_animation_header* animation;
    uint8_t play_mode;
    uint8_t use_clipping;
};

namespace FLCQUEUE {
    extern int16_t n_queue_entries;

    extern s_flic_queue_entry flic_queue[100];

    /**
     * Resets the queued FLIC animation state.
     */
    void __cdecl Allocate_Flic_Queue_And_Pointers();

    /**
     * Clears the queued FLIC animation state.
     */
    void __cdecl Free_Flic_Queue_And_Pointers();

    /**
     * Draws and advances every queued FLIC animation.
     */
    void __cdecl Draw_Flic_Queue();

    /**
     * Draws and advances one queued FLIC animation entry.
     * @param entry_idx Index of the queue entry to process.
     */
    void __cdecl Draw_Queue_Entry(int16_t entry_idx);

    /**
     * Removes one queued FLIC animation entry and compacts the queue.
     * @param entry_idx Index of the queue entry to remove.
     */
    void __cdecl Squeze_Out_Queue_Entry(int16_t entry_idx);

    /**
     * Appends an animation to the queue when capacity remains.
     * @param x Screen X position to draw the animation at.
     * @param y Screen Y position to draw the animation at.
     * @param animation Animation resource to enqueue.
     * @param play_mode Playback mode observed by Draw_Queue_Entry (0 loops, 1 removes after the last frame).
     * @param use_clipping Nonzero to clip drawing to the game window.
     */
    void __cdecl Add_Flic_To_End_Of_Queue(int16_t x, int16_t y, s_animation_header* animation, uint8_t play_mode, uint8_t use_clipping);

    /**
     * Queues the standard explosion animation near the given position.
     * @param x Base screen X position for the effect.
     * @param y Base screen Y position for the effect.
     */
    void __cdecl Add_Explosion_Anim(int16_t x, int16_t y);
} // namespace FLCQUEUE