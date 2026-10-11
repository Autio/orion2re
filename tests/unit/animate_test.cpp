#include "pch.h"

#include "doctest/doctest.h"
#include "game/animate.h"

DOCTEST_TEST_SUITE("animation frame selection") {

DOCTEST_TEST_CASE("frame count uses its unsigned 16-bit value") {
    s_animation_header animation = {};
    animation.frame_count = -32767;

    animate::Set_Animation_Frame(&animation, 1);

    DOCTEST_CHECK(animation.current_frame == 1);
}

DOCTEST_TEST_CASE("high-bit requested frame clamps to the last frame") {
    s_animation_header animation = {};
    animation.frame_count = 3;

    animate::Set_Animation_Frame(&animation, -1);

    DOCTEST_CHECK(animation.current_frame == 2);
}

}