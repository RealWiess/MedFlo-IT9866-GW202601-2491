add_library(saradc STATIC)

target_include_directories(saradc
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
)

target_sources(saradc
PRIVATE
    saradc.c
    saradc_hw.c
)