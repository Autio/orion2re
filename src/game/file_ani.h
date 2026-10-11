#pragma once

#include <cstdint>

namespace file_ani {
#define FILE_ANI_PRELOAD_SIZE 0x2000u
#define FILE_ANI_MULTI_SLOT_COUNT 3u
#define FILE_ANI_MULTI_SLOT_SIZE 0xAAAu

    extern uint8_t* file_animation_buffer;

    extern uint32_t file_animation_open;

    extern uint32_t file_animation_length;

    extern uint32_t file_animation_end;

    extern s_vfs_file* file_animation_fptr;

    extern uint32_t file_animation_opened_flag[FILE_ANI_MULTI_SLOT_COUNT];

    extern s_vfs_file* file_animation[FILE_ANI_MULTI_SLOT_COUNT];

    extern uint32_t file_animation_start_multi[FILE_ANI_MULTI_SLOT_COUNT];

    extern uint32_t file_animation_end_multi[FILE_ANI_MULTI_SLOT_COUNT];

    extern uint32_t* file_lbx_header;

    extern uint32_t file_animation_length_multi[FILE_ANI_MULTI_SLOT_COUNT];

    extern uint32_t file_animation_start;

    extern uint8_t* file_animation_header;

    /**
     * Sets the buffer used to load animation frame data.
     * @param buffer Destination buffer for loaded frame bytes.
     */
    void __cdecl Set_File_Animation_Buffer(void* buffer);

    /** Resets the animation frame buffer to the default offscreen surface. */
    void __cdecl Reset_File_Animation_Buffer();

    /**
     * Opens an animation file entry and loads its header into the shared I/O buffer.
     * @param filename LBX file path.
     * @param entry_index Zero-based animation entry index.
     */
    void __cdecl Open_File_Animation(const char* filename, int16_t entry_index);

    /**
     * Opens an animation file entry into one of the multi-animation slots.
     * @param filename LBX file path.
     * @param entry_index Zero-based animation entry index.
     * @param animation_slot Multi-animation slot index in the range [0, 2].
     */
    void __cdecl Open_Multi_File_Animation(const char* filename, int16_t entry_index, int16_t animation_slot);

    /**
     * Clamps and sets the current frame for the active single-file animation.
     * @param frame_index Requested frame index.
     */
    void __cdecl Set_File_Animation_Frame(int16_t frame_index);

    /** Calls Draw_File_Animation through the original thunk entry point. */
    void __cdecl thunk_Draw_File_Animation();

    /** Draws the current single-file animation frame and advances the frame state. */
    void __cdecl Draw_File_Animation();

    /** Draws the current single-file differential frame and advances the frame state. */
    void __cdecl Draw_File_Animation_Stencil();

    /** Returns the current frame index for the active single-file animation. */
    int16_t __cdecl Get_File_Animation_Frame();

    /** Returns the highest valid frame index for the active single-file animation. */
    int16_t __cdecl Get_File_Animation_Total_Frames();

    /**
     * Draws the current frame from a multi-animation slot and advances the frame state.
     * @param anim_idx Multi-animation slot index.
     */
    void __cdecl Draw_Multi_File_Animation(int16_t anim_idx);

    /**
     * Draws the current differential frame from a multi-animation slot and advances the frame state.
     * @param anim_idx Multi-animation slot index.
     */
    void __cdecl Draw_Multi_File_Animation_Stencil(int16_t anim_idx);

    /**
     * Returns the current frame index for a multi-animation slot.
     * @param anim_idx Multi-animation slot index.
     */
    int16_t __cdecl Get_Multi_File_Animation_Frame(int16_t anim_idx);

    /**
     * Clamps and sets the current frame for a multi-animation slot.
     * @param anim_idx Multi-animation slot index.
     * @param frame_index Requested frame index.
     */
    void __cdecl Set_Multi_File_Animation_Frame(int16_t anim_idx, int16_t frame_index);

    /**
     * Applies palette data for a multi-animation slot when the animation requests it.
     * @param anim_idx Multi-animation slot index.
     */
    void __cdecl Draw_Multi_File_Animation_Palette(int32_t anim_idx);
} // namespace file_ani