get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

if (ENABLE_CODECS_PLUGIN)
    set(TARGET_NAME "${PLUGIN_NAME}.plugin")
    add_executable(${TARGET_NAME})

    if (DEFINED CFG_ROM_COMPRESS)
        set(args ${args} -z -b 512K)
        add_custom_command(
            TARGET ${PLUGIN_NAME}.plugin
            POST_BUILD
            COMMAND ${CMAKE_OBJCOPY}
                    #-O binary ${CMAKE_CURRENT_BINARY_DIR}/mp3v2dec.plugin ${LIBRARY_OUTPUT_PATH}/mp3v2dec.codecs
                    -O binary ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin ${LIBRARY_OUTPUT_PATH}/mp3.rawcodecs
            COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/mp3.rawcodecs
            COMMAND mkrom
            ARGS ${args} ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${LIBRARY_OUTPUT_PATH}/mp3.rawcodecs ${LIBRARY_OUTPUT_PATH}/mp3.codecs
        )
    endif()
else()
    set(TARGET_NAME "${PLUGIN_NAME}")
    add_executable(${TARGET_NAME})
endif()

target_include_directories(${TARGET_NAME}
PRIVATE
    .
    ./libavutil
    ${PROJECT_SOURCE_DIR}/sdk/include
)

target_sources(${TARGET_NAME}
PRIVATE
    main.c
    mpegaudiodec.c
    mpegaudio.c
    mpegaudiodata.c
    libavutil/mem.c
    mpegaudiodecheader.c
    dct32.c
    mpegaudiodsp.c
    mpegaudiodsp_template.c
    bitstream.c
    libavutil/mathematics.c
)

add_dependencies(${TARGET_NAME} main)
