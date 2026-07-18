file(COPY
    ${CMAKE_BINARY_DIR}/../arm_lite_codec/lib/fa626/stl_init.hex
    DESTINATION ${PROJECT_SOURCE_DIR}/sdk/share/stl/fa626/src/.
)

file(COPY
    ${CMAKE_BINARY_DIR}/../arm_lite_codec/lib/fa626/stl_runtime.hex
    DESTINATION ${PROJECT_SOURCE_DIR}/sdk/share/stl/fa626/src/.
)

enable_language(C ASM)

add_library(stl)

target_include_directories(stl
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include
    ${PROJECT_SOURCE_DIR}/sdk/chip/it${CFG_CHIP_FAMILY}/include/
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

target_compile_definitions(stl
PRIVATE
    CFG_UPGRADE_NOR_IMAGE_SIZE=${CFG_UPGRADE_NOR_IMAGE_SIZE}
    $<$<BOOL:${CFG_BOOTLOADER_ENABLE}>:CFG_BOOTLOADER_ENABLE>
    USE_MARCHX_TEST
)

target_sources(stl
PRIVATE
    stl_clock.c
    stl_cpu_startup.S
    stl_cpu_runtime.S
    stl_cpu.c
    stl_cpu2.c
    stl_crc.c
    stl_ram.S
    stl_ram_runtime.c
    stl_rom.c
    stl_stack.c
    stl_startup.c
)

target_link_libraries(stl
PRIVATE
    risc
    upgrade
)