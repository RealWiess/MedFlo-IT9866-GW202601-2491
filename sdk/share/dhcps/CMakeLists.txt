add_library(dhcps STATIC)

target_compile_definitions(dhcps
PRIVATE
    CFG_DHCPS_ADDR_COUNT=${CFG_DHCPS_ADDR_COUNT}
)

target_include_directories(dhcps
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/driver/
)

target_sources(dhcps
PRIVATE
    dhcps.c
    dhcps.h
)