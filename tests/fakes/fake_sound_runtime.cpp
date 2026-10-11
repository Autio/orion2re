#include "pch.h"

namespace exitm {
    [[noreturn]] void __cdecl Exit_With_Message(const char*) {
        std::abort();
    }
} // namespace exitm