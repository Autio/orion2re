#include "pch.h"

#include "fakes/fake_test_support.h"

namespace sound {
    uint8_t close_stream = 1;
    uint8_t musicActive = 0;

    SDL_AudioStream* __cdecl Open_Playback_Stream(uint16_t bits_per_sample, uint16_t channels, uint32_t sample_rate, int32_t volume_percent) {
        (void)bits_per_sample;
        (void)channels;
        (void)sample_rate;
        (void)volume_percent;
        return nullptr;
    }

    void __stdcall Sound_Timer_Callback(uint32_t timer_id, uint32_t message, uint32_t user_data, uint32_t param1, uint32_t param2) {
        (void)timer_id;
        (void)message;
        (void)user_data;
        (void)param1;
        (void)param2;
    }

    void __cdecl Fade_Stream_Down(uint8_t fade_rate) {
        (void)fade_rate;
    }

    int32_t __cdecl Get_Stream_Volume() {
        return 0;
    }

    void __cdecl Stop_Stream() {
    }

    void __cdecl Stream_On() {
    }

    void __cdecl Set_Stream_Max_Volume(int32_t volume) {
        (void)volume;
    }

    void __cdecl Set_Stream_Filename(const char* filename) {
        (void)filename;
    }

    void __cdecl Play_Stream(int32_t stream_id) {
        (void)stream_id;
    }

    void __cdecl Loop_Stream(int32_t count) {
        (void)count;
    }

    void __cdecl Set_Stream_Volume(int32_t volume_percent) {
        (void)volume_percent;
    }

    void __cdecl Fade_Stream_Up(uint8_t fade_increment) {
        (void)fade_increment;
    }

    uint8_t __cdecl Is_Stream_Playing() {
        return 0;
    }

    void __cdecl Set_Audio_Buffer(void* buffer) {
        (void)buffer;
    }

    void __cdecl Sound_On() {
    }

    void __cdecl Stop_All_Sounds() {
    }

    void __cdecl Resume_Sound(int32_t channel_idx) {
        (void)channel_idx;
    }

    void __cdecl Set_Sound_Max_Volume(int32_t volume) {
        (void)volume;
    }

    void __cdecl Stop_Sound(int32_t channel_idx) {
        (void)channel_idx;
    }

    void __cdecl Sound_Update() {
    }

    int32_t __cdecl Play_Sound(int32_t resource_id) {
        test::fakes::g_last_played_sound = resource_id;
        return resource_id;
    }

    void __cdecl Fadeout_Sound(int16_t channel_idx, int8_t volume_idx) {
        (void)channel_idx;
        (void)volume_idx;
    }

    void __cdecl Pitch_Bend_Sound(int32_t handle, int32_t pitch) {
        (void)handle;
        (void)pitch;
    }

    void __cdecl Loop_Sound(int32_t channel_idx, int8_t loop_count) {
        (void)channel_idx;
        (void)loop_count;
    }

    void __cdecl Destroy_Sound() {
    }

    void __cdecl Init_Sound() {
    }
}

namespace ERICNET {
    int __cdecl Mox_Play_Sound(int16_t sound_id) {
        return sound::Play_Sound(sound_id);
    }
}