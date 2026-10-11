#pragma once

#include <cstdint>
#include <cstdio>

struct s_lbx_header;
struct s_animation_header;

namespace farload {
    /* Reports a farload error and terminates with a composed message.
     * lib_name: Library or file name associated with the failure.
     * error_code: Internal error selector used to choose the message text.
     * entry_idx: Entry index associated with the failure.
     */
    void __cdecl Error_Handler(const char* lib_name, int32_t error_code, int32_t entry_idx);

    extern uint32_t farload_file_flag;
    extern char farload_file_name[16];
    extern FILE* farload_fptr;
    extern s_lbx_header lbx_header;
    extern char secondary_drive_path[52];

    /* Reloads an LBX entry using the first-block allocator.
     * path: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * allocation_base: Allocation anchor passed to the allocator.
     */
    void* __cdecl Far_Reload(const char* path, int32_t entry_idx, void* allocation_base);

    /* Loads structured record data into a caller-supplied buffer.
     * filename: Library path to load from.
     * resource_id: Zero-based LBX entry index.
     * buffer: Destination buffer for the loaded data.
     * start_idx: First record to load.
     * record_count: Number of records to load.
     * expected_element_size: Expected size of each record in bytes.
     */
    void* __cdecl Farload_Data_Static(const char* filename, int32_t resource_id, void* buffer, int32_t start_idx, int32_t record_count, int32_t expected_element_size);

    /* Reloads an LBX entry using the next-block allocator.
     * filename: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * buffer: Allocation anchor passed to the allocator.
     */
    void* __cdecl Far_Reload_Next(const char* filename, int32_t entry_idx, void* buffer);

    /* Loads record data from a structured LBX entry.
     * library_name: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * buffer: Destination buffer or allocation anchor, depending on allocation mode.
     * allocation_mode: Selects which allocator path to use.
     * start_idx: First record to load.
     * record_count: Number of records to load, or 0xFFFF to load the remainder.
     * expected_element_size: Expected size of each record in bytes.
     */
    void* __cdecl Farload_Library_Data(const char* library_name, int32_t entry_idx, void* buffer, int16_t allocation_mode, uint32_t start_idx, uint16_t record_count, uint16_t expected_element_size);

    /* Reloads structured record data using the first-block allocator.
     * path: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * buffer: Allocation anchor passed to the allocator.
     * start_idx: First record to load.
     * record_count: Number of records to load.
     * expected_element_size: Expected size of each record in bytes.
     */
    void* __cdecl Far_Reload_Data(const char* path, int32_t entry_idx, void* buffer, uint32_t start_idx, uint16_t record_count, uint16_t expected_element_size);

    /* Reloads structured record data using the next-block allocator.
     * filename: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * buffer: Allocation anchor passed to the allocator.
     * start_idx: First record to load.
     * record_count: Number of records to load.
     * expected_element_size: Expected size of each record in bytes.
     */
    void* __cdecl Far_Reload_Next_Data(const char* filename, int32_t entry_idx, void* buffer, uint32_t start_idx, uint16_t record_count, uint16_t expected_element_size);

    /* Loads one LBX entry and allocates storage for it.
     * lib_name: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     * allocation_base: Allocation anchor used by chained allocator modes.
     * allocation_mode: Selects which allocator path to use.
     */
    void* __cdecl Farload_Library(const char* lib_name, int32_t entry_idx, void* allocation_base, int32_t allocation_mode);

    /* Reads the record count stored at the start of a structured LBX entry.
     * file_name: Library path to inspect.
     * entry_idx: Zero-based LBX entry index.
     */
    int16_t __cdecl Get_Farload_Data_Record_Count(const char* file_name, int32_t entry_idx);

    /* Loads one LBX entry into a newly allocated buffer.
     * file_name: Library path to load from.
     * entry_idx: Zero-based LBX entry index.
     */
    void* __cdecl Farload(char* file_name, int32_t entry_idx);

    /* Stores a fallback directory used when the primary library path cannot be opened.
     * input_path: Directory path to append file names to during fallback loads.
     */
    void __cdecl Set_Alternate_Path(char* input_path);
} // namespace farload