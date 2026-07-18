add_library(can_hal STATIC)

target_include_directories(can_hal
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/can_bus
)

target_link_libraries(can_hal
PRIVATE
    can_bus
)

target_sources(can_hal
PRIVATE
    can_handle.c
    can_handle.h
)
