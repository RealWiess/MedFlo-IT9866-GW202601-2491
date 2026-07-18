add_library(nimble)

target_include_directories(nimble
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/nimble
)

if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE)
    target_include_directories(nimble
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble/
    )
elseif (DEFINED CFG_AP6212BLE)
    target_include_directories(nimble
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/ap6212ble/
    )
elseif(DEFINED CFG_AP6236BLE)
    target_include_directories(nimble
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/ap6236ble
    )
elseif(DEFINED CFG_AP6256BLE)
    target_include_directories(nimble
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/ap6256ble
    )
elseif(DEFINED CFG_AP6203BLE)
    target_include_directories(nimble
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/ap6203ble
    )
endif()

file(GLOB ble_SRC
    host/src/*.c
)

#get_filename_component(rm_file ${CMAKE_CURRENT_SOURCE_DIR}/host/src/ble_gatts_lcl.c ABSOLUTE)

#list(REMOVE_ITEM ble_SRC "${rm_file}")

file(GLOB ble_crypt_SRC
    tinycrypt/src/*.c
)

file(GLOB ble_services
    host/services/gap/src/*.c
    host/services/gatt/src/*.c
)

file(GLOB ble_mesh
    host/mesh/src/*.c
)

file(GLOB ble_uart
    transport/uart/*.c
)

file(GLOB ble_store
    host/store/config/src/*.c
)

if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE  OR DEFINED CFG_RTL8821BLE)
    file(GLOB ble_hal
        transport/uart/rtl8723ble/*.c
    )
elseif (DEFINED CFG_AP6256BLE OR DEFINED CFG_AP6236BLE OR DEFINED CFG_AP6212BLE OR DEFINED CFG_AP6203BLE)
    file(GLOB ble_hal
        transport/uart/ap6236ble/*.c
    )
endif()

target_sources(nimble
PRIVATE
    ${ble_SRC}
    ${ble_crypt_SRC}
    ${ble_services}
    ${ble_mesh}
    ${ble_uart}
    ${ble_hal}
    ${ble_store}
    os_msys_init.c
    os_mbuf.c
    os_mempool.c
    endian.c
    mem.c
    npl/freertos/src/npl_os_freertos.c
    host/util/src/addr.c
    nimble_port.c
)

target_compile_definitions_ifdef(CFG_NIMBLE_MESH_TEST nimble PRIVATE CFG_NIMBLE_MESH_TEST)
