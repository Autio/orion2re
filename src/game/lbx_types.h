#pragma once

struct s_vfs_file;

// Computed in-memory metadata; this is not read directly from the LBX file.
struct s_lbx_stream_entry {
    uint32_t size;
    uint32_t offset;
};

// Only the on-disk LBX header needs packed layout.
#pragma pack(push, 1)

struct s_lbx_disk_header {
    uint16_t entry_count;
    uint16_t magic;
    uint32_t header_data;
};

#pragma pack(pop)

// Runtime stream state. These structs intentionally keep native pointer layout.
struct s_lbx_stream_header {
    uint16_t entry_count;
    uint16_t magic;
    uint32_t header_data;
    s_vfs_file* file;
    uint32_t first_offset;
    s_lbx_stream_entry entries[509];
    uint32_t padding;
    char filename[80];
};

struct s_wave_stream_header {
    uint16_t entry_count;
    uint16_t magic;
    uint32_t header_data;
    s_vfs_file* file;
    uint32_t first_offset;
    s_lbx_stream_entry entries[509];
    uint32_t padding;
    char filename[80];
};