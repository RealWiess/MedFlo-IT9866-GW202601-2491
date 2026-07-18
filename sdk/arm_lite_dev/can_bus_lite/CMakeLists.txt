get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_executable(${PLUGIN_NAME}.plugin)

target_include_directories(${PLUGIN_NAME}.plugin
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
)

target_sources(${PLUGIN_NAME}.plugin
PRIVATE
    main.c
    clock.c
    can_bus_lite/can_armlite_api.c
    can_bus_lite/can_hw_armlite.c
)

add_custom_command(
    TARGET ${PLUGIN_NAME}.plugin
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.raw
    POST_BUILD
    COMMAND ${CMAKE_OBJDUMP}
            -D ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin > ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.dbg
    COMMAND ${READELF}
            -a ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin > ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.symbol
    COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.raw
)
