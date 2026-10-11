#include "pch.h"

#include <cstdlib>
#include <cstring>

namespace allocate {
    int32_t mark_stack_pos[2] = {};
    s_mark_stack_entry mark_stack[MARK_STACK_COUNT] = {};

    void __cdecl Allocation_Error(int32_t requested_bytes) {
        char message[64];
        snprintf(message, sizeof(message), "alloc %d", requested_bytes);
        exitm::Exit_With_Message(message);
    }

    void* __cdecl Allocate_Far_Space(uint32_t size) {
        void* ptr = std::malloc(size);
        if (ptr == nullptr) {
            exitm::Exit_With_Message("Null Pointer");
        }
        return ptr;
    }

    void* __cdecl Allocate_Dos_Space(uint32_t size) {
        return Allocate_Far_Space(size);
    }

    void* __cdecl Allocate_Space_No_Header(uint32_t size) {
        return Allocate_Far_Space(size);
    }

    void* __cdecl Allocate_First_Block_No_Header(void* block_ptr, uint32_t size) {
        ((s_alloc_header*)block_ptr - 1)->current = 0;
        return Allocate_Next_Block_No_Header(block_ptr, size);
    }

    void* __cdecl Allocate_Next_Block_No_Header(void* block_ptr, uint32_t size) {
        auto* pool_header = (s_alloc_header*)block_ptr - 1;
        uint32_t aligned_size = (size & 1u) != 0 ? ((size & 0xFFFFFFFCu) + 4u) : size;
        uint32_t current_offset = pool_header->current;
        uint32_t next_offset = current_offset + aligned_size;

        if (next_offset > pool_header->size) {
            return nullptr;
        }

        pool_header->current = next_offset;
        return (uint8_t*)block_ptr + current_offset;
    }

    void* __cdecl Allocate_Next_Block(void* block, uint32_t size) {
        uint32_t padded_size = (size & 1u) != 0 ? ((size & 0xFFFFFFFCu) + 4u) : size;
        auto* pool_header = (s_alloc_header*)block - 1;
        uint32_t current_offset = pool_header->current;
        uint32_t total_required = current_offset + padded_size + sizeof(s_alloc_header);

        if (total_required > pool_header->size) {
            Allocation_Error((int32_t)size);
        }

        auto* sub_block_header = (s_alloc_header*)((uint8_t*)block + current_offset);
        pool_header->current = total_required;
        sub_block_header->current = 0;
        sub_block_header->size = size;
        sub_block_header->reserved2 = 0;
        return sub_block_header + 1;
    }

    void* __cdecl Allocate_First_Block(void* block, uint32_t size) {
        ((s_alloc_header*)block - 1)->current = 0;
        return Allocate_Next_Block(block, size);
    }

    void __cdecl Reallocation_Error(uint32_t requested_size, int32_t bytes_short, int32_t block_capacity) {
        (void)bytes_short;
        (void)block_capacity;
        Allocation_Error((int32_t)requested_size);
    }

    int32_t __cdecl Get_Remaining_Block_Space(void* block) {
        const auto* header = (s_alloc_header*)block - 1;
        return (int32_t)(header->size - header->current);
    }

    uint32_t __cdecl Get_Max_Block_Space(void* block) {
        return ((s_alloc_header*)block - 1)->size;
    }

    void __cdecl Push_Block(void* block) {
        if (mark_stack_pos[0] >= MARK_STACK_COUNT) {
            exitm::Exit_With_Message("Mark Block Stack Size Exceeded");
        }

        int32_t pos = mark_stack_pos[0]++;
        mark_stack[pos].block = block;
        mark_stack[pos].current = ((s_alloc_header*)block - 1)->current;
    }

    void __cdecl Pop_Block(void* unused_block) {
        (void)unused_block;
        int32_t pos = --mark_stack_pos[0];
        ((s_alloc_header*)mark_stack[pos].block - 1)->current = mark_stack[pos].current;
    }

    void __cdecl Mark_Block(void* block_ptr) {
        auto* header = (s_alloc_header*)block_ptr - 1;
        header->reserved2 = header->current;
    }

    void __cdecl Release_Block(void* block) {
        auto* header = (s_alloc_header*)block - 1;
        header->current = header->reserved2;
    }

    void* __cdecl Allocate_Space(uint32_t size) {
        uint32_t padded_size = (size & 1u) != 0 ? ((size & 0xFFFFFFFCu) + 4u) : size;
        auto* header = (s_alloc_header*)Allocate_Far_Space(padded_size + sizeof(s_alloc_header));
        header->current = 0;
        header->size = size;
        header->reserved2 = 0;
        return header + 1;
    }

    void Allocate_Data_Space() {
    }
}