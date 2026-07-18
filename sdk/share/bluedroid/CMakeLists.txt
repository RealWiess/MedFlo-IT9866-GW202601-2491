add_library(bluedroid INTERFACE)
add_library(bluedroid_public)
add_library(bluedroid_private)

target_link_libraries(bluedroid INTERFACE bluedroid_public)
target_link_libraries(bluedroid INTERFACE bluedroid_private)
target_link_libraries(bluedroid_private PRIVATE bluedroid_public)

if ((${CMAKE_PROJECT_NAME} STREQUAL sdk_standard_ngpl)
 OR (${CMAKE_PROJECT_NAME} STREQUAL sdk_standard_ngpl_intl))

    set(CFG_RTL8821BLE y)
    add_definitions(
        -DCFG_GPIO_BT_REG_ON=27
    )
endif()

if (DEFINED CFG_RTL8723BLE)
    set (BT_MODULE RTL8723)
    add_definitions(
        -DCFG_RTL8723BLE
    )
elseif(DEFINED CFG_RTL8733BLE)
    set (BT_MODULE RTL8733)
    add_definitions(
        -DCFG_RTL8733BLE
    )
elseif(DEFINED CFG_RTL8821BLE)
    set (BT_MODULE RTL8821)
    add_definitions(
        -DCFG_RTL8821BLE
    )
elseif(DEFINED CFG_MT7921BLE)
    set (BT_MODULE MT7921)
    add_definitions(
    -DCFG_MT7921BLE
    )
endif()

target_include_directories(bluedroid
INTERFACE
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid
)

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/ar
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/auto_conn
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/av
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/dm
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/gatt
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/hf_client
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/hh
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/jv
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/sdp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/sys
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/pce
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/bip
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/a2dp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/battery
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/dis
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/a2dp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/avrc
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/hid
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/btc/profile/std/smp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/common
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/device
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/external/sbc/decoder
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/external/sbc/encoder
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/hci
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/hci/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/hci/include/hci
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/hci/vendor/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/bta/bip
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/osi
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/a2dp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/avct
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/avdt
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/avrc
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/btm
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/gap
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/gatt
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/include
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/l2cap
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/rfcomm
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/sdp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/stack/smp
    ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/include/api
)

if (DEFINED CFG_RTL8723BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/rtl/src/fwpatches/RTL8723/
    )
elseif (DEFINED CFG_RTL8733BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/rtl/src/fwpatches/RTL8733/
    )
elseif (DEFINED CFG_RTL8821BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/rtl/src/fwpatches/RTL8821/
    )
elseif (DEFINED CFG_AP6256BLE OR DEFINED CFG_AP6236BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/ap6236ble/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/ampak/src/fwpatches/AP6256/
    )
elseif (DEFINED CFG_MT7921BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/include/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/include/sdio/
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/mt7921ble/
		${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/hci/picus/*.c
		${PROJECT_SOURCE_DIR}/sdk/share/bluedroid/hci/boots/*.c
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/fwpatches/MT7921/
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/*.c
    )
endif()

file(GLOB bluedroid_api_SRC
    api/*.c
)

file(GLOB bluedroid_bta_SRC
    bta/ar/*.c
    bta/auto_conn/*.c
    bta/av/*.c
    bta/dm/*.c
    bta/gatt/*.c
    bta/hf_client/*.c
    bta/hh/*.c
    bta/jv/*.c
    bta/sdp/*.c
    bta/sys/*.c
    bta/pce/*.c
    bta/opp_server/*.c
    bta/bip/*.c
)

file(GLOB bluedroid_btc_SRC
    btc/core/*.c
    btc/profile/std/a2dp/*.c
    btc/profile/std/avrc/*.c
    btc/profile/std/battery/*.c
    btc/profile/std/dis/*.c
    btc/profile/std/gap/*.c
    btc/profile/std/gatt/*.c
    btc/profile/std/hf_client/*.c
    btc/profile/std/hid_le/*.c
    btc/profile/std/smp/*.c
    btc/profile/std/spp/*.c
)

file(GLOB bluedroid_device_SRC
    device/*.c
)

file(GLOB bluedroid_external_SRC
    external/sbc/decoder/*.c
    external/sbc/encoder/*.c
)

if (DEFINED CFG_RTL8723BLE)
    file(GLOB bluedroid_hci_SRC
        hci/*.c
        hci/vendor/src/*.c
        hci/vendor/src/RTL8723/*.c
    )
elseif (DEFINED CFG_RTL8733BLE)
    file(GLOB bluedroid_hci_SRC
        hci/*.c
        hci/vendor/src/*.c
        hci/vendor/src/RTL8733/*.c
    )
elseif (DEFINED CFG_RTL8821BLE)
    file(GLOB bluedroid_hci_SRC
        hci/*.c
        hci/vendor/src/*.c
        hci/vendor/src/RTL8821/*.c
    )
elseif (DEFINED CFG_MT7921BLE)
    file(GLOB bluedroid_hci_SRC
        hci/buffer_allocator.c
        hci/hci_audio.c
        hci/hci_hal.c
        hci/mtk_hci_hal/hci_hal_sdio.c
        hci/picus/picus.c
        hci/picus/bt_picus_cli.c
        hci/boots/*.c
        hci/hci_layer.c
        hci/packet_fragmenter.c
        hci/hci_packet_parser.c
        hci/hci_packet_factory.c
    )
endif()

if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE)
    file(GLOB bluetooth_drv_SRC
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/rtl/src/*.c
    )
elseif (DEFINED CFG_MT7921BLE)
    file(GLOB bluetooth_drv_SRC
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/*.c
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/sdio/*.c
    )
endif()

if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE)
    file(GLOB bluetooth_fw
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/rtl/src/fwpatches/${BT_MODULE}/*.c
    )
elseif (DEFINED CFG_MT7921BLE)
    file(GLOB bluetooth_fw
        ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/fwpatches/${BT_MODULE}/*.c
    )
endif()

file(GLOB bluedroid_main_SRC
    main/*.c
)

file(GLOB bluedroid_osi_SRC
    osi/*.c
)

file(GLOB bluedroid_stack_SRC
    stack/a2dp/*.c
    stack/avdt/*.c
    stack/avrc/*.c
    stack/btm/*.c
    stack/btu/*.c
    stack/gap/*.c
    stack/gatt/*.c
    stack/hcic/*.c
    stack/l2cap/*.c
    stack/rfcomm/*.c
    stack/sdp/*.c
    stack/smp/*.c
    stack/obex/*.c
    stack/avct/*.c
)

if (DEFINED CFG_RTL8723BLE)
    file(GLOB bluedroid_hal_SRC
        transport/uart/rtl8723ble/*.c
    )
elseif (DEFINED CFG_AP6256BLE OR DEFINED CFG_AP6236BLE)
    file(GLOB bluedroid_hal_SRC
        transport/uart/ap6236ble/*.c
    )
endif()

target_sources(bluedroid_private
PRIVATE
    ${bluedroid_api_SRC}
    ${bluedroid_bta_SRC}
    ${bluedroid_btc_SRC}
    ${bluedroid_device_SRC}
    ${bluedroid_external_SRC}
    #${bluedroid_hci_SRC}
    ${bluedroid_main_SRC}
    ${bluedroid_osi_SRC}
    ${bluedroid_stack_SRC}
    ${bluetooth_drv_SRC}
    ${bluetooth_fw}
)

add_library(bluedroid_private_archive STATIC IMPORTED)
set_target_properties(bluedroid_private_archive PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/../lib/${CFG_CPU_NAME}/libbluedroid_private.a")
target_link_libraries(bluedroid_private INTERFACE bluedroid_private_archive bluedroid)
target_link_libraries(bluedroid_private_archive INTERFACE bluedroid_public)

target_sources(bluedroid_public
PRIVATE
    ${bluedroid_hci_SRC}
)
