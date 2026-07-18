get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

if (ENABLE_CODECS_PLUGIN)
    set(TARGET_NAME "${PLUGIN_NAME}.plugin")
    add_executable(${TARGET_NAME})

    if (DEFINED CFG_ROM_COMPRESS)
        set(args ${args} -z -b 512K)

        add_custom_command(
            TARGET vorbisdecode.plugin
            POST_BUILD
            COMMAND ${CMAKE_OBJCOPY}
                    #-O binary ${CMAKE_CURRENT_BINARY_DIR}/vorbisdecode.plugin ${LIBRARY_OUTPUT_PATH}/vorbis.codecs
                    -O binary ${CMAKE_CURRENT_BINARY_DIR}/vorbisdecode.plugin ${LIBRARY_OUTPUT_PATH}/vorbis.rawcodecs
            COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/vorbis.rawcodecs
            COMMAND mkrom
            ARGS ${args} ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${LIBRARY_OUTPUT_PATH}/vorbis.rawcodecs ${LIBRARY_OUTPUT_PATH}/vorbis.codecs
        )
    endif()
else()
    set(TARGET_NAME "${PLUGIN_NAME}")
    add_executable(${TARGET_NAME})
endif()

target_compile_definitions(${TARGET_NAME}
PRIVATE
    _ARM_ASSEM_
    _LOW_ACCURACY_
)

target_include_directories(${TARGET_NAME}
PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${TARGET_NAME}
PRIVATE
    memlib.c
    mm.c
    bitwise.c
    framing.c
    block.c
    codebook.c
    floor0.c
    floor1.c
    info.c
    mapping0.c
    mdct.c
    registry.c
    res012.c
    sharedbook.c
    synthesis.c
    # vorbisfile.c
    window.c
    main.c
)

add_dependencies(${TARGET_NAME} main)
