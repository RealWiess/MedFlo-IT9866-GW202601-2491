add_library(can_bus STATIC)

target_include_directories(can_bus
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/can_bus/it9860
)

target_sources(can_bus
PRIVATE
    can_api.c
    can_hw.c
)
