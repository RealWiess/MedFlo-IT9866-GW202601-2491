add_library(brotli STATIC)

target_compile_definitions(brotli
PRIVATE
    ITP_FAT_ACCEL
)

target_include_directories(brotli
PRIVATE
    c/include
)

target_sources(brotli
PRIVATE
    c/common/constants.c
    c/common/context.c
    c/common/dictionary.c
    c/common/platform.c
    c/common/shared_dictionary.c
    c/common/transform.c
    c/dec/bit_reader.c
    c/dec/decode.c
    c/dec/huffman.c
    c/dec/state.c
)