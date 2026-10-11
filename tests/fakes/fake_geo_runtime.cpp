#include "pch.h"

#include <cstring>

namespace {
    s_nebula nebulas[MAX_NEBULAS];
}

namespace MOX {
    s_nebula* nebula = nebulas;
    s_animation_header* nebula_pict_seg[12][4];
}

namespace test_geo_runtime {
    void Reset_() {
        std::memset(nebulas, 0, sizeof(nebulas));
        std::memset(MOX::nebula_pict_seg, 0, sizeof(MOX::nebula_pict_seg));
    }

    s_nebula* Nebula_(int index) {
        return &nebulas[index];
    }

    void Set_Nebula_Picture_(int type, s_animation_header* animation) {
        MOX::nebula_pict_seg[type][3] = animation;
    }
}