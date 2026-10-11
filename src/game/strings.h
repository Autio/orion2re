#pragma once

namespace strings {
    /** Swaps the two 16-bit values in place.
     * @param lhs First value to swap.
     * @param rhs Second value to swap.
     */
    void __cdecl Swap_Short(int16_t* lhs, int16_t* rhs);

    /** Swaps the two 32-bit values in place.
     * @param lhs First value to swap.
     * @param rhs Second value to swap.
     */
    void __cdecl Swap(int32_t* lhs, int32_t* rhs);

    /** Swaps the two 8-bit values in place.
     * @param lhs First value to swap.
     * @param rhs Second value to swap.
     */
    void __cdecl Swap_Char(uint8_t* lhs, uint8_t* rhs);

    extern const char s__00551100[1];
    extern const char s_plus_00551118[2];
    extern const char s_minus_0055111c[2];
    extern const char s_space_00551120[2];
    extern const char s_B_005560e4[2];
    extern const char s_S_005569cc[2];
    extern const char s_O_00556afc[2];
    extern const char s_A_00557ed4[2];
    extern const char s_M_00557ed8[2];
    extern const char s_fmt_plus_d_00557ed0[4];
    extern const char s_fmt_plus_15bc_00558cd4[6];
    extern const char s_fmt_plus_10bc_00558cdc[6];
    extern const char s_fmt_plus_d_percent_00558ce4[6];
    extern const char s_fmt_minus_d_bc_00558cec[6];
    extern const char s_fmt_minus_d_percent_00558cf4[6];
    extern const char s_fmt_plus_d_percent_short_00558cfc[5];
    extern const char s_ld_00558280[4];
    extern const char s_0_0055110c[3];
    extern const char s_1_00551110[3];
    extern const char s__0055ac34[2];
    extern const char s_t_T0_5_124__0055ac38[13];

    /** Loads the shared game string tables. */
    void Load_Strings();

    /** Converts uppercase ASCII letters in the string buffer to lowercase in place.
     * @param str NUL-terminated string buffer to modify.
     */
    void __cdecl String_To_Lower(char* str);

    /** Subtracts 0x20 from each byte in the 0x61-0x7F range of the string buffer.
     * @param str NUL-terminated string buffer to modify.
     */
    void __cdecl String_To_Upper(char* str);

    /** Removes leading and trailing space characters from the string buffer in place.
     * @param str NUL-terminated string buffer to trim.
     */
    void __cdecl Trim(char* str);

    /** Removes one element from a packed array by shifting later elements down over it.
     * @param index Zero-based element index to remove.
     * @param array Array storage to compact in place.
     * @param element_size_bytes Size of each element in bytes.
     * @param element_count Number of valid elements in the array.
     */
    void __cdecl Delete_Structure(int32_t index, void* array, uint32_t element_size_bytes, uint32_t element_count);
} // namespace strings