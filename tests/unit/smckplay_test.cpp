#include "pch.h"

#include "doctest/doctest.h"
#include "game/smckplay.h"

namespace test::fakes {
    extern int playback_stream_open_count;
    extern int32_t last_playback_stream_volume;

    void Reset_Smacker_Audio_Fake_();
}

DOCTEST_TEST_CASE("embedded Smacker audio follows the enabled channel volume") {
    struct s_audio_policy_case {
        uint8_t music_on;
        int8_t music_level;
        uint8_t sound_fx_on;
        int8_t sound_fx_level;
        int expected_open_count;
        int32_t expected_volume;
    };

    const s_audio_policy_case cases[] = {
        {0, 25, 0, 75, 0, 0},
        {0, 25, 1, 75, 1, 75},
        {1, 25, 0, 75, 1, 25},
        {1, 25, 1, 75, 1, 25},
    };

    for (const s_audio_policy_case& test_case : cases) {
        test::fakes::Reset_Smacker_Audio_Fake_();
        MOX::settings.music_on = test_case.music_on;
        MOX::settings.music_level = test_case.music_level;
        MOX::settings.sound_fx_on = test_case.sound_fx_on;
        MOX::settings.sound_fx_level = test_case.sound_fx_level;

        const char audio_enabled =
            (test_case.music_on != 0 || test_case.sound_fx_on != 0) ? 1 : 0;

        DOCTEST_REQUIRE(smckplay::Open_Smacker_Animation("policy.smk", audio_enabled, 0) == 1);
        DOCTEST_CHECK(test::fakes::playback_stream_open_count == test_case.expected_open_count);
        if (test_case.expected_open_count != 0) {
            DOCTEST_CHECK(test::fakes::last_playback_stream_volume == test_case.expected_volume);
        }

        smckplay::Close_Smacker_Animation();
    }
}