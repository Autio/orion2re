#pragma once

#include <cstddef>
#include <cstdint>

namespace arena {
    inline constexpr size_t DEFAULT_CHUNK_CAPACITY = 32 * 1024;

    struct s_arena;

    struct s_stats {
        size_t chunk_count;
        size_t used_bytes;
        size_t reserved_bytes;
        uint32_t reference_count;
    };

    s_arena* Create(size_t chunk_capacity = DEFAULT_CHUNK_CAPACITY);
    void* Allocate(s_arena* arena, size_t size);
    void* Allocate_Aligned(s_arena* arena, size_t size, size_t alignment);
    void* Allocate_Zeroed(s_arena* arena, size_t size);
    void* Allocate_Zeroed_Aligned(s_arena* arena, size_t size, size_t alignment);
    char* Copy_String(s_arena* arena, const char* source);
    char* Copy_String_N(s_arena* arena, const char* source, size_t length);
    bool Contains(const s_arena* arena, const void* pointer);

    void Add_Ref(s_arena* arena);
    uint32_t Release(s_arena* arena);
    s_stats Get_Stats(const s_arena* arena);
} // namespace arena