enable_language(C ASM)

add_executable(stl_runtime.plugin)

target_sources(stl_runtime.plugin
PRIVATE
    ../../src/stl_cpu_startup.S
    main.c
)

add_custom_command(
TARGET
    stl_runtime.plugin
POST_BUILD
    COMMAND ${CMAKE_OBJDUMP}
            -D ${CMAKE_CURRENT_BINARY_DIR}/stl_runtime.plugin > ${LIBRARY_OUTPUT_PATH}/stl_runtime.dbg
    COMMAND ${READELF}
            -a ${CMAKE_CURRENT_BINARY_DIR}/stl_runtime.plugin > ${CMAKE_CURRENT_BINARY_DIR}/stl_runtime.symbol
    COMMAND ${CMAKE_OBJCOPY}
            -O binary ${CMAKE_CURRENT_BINARY_DIR}/stl_runtime.plugin ${LIBRARY_OUTPUT_PATH}/stl_runtime.raw
    COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/stl_runtime.raw
)

add_dependencies(stl_runtime.plugin main)
