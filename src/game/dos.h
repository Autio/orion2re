#pragma once

namespace dos {
#define DOS_MAX_PATH 512

    /**
     * Returns the available free space on the current drive in bytes.
     * @return Available free space, or 0 if the query fails.
     */
    int32_t __cdecl Disk_Free(void);

    /**
     * Finds the first matching file for the provided pattern.
     * @param pattern Search pattern.
     * @param output_name Receives the matched filename.
     * @param output_name_size Size of output_name in bytes.
     * @return true if a matching filename was written to output_name, otherwise false.
     */
    bool __cdecl DIR(const char* pattern, char* output_name, size_t output_name_size);

    /**
     * Deletes the file at the specified path.
     * @param path Path to the file to remove.
     */
    void __cdecl Delete_File(const char* path);

    /**
     * Retrieves a file's last-write timestamp in local time.
     * @param pathname Path to the file to inspect.
     * @return Pointer to the cached local-time structure, or nullptr if the file is not found.
     */
    struct tm* __cdecl Time_Stamp(const char* pathname);
} // namespace dos