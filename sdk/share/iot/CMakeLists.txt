add_library(iot STATIC)

target_include_directories(iot
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/iot
)

target_sources(iot
PRIVATE
    iot_socket.c
    iot_system.c
    iot_wlan.c
    protocol_wrapper.c
    airoha_iot_bus.c
)