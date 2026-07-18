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

if (DEFINED CFG_DEMO_WIDGETS)
    add_definitions(
        -DCFG_DEMO_WIDGETS
    )
elseif (DEFINED CFG_DEMO_BENCHMARK)
    add_definitions(
        -DCFG_DEMO_BENCHMARK
    )
elseif (DEFINED CFG_DEMO_STRESS)
    add_definitions(
        -DCFG_DEMO_STRESS
    )
elseif (DEFINED CFG_DEMO_KEYPAD_AND_ENCODER)
    add_definitions(
        -DCFG_DEMO_KEYPAD_AND_ENCODER
    )
elseif (DEFINED CFG_DEMO_MUSIC)
    add_definitions(
        -DCFG_DEMO_MUSIC
    )
elseif (DEFINED CFG_DEMO_RENDER)
    add_definitions(
        -DCFG_DEMO_RENDER
    )
elseif (DEFINED CFG_DEMO_VECTOR_GRAPHIC)
    add_definitions(
        -DCFG_DEMO_VECTOR_GRAPHIC
    )
endif()

add_executable(${CMAKE_PROJECT_NAME}
    main_$ENV{CFG_PLATFORM}.c
    main.c
    )

target_link_libraries(${CMAKE_PROJECT_NAME} lvgl_demos)

# build
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
