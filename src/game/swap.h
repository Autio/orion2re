#pragma once
#include <cstdint>

namespace swap {
    extern int32_t swap_block_size;
    extern uint8_t* swap_block_address;

    /* Saves the allocator-backed block payload and its allocation header to the temporary swap file.
     * block_address: Pointer to the payload bytes immediately after the allocation header.
     * block_size: Payload size in bytes, excluding the allocation header.
     */
    void __cdecl Save_Block_To_Drive(uint8_t* block_address, int32_t block_size);
    /* Restores the previously saved allocator-backed block payload and header from the temporary swap file. */
    void __cdecl Restore_Block_From_Drive();
} // namespace swap