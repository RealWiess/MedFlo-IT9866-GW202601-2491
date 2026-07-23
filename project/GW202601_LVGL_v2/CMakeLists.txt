# GW202601_LVGL_v2 — Clean LVGL project based on SDK lv_demos pattern
# Correct init sequence: itpInit() → lvgl_init() → POST_RESET → display → BACKLIGHT_ON

add_definitions(
    -DLV_CONF_INCLUDE_SIMPLE
)

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

# Stage 1: Use lv_demo_widgets to verify display + backlight
add_definitions(
    -DCFG_DEMO_WIDGETS
)

add_executable(${CMAKE_PROJECT_NAME}
    main_$ENV{CFG_PLATFORM}.c
    main.c
    )

target_link_libraries(${CMAKE_PROJECT_NAME} lvgl_demos)

# build
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
