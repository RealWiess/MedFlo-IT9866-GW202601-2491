# gw_lvgl_test — LVGL + WiFi(BLE) Gateway
# Pattern: SDK 2491 test_nimble + test_wifi_sdio + lv_demos

# Build bootloader first
if (DEFINED CFG_BOOTLOADER_ENABLE)
    set(PROJECT_NAME_BACKUP ${CMAKE_PROJECT_NAME})
    set(CMAKE_PROJECT_NAME bootloader)
    add_subdirectory(${PROJECT_SOURCE_DIR}/project/bootloader ${CMAKE_CURRENT_BINARY_DIR}/bootloader)
    set(CMAKE_PROJECT_NAME ${PROJECT_NAME_BACKUP})
endif()

add_definitions(
    -DLV_CONF_INCLUDE_SIMPLE
)

# ── WiFi Manager + lwIP MQTT includes (pattern: test_wifi_sdio) ──
include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
    ${PROJECT_SOURCE_DIR}/sdk/include/lwip/apps
)

# ── NimBLE includes (pattern: test_nimble line 2-5) ──
include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/nimble
    ${PROJECT_SOURCE_DIR}/sdk/share/nimble/npl/freertos/include
)

# ── RTL8821 BLE include (pattern: test_nimble line 9-12) ──
if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble
    )
endif()

# ── LVGL includes (pattern: lv_demos) ──
include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/share/lvgl
    ${PROJECT_SOURCE_DIR}/sdk/share/lvgl/demos
)

if ($ENV{CFG_PLATFORM} STREQUAL win32)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/lvgl/win32
    )
else()
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/lvgl/ite_it2d
    )
endif()

# ── Executable (pattern: test_nimble + lv_demos) ──
add_executable(${CMAKE_PROJECT_NAME}
    main_$ENV{CFG_PLATFORM}.c
    main.c
    ui_screen.c
    wifi_app.c
    ble_init.c
    mqtt_app.c
    img_bg_480.c
    logo_nmgw.c
)

# ── SDK auto-links lvgl, it2d, brotli, wifi_mgr, nimble, bluetooth via build.cmake ──
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
