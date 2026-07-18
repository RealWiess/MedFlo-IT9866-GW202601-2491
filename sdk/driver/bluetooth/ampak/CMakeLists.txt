add_library(bluetooth STATIC)

target_include_directories(bluetooth
PRIVATE
    .
)

#set (BT_MODULE AP6255)
if (DEFINED CFG_AP6256BLE)
    set (BT_MODULE AP6256)
elseif(DEFINED CFG_AP6236BLE)
    set (BT_MODULE AP6236)
elseif(DEFINED CFG_AP6212BLE)
    set (BT_MODULE AP6212)
elseif(DEFINED CFG_AP6203BLE)
    set (BT_MODULE AP6203)
endif()

target_sources(bluetooth
PRIVATE
    src/fwpatches/${BT_MODULE}/bt_firmware_image.c
    src/bt_main.c
)