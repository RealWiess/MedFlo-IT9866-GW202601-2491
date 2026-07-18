add_library(jpg STATIC)

target_compile_definitions(jpg
PRIVATE
    CFG_ITP_PLATFORM
    #CFG_PAL_PLATFORM
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(jpg
    PRIVATE
        DEBUG
    )
endif()

target_include_directories(jpg
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/jpg
    ${PROJECT_SOURCE_DIR}/sdk/include/isp
)

target_sources(jpg
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/jpg/ite_jpg.h
    ${PROJECT_SOURCE_DIR}/sdk/include/jpg/jpg_err.h
    ${PROJECT_SOURCE_DIR}/sdk/include/jpg/jpg_stream.h
    ite_jpg.c
    jpg_types.h
    jpg_codec.h
    jpg_common.c
    jpg_common.h
    jpg_config.h
    jpg_dec_mjpg.c
    jpg_dec_cmd.c
    jpg_decoder.c
    jpg_defs.h
    jpg_enc_mjpg.c
    jpg_encoder.c
    jpg_hw.c
    jpg_hw.h
    jpg_parser.c
    jpg_parser.h
    jpg_reg.h
    jpg_stream_file.c
    jpg_stream_mem.c
    register_template.h
    jpg_extern_link.c
    jpg_extern_link.h
)