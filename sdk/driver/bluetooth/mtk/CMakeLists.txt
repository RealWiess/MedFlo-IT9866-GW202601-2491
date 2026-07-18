if (DEFINED CFG_MT7921BLE)
	set (BT_MODULE MT7921)
endif()

file(GLOB MTK_FW src/fwpatches/${BT_MODULE}/*.c)

include_directories(src/include src/include/sdio ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/mt7921ble ${PROJECT_SOURCE_DIR}/sdk/share/bluedroid)

file(GLOB MTK_SRC src/*.c src/sdio/*.c)

message("MTK_SRC ${MTK_SRC}")

add_library(bluetooth STATIC ${MTK_SRC} ${MTK_FW})