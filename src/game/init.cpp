#include "pch.h"

namespace init {
    namespace {
        constexpr size_t kIoBufferSize = 0x2000;
    }

    s_animation_header* io_buffer;

    void __cdecl Create_IO_Buffer() {
        io_buffer = (s_animation_header*)malloc(kIoBufferSize);
        memset(io_buffer, 0, kIoBufferSize);
    }

    int8_t __cdecl Init_Drivers(uint32_t unused, char* font_name) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: Init_Drivers start font='%s'", font_name != nullptr ? font_name : "<null>");

        init::Create_IO_Buffer();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: io buffer created");

        video::Init_Video_Drivers();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: video drivers initialized");

        remap::Sort_Palette_List();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: palette list sorted");

        sound::Init_Sound();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: sound initialized");

        if (*font_name == '\0') {
            sysfont::Load_System_Font();
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: system font loaded");
        } else {
            fonts::Load_Font_File(font_name);
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: font file loaded '%s'", font_name);
        }

        file_ani::Reset_File_Animation_Buffer();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: file animation buffer reset");

        // Clear both video pages to black (0)
        video::Set_Page_Off();
        graphics::Fill(0, 0, 639, 479, 0);

        platform::Toggle_Pages_Stub();

        video::Set_Page_Off();
        graphics::Fill(0, 0, 639, 479, 0);

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "init: Init_Drivers complete");
        return 1;
    }
} // namespace init