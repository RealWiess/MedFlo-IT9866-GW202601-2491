add_library(axispi STATIC)
add_library(spi STATIC)

target_link_libraries(spi PRIVATE itp_boot)

if (DEFINED CFG_WATCHDOG_ENABLE)
    add_definitions(
        -DCFG_WATCHDOG_ENABLE
        -DCFG_WATCHDOG_TIMEOUT=${CFG_WATCHDOG_TIMEOUT}
        -DCFG_WATCHDOG_REFRESH_INTERVAL=${CFG_WATCHDOG_REFRESH_INTERVAL}
    )

    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_WATCHDOG_INTR)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_WATCHDOG_IDLETASK)
endif()

if (DEFINED CFG_ARM_NOR_WRITER)
    target_compile_definitions(axispi
    PRIVATE
        ARM_NOR_WRITER
    )
    target_compile_definitions(spi
    PRIVATE
        ARM_NOR_WRITER
    )
endif()

target_include_directories(axispi
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/nor
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/axispi
)
target_include_directories(spi
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/nor
)

target_sources(axispi
PRIVATE
    ./axispi/axispi_hw.c
    ./axispi/mmp_axispi_multi.c
)
target_sources(spi
PRIVATE
    ./spi/spi_hw.c
    ./spi/mmp_spi.c
    ./spi/armNorWriterUtility.c
)
