#pragma once

namespace reduce {
    extern s_palette_entry reduced_palette_list[512];

    extern int16_t ncubes;

    extern uint16_t active_cube_count;

    extern int16_t HistPtr[512];

    extern s_palette_cube cube_list[256];

    extern uint8_t color_weight[512];

    /** Clears the accumulated palette entries queued for reduction. */
    void __cdecl Clear_Reduced_Palette_Groups();

    /** Appends an animation palette to the shared reduction list.
     *  @param anim Animation resource whose embedded palette is appended.
     */
    void __cdecl Add_Reduced_Palette_Group(s_animation_header* anim);

    /** Initializes the first cube to cover the source palette entries.
     *  @param palette Palette entries being reduced.
     *  @param num_colors Number of source entries to include.
     */
    void __cdecl Initial_Cube(s_palette_entry* palette, int16_t num_colors);

    /** Recomputes a cube's RGB bounds from the indexed palette entries.
     *  @param cube Cube whose bounds are updated.
     *  @param palette_entries Source palette entries referenced by the histogram indices.
     */
    void __cdecl Shrink_Cube(s_palette_cube* cube, s_palette_entry* palette_entries);

    /** Finds the splittable cube with the widest remaining color range. */
    int16_t __cdecl Find_Next_Lowest_Cube_Level();

    /** Returns the dominant color axis for a cube.
     *  @param cube_idx Index of the cube to inspect.
     */
    int16_t __cdecl Find_Longest_Axis(int16_t cube_idx);

    /** Sorts cube member indices by one RGB channel.
     *  @param palette Palette entries referenced by the indices.
     *  @param indices Histogram indices to sort in place.
     *  @param num_indices Number of indices in the range.
     *  @param color_channel RGB channel selector: 0 red, 1 green, 2 blue.
     */
    void __cdecl Sort_Cube_Points(s_palette_entry* palette, int16_t* indices, int16_t num_indices, int16_t color_channel);

    /** Splits one cube into two weighted subranges along the chosen color axis.
     *  @param palette_entries Palette entries referenced by the cube histogram range.
     *  @param cube_index Index of the cube to split.
     *  @param color_channel RGB channel selector: 0 red, 1 green, 2 blue.
     */
    void __cdecl Split_Cube(s_palette_entry* palette_entries, int16_t cube_index, int16_t color_channel);

    /** Reduces a palette range to a smaller set of representative colors.
     *  @param palette Palette to rewrite with reduced colors.
     *  @param start_index First destination palette slot to update.
     *  @param num_colors_src Number of source colors to reduce.
     *  @param num_colors_dst Number of reduced colors to produce.
     */
    void __cdecl Reduce_Palette(s_palette_entry* palette, int16_t start_index, int16_t num_colors_src, int16_t num_colors_dst);

    /** Reduces queued palette groups and copies the result into the active video palette.
     *  @param start_index First destination index in the current palette.
     *  @param color_count Number of reduced colors to emit.
     */
    void __cdecl Compress_Reduced_Palette_Groups(int16_t start_index, int16_t color_count);
} // namespace reduce