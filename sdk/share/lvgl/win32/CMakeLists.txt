add_definitions(
    -DLV_CONF_INCLUDE_SIMPLE
)

include_directories(
	${PROJECT_SOURCE_DIR}/sdk/share/rlottie/inc
	${PROJECT_SOURCE_DIR}/sdk/include
	${PROJECT_SOURCE_DIR}/sdk/share/speedy
	${LVGL_ROOT_DIR}/win32
	${LVGL_ROOT_DIR}/src
	${LVGL_ROOT_DIR}
)

file(GLOB_RECURSE SOURCES ${LVGL_ROOT_DIR}/src/*.c ${LVGL_ROOT_DIR}/src/*.cpp)
file(GLOB_RECURSE EXAMPLE_SOURCES ${LVGL_ROOT_DIR}/examples/*.c)
file(GLOB_RECURSE DEMO_SOURCES ${LVGL_ROOT_DIR}/demos/*.c)

add_library(lvgl STATIC ${SOURCES})
add_library(lvgl_examples STATIC ${EXAMPLE_SOURCES})
add_library(lvgl_demos STATIC ${DEMO_SOURCES})

target_compile_definitions(lvgl
PRIVATE
    __STDC_LIMIT_MACROS
    XML_STATIC
)
