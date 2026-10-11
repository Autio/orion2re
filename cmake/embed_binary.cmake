if(NOT INPUT_FILE OR NOT OUTPUT_FILE OR NOT SYMBOL_NAME)
    message(FATAL_ERROR "INPUT_FILE, OUTPUT_FILE, and SYMBOL_NAME are required.")
endif()

file(READ "${INPUT_FILE}" INPUT_HEX HEX)
string(LENGTH "${INPUT_HEX}" INPUT_HEX_LENGTH)
math(EXPR INPUT_SIZE "${INPUT_HEX_LENGTH} / 2")

get_filename_component(OUTPUT_DIRECTORY "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
file(WRITE "${OUTPUT_FILE}"
    "#pragma once\n\n"
    "static const unsigned char ${SYMBOL_NAME}_data[] = {\n")

set(OFFSET 0)
while(OFFSET LESS INPUT_HEX_LENGTH)
    math(EXPR REMAINING "${INPUT_HEX_LENGTH} - ${OFFSET}")
    if(REMAINING GREATER 48)
        set(CHUNK_LENGTH 48)
    else()
        set(CHUNK_LENGTH "${REMAINING}")
    endif()

    string(SUBSTRING "${INPUT_HEX}" ${OFFSET} ${CHUNK_LENGTH} CHUNK)
    string(REGEX REPLACE "(..)" "0x\\1, " CHUNK "${CHUNK}")
    file(APPEND "${OUTPUT_FILE}" "    ${CHUNK}\n")
    math(EXPR OFFSET "${OFFSET} + ${CHUNK_LENGTH}")
endwhile()

file(APPEND "${OUTPUT_FILE}"
    "};\n"
    "static const int ${SYMBOL_NAME}_size = ${INPUT_SIZE};\n")