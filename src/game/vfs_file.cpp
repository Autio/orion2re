#include "pch.h"

#include "vfs_detail.h"

namespace vfs_detail {
    const s_vfs_backend_ops g_vfs_raw_file_ops = {
        Raw_File_Read_,
        Raw_File_Size_,
        Raw_File_Seek_,
        Raw_File_Tell_,
        Raw_File_Close_,
    };

    bool Determine_File_Size_(FILE* file, int32_t* size_out) {
        long original_offset;
        long end_offset;

        if (file == nullptr || size_out == nullptr) {
            return false;
        }

        original_offset = ::ftell(file);
        if (original_offset < 0) {
            return false;
        }

        if (::fseek(file, 0, SEEK_END) != 0) {
            return false;
        }

        end_offset = ::ftell(file);
        if (end_offset < 0 || end_offset > INT32_MAX) {
            ::fseek(file, original_offset, SEEK_SET);
            return false;
        }

        if (::fseek(file, original_offset, SEEK_SET) != 0) {
            return false;
        }

        *size_out = (int32_t)end_offset;
        return true;
    }

    s_vfs_file* Open_Raw_File_(const char* resolved_path, const char* mode) {
        FILE* file;
        s_vfs_raw_file_stream* raw_stream;
        s_vfs_file* vfs_file;

        file = fopen_case(resolved_path, mode);
        if (file == nullptr) {
            return nullptr;
        }

        raw_stream = new s_vfs_raw_file_stream{};
        raw_stream->file = file;
        raw_stream->data_offset = 0;

        vfs_file = new s_vfs_file{};
        vfs_file->backend_kind = VFS_BACKEND_RAW_FILE;
        vfs_file->ops = &g_vfs_raw_file_ops;
        vfs_file->backend_state = raw_stream;
        vfs_file->position = 0;

        if (!Determine_File_Size_(file, &vfs_file->data_size)) {
            ::fclose(file);
            delete raw_stream;
            delete vfs_file;
            return nullptr;
        }

        return vfs_file;
    }

    s_vfs_raw_file_stream* Get_Raw_File_Stream_(s_vfs_file* file) {
        if (file == nullptr || file->backend_kind != VFS_BACKEND_RAW_FILE) {
            return nullptr;
        }

        return (s_vfs_raw_file_stream*)file->backend_state;
    }

    size_t Raw_File_Read_(void* ptr, size_t size, size_t count, s_vfs_file* file) {
        s_vfs_raw_file_stream* raw_stream;
        size_t remaining_bytes;
        size_t max_count;
        size_t read_count;

        if (ptr == nullptr || file == nullptr || size == 0 || count == 0) {
            return 0;
        }

        if (file->position >= file->data_size) {
            return 0;
        }

        raw_stream = Get_Raw_File_Stream_(file);
        if (raw_stream == nullptr) {
            return 0;
        }

        remaining_bytes = (size_t)(file->data_size - file->position);
        max_count = remaining_bytes / size;
        if (max_count == 0) {
            return 0;
        }

        if (max_count > count) {
            max_count = count;
        }

        read_count = ::fread(ptr, size, max_count, raw_stream->file);
        if (read_count > 0) {
            file->position += (int32_t)(read_count * size);
        }

        return read_count;
    }

    int32_t Raw_File_Size_(s_vfs_file* file) {
        if (file == nullptr) {
            return -1;
        }

        return file->data_size;
    }

    int32_t Raw_File_Seek_(s_vfs_file* file, int32_t offset, int32_t origin) {
        s_vfs_raw_file_stream* raw_stream;
        int64_t new_position;

        if (file == nullptr) {
            return -1;
        }

        switch (origin) {
            case SEEK_SET:
                new_position = offset;
                break;
            case SEEK_CUR:
                new_position = (int64_t)file->position + offset;
                break;
            case SEEK_END:
                new_position = (int64_t)file->data_size + offset;
                break;
            default:
                return -1;
        }

        if (new_position < 0 || new_position > file->data_size) {
            return -1;
        }

        raw_stream = Get_Raw_File_Stream_(file);
        if (raw_stream == nullptr || ::fseek(raw_stream->file, raw_stream->data_offset + (long)new_position, SEEK_SET) != 0) {
            return -1;
        }

        file->position = (int32_t)new_position;
        return 0;
    }

    int32_t Raw_File_Tell_(s_vfs_file* file) {
        if (file == nullptr) {
            return -1;
        }

        return file->position;
    }

    int32_t Raw_File_Close_(s_vfs_file* file) {
        s_vfs_raw_file_stream* raw_stream;
        int result;

        if (file == nullptr) {
            return -1;
        }

        raw_stream = Get_Raw_File_Stream_(file);
        result = raw_stream != nullptr ? ::fclose(raw_stream->file) : -1;
        delete raw_stream;
        delete file;
        return result;
    }
} // namespace vfs_detail