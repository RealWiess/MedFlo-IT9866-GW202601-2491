enable_language(C ASM)

add_executable(stl_init.plugin)

target_sources(stl_init.plugin
PRIVATE
    ../../src/stl_cpu_startup.S
    main.c
)

add_custom_command(
TARGET
    stl_init.plugin
POST_BUILD
    COMMAND ${CMAKE_OBJDUMP}
            -D ${CMAKE_CURRENT_BINARY_DIR}/stl_init.plugin > ${LIBRARY_OUTPUT_PATH}/stl_init.dbg
    COMMAND ${READELF}
            -a ${CMAKE_CURRENT_BINARY_DIR}/stl_init.plugin > ${CMAKE_CURRENT_BINARY_DIR}/stl_init.symbol
    COMMAND ${CMAKE_OBJCOPY}
            -O binary ${CMAKE_CURRENT_BINARY_DIR}/stl_init.plugin ${LIBRARY_OUTPUT_PATH}/stl_init.raw
    COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/stl_init.raw
)

add_dependencies(stl_init.plugin main)
