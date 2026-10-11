#include "pch.h"

#include "doctest/doctest.h"
#include "game/sound.h"
#include "game/sound_types.h"

#include <vector>

namespace sound {
    extern int32_t masterVolume;
    extern SDL_Mutex* soundStateMutex;
    extern uint8_t soundOn;
    extern s_sound_channel soundChannels[16];

    int32_t __cdecl Play_Sound_File(int32_t channel_idx, void* wave_data, uint32_t wave_data_size);
} // namespace sound

DOCTEST_TEST_CASE("long sound effects queue their complete sample") {
    constexpr uint32_t sample_size = 0x10000;
    std::vector<uint8_t> wave_data(sizeof(s_sound_wav_header_disk) + sample_size);

    s_sound_wav_header_disk header = {
        {'R', 'I', 'F', 'F'},
        (uint32_t)(sizeof(s_sound_wav_header_disk) - 8 + sample_size),
        {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '},
        16,
        1,
        1,
        8000,
        8000,
        1,
        8,
        {'d', 'a', 't', 'a'},
        sample_size,
    };
    memcpy(wave_data.data(), &header, sizeof(header));

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    DOCTEST_REQUIRE(SDL_InitSubSystem(SDL_INIT_AUDIO));

    sound::soundStateMutex = SDL_CreateMutex();
    DOCTEST_REQUIRE(sound::soundStateMutex != nullptr);
    sound::masterVolume = 100;
    sound::soundOn = 1;
    memset(sound::soundChannels, 0, sizeof(sound::soundChannels));

    DOCTEST_CHECK(sound::Play_Sound_File(0, wave_data.data(), (uint32_t)wave_data.size()) == 0);
    DOCTEST_CHECK(sound::soundChannels[0].sample_offset == sample_size);
    DOCTEST_CHECK((sound::soundChannels[0].flags & 0x02) == 0);

    sound::Stop_Sound(0);
    SDL_DestroyMutex(sound::soundStateMutex);
    sound::soundStateMutex = nullptr;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

DOCTEST_TEST_CASE("rapid animation updates do not accelerate sound fades") {
    constexpr uint32_t sample_size = 13056;
    std::vector<uint8_t> wave_data(sizeof(s_sound_wav_header_disk) + sample_size);

    s_sound_wav_header_disk header = {
        {'R', 'I', 'F', 'F'},
        (uint32_t)(sizeof(s_sound_wav_header_disk) - 8 + sample_size),
        {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '},
        16,
        1,
        1,
        22050,
        22050,
        1,
        8,
        {'d', 'a', 't', 'a'},
        sample_size,
    };
    memcpy(wave_data.data(), &header, sizeof(header));

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    DOCTEST_REQUIRE(SDL_InitSubSystem(SDL_INIT_AUDIO));

    sound::soundStateMutex = SDL_CreateMutex();
    DOCTEST_REQUIRE(sound::soundStateMutex != nullptr);
    sound::masterVolume = 100;
    sound::soundOn = 1;
    memset(sound::soundChannels, 0, sizeof(sound::soundChannels));

    DOCTEST_REQUIRE(
        sound::Play_Sound_File(0, wave_data.data(),
            (uint32_t)wave_data.size()) == 0);
    sound::Fadeout_Sound(0, 10);

    for (int i = 0; i < 10; ++i) {
        sound::Sound_Update();
    }

    DOCTEST_CHECK(sound::soundChannels[0].buffer != nullptr);
    DOCTEST_CHECK((sound::soundChannels[0].flags & 0x01) != 0);

    sound::Stop_Sound(0);
    SDL_DestroyMutex(sound::soundStateMutex);
    sound::soundStateMutex = nullptr;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}