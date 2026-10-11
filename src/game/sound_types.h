#pragma once

#pragma pack(push, 1)

struct s_sound_wav_header_disk {
    char riff_tag[4];
    uint32_t chunk_size;
    char wave_tag[4];
    char fmt_tag[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_tag[4];
    uint32_t data_size;
};

#pragma pack(pop)

struct s_sound_channel {
    uint8_t* sample_data;
    int8_t loop_count;
    undefined field2_0x5;
    int8_t volume_index;
    int32_t current_volume;
    uint32_t sample_size;
    uint32_t sample_rate;
    uint32_t sample_offset;
    uint32_t ds_buffer_size;
    uint32_t fade_update_ms;
    uint32_t write_cursor;
    uint8_t flags;
    SDL_AudioStream* buffer;
};

struct s_sound_index {
    uint32_t buffer_offset;
    uint32_t buffer_size;
    int32_t num_active_channels;
    uint8_t active_channel_ids[16];
    int32_t resource_id;
    int32_t next_id;
    int32_t prev_id;
};

struct s_sound_stream {
    char riff_tag[4];
    uint32_t riff_size;
    char wave_tag[4];
    char fmt_tag[4];
    uint32_t fmt_size;
    uint16_t format_tag;
    uint16_t channels;
    uint32_t samples_per_sec;
    uint32_t avg_bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_tag[4];
    uint32_t total_data_size;
    uint32_t data_start_offset;
    undefined field14_0x30;
    undefined field15_0x31;
    undefined field16_0x32;
    uint32_t data_remaining;
    uint32_t bytes_written;
    uint32_t last_lock_size;
    uint32_t field20_0x3f;
    uint32_t write_cursor;
    uint8_t status;
    SDL_AudioStream* ds_buffer;
    uint32_t ds_buffer_size;
    uint32_t block_size;
};

struct s_sound_wav_header {
    char riff_tag[4];
    uint32_t chunk_size;
    char wave_tag[4];
    char fmt_tag[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_tag[4];
    uint32_t data_size;
    void* data_ptr;
};