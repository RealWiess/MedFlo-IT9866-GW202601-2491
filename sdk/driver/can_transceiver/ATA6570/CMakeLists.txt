add_library(can_transceiver STATIC)

target_include_directories(can_transceiver
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/can_transceiver/
)

target_sources(can_transceiver
PRIVATE
    ata_api.c
    sw_spi.c
)
