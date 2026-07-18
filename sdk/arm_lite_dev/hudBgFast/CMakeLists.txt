get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_executable(${PLUGIN_NAME}.plugin)
target_sources(${PLUGIN_NAME}.plugin
PRIVATE
    main.c
)

# This custom command is invoked after the plugin is built.
# The command generates a raw binary of the plugin, a debug symbols file,
# a symbol table file and a converted raw image using the DATACONV tool.
# The plugin is built from a single C source file: main.c
# The raw binary is named <plugin_name>.raw
# The debug symbols file is named <plugin_name>.dbg
# The symbol table file is named <plugin_name>.symbol
# The converted raw image is named <plugin_name>.raw
#
# The command consists of 4 sub-commands which are:
# 1. ${CMAKE_OBJCOPY} -O binary <plugin_binary> <raw_image>
#     This generates a raw binary of the plugin.
# 2. ${CMAKE_OBJDUMP} -D <plugin_binary> > <debug_symbols_file>
#     This generates a debug symbols file (a text file) for the plugin.
# 3. ${READELF} -a <plugin_binary> > <symbol_table_file>
#     This generates a symbol table file (a text file) for the plugin.
# 4. ${DATACONV} -x <raw_image>
#     This converts the raw image to hex string format for the plugin.

add_custom_command(
    TARGET ${PLUGIN_NAME}.plugin
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.raw
    POST_BUILD
    COMMAND ${CMAKE_OBJDUMP}
            -D ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin > ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.dbg
    POST_BUILD
    COMMAND ${READELF}
            -a ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin > ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.symbol
    POST_BUILD
    COMMAND ${DATACONV} -x ${LIBRARY_OUTPUT_PATH}/${PLUGIN_NAME}.raw
)
