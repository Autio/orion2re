#include "pch.h"

extern "C" size_t SDLCALL SDL_strlcpy(char* destination, const char* source,
                                       size_t destination_size) {
    const size_t source_length = strlen(source);
    if (destination_size != 0) {
        const size_t copy_length = source_length < destination_size - 1
            ? source_length
            : destination_size - 1;
        memcpy(destination, source, copy_length);
        destination[copy_length] = '\0';
    }
    return source_length;
}

extern "C" size_t SDLCALL SDL_strlcat(char* destination, const char* source,
                                       size_t destination_size) {
    size_t destination_length = 0;
    while (destination_length < destination_size &&
           destination[destination_length] != '\0') {
        destination_length++;
    }

    const size_t source_length = strlen(source);
    if (destination_length < destination_size) {
        const size_t available = destination_size - destination_length - 1;
        const size_t copy_length = source_length < available
            ? source_length
            : available;
        memcpy(destination + destination_length, source, copy_length);
        destination[destination_length + copy_length] = '\0';
    }

    return destination_length + source_length;
}