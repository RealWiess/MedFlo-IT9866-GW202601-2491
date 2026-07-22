# gw_lvgl_test — Pure SDK lv_demos pattern with Gateway LCD config
# No ITU, no SDL, no WiFi, no BLE — just LVGL + IT2D display

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

include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/share/lvgl
    ${PROJECT_SOURCE_DIR}/sdk/share/lvgl/demos
    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr
    ${PROJECT_SOURCE_DIR}/sdk/include
    ${PROJECT_SOURCE_DIR}/sdk/include/nimble
    ${PROJECT_SOURCE_DIR}/sdk/share/nimble/npl/freertos/include
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

add_executable(${CMAKE_PROJECT_NAME}
    main_$ENV{CFG_PLATFORM}.c
    main.c
    ui_screen.c
    wifi_app.c
    ble_init.c
    img_bg_480.c
    logo_nmgw.c
    )

target_link_libraries(${CMAKE_PROJECT_NAME} lvgl lvgl_demos it2d brotli)

# build
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
