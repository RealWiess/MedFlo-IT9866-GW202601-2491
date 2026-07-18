get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

if (ENABLE_CODECS_PLUGIN)
    set(TARGET_NAME "${PLUGIN_NAME}.plugin")
    add_executable(${TARGET_NAME})

    set(args ${args} -z -b 512K)
    add_custom_command(
        TARGET ${PLUGIN_NAME}.plugin
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY}
                #-O binary ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin ${LIBRARY_OUTPUT_PATH}/flac.codecs
                -O binary ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin ${LIBRARY_OUTPUT_PATH}/flac.rawcodecs
        COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/flac.rawcodecs
        COMMAND mkrom
        ARGS ${args} ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${LIBRARY_OUTPUT_PATH}/flac.rawcodecs ${LIBRARY_OUTPUT_PATH}/flac.codecs
    )
else()
    set(TARGET_NAME "${PLUGIN_NAME}")
    add_executable(${TARGET_NAME})
endif()

target_include_directories(${TARGET_NAME}
PRIVATE
    .
    ./include
    ${PROJECT_SOURCE_DIR}/sdk/include
)

target_compile_options(${TARGET_NAME}
PRIVATE
    #-Wall
    -Werror
    #-Wextra
)

target_sources(${TARGET_NAME}
PRIVATE
    bitmath.c
    bitreader.c
    bitwriter.c
    cpu.c
    crc.c
    fixed.c
    fixed_intrin_sse2.c
    fixed_intrin_ssse3.c
    float.c
    format.c
    lpc.c
    lpc_intrin_neon.c
    lpc_intrin_sse2.c
    lpc_intrin_sse41.c
    lpc_intrin_avx2.c
    lpc_intrin_fma.c
    lpc_intrin_vsx.c
    md5.c
    memory.c
    stream_decoder.c
    bitwise.c
    framing.c
    ogg_decoder_aspect.c
    ogg_mapping.c
    my_alloc.c
    main.c
)

add_dependencies(${TARGET_NAME} main)
