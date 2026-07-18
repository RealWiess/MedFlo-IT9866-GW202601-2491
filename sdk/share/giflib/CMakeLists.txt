add_library(giflib STATIC)

target_compile_definitions(giflib
PRIVATE
    HAVE_CONFIG_H
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)

    target_compile_definitions(giflib
    PRIVATE
        DEBUG
    )
endif()

target_include_directories(giflib
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include
)

target_sources(giflib
PRIVATE
    dgif_lib.c
    egif_lib.c
    getarg.c
    gif_err.c
    gif_font.c
    gif_hash.c
    gif2rgb.c
    gifalloc.c
    gifbg.c
    gifbuild.c
    gifclrmp.c
    gifcolor.c
    gifecho.c
    giffilter.c
    giffix.c
    gifhisto.c
    gifinto.c
    gifsponge.c
    giftext.c
    giftool.c
    gifwedge.c
    openbsd-reallocarray.c
    qprintf.c
    quantize.c
)