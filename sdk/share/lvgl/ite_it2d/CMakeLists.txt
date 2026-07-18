add_definitions(
    -DLV_CONF_INCLUDE_SIMPLE
)

include_directories(
	${PROJECT_SOURCE_DIR}/sdk/driver/it2d
  ${PROJECT_SOURCE_DIR}/sdk/driver/itdcps
  ${PROJECT_SOURCE_DIR}/sdk/driver/itdpu
  ${PROJECT_SOURCE_DIR}/sdk/driver/itjpeg
  ${PROJECT_SOURCE_DIR}/sdk/driver/itvp
	${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
	${PROJECT_SOURCE_DIR}/sdk/share/rlottie/inc
	${PROJECT_SOURCE_DIR}/sdk/include
	${LVGL_ROOT_DIR}/ite_it2d
	${LVGL_ROOT_DIR}/src
	${LVGL_ROOT_DIR}
)

if (DEFINED CFG_BUILD_ITDCPS)
    add_definitions(
        -DCONFIG_ITE_ITDCPS
    )
endif()

if (DEFINED CFG_BUILD_ITDPU)
    add_definitions(
        -DCONFIG_ITE_ITDPU
    )
endif()

if (DEFINED CFG_BUILD_ITJPEG)
    add_definitions(
        -DCONFIG_ITE_ITJPEG
    )
endif()

if (DEFINED CFG_BUILD_ITVP)
    add_definitions(
        -DCONFIG_ITE_ITVP
    )
endif()

file(GLOB_RECURSE SOURCES ${LVGL_ROOT_DIR}/src/*.c ${LVGL_ROOT_DIR}/src/*.cpp)
file(GLOB_RECURSE EXAMPLE_SOURCES ${LVGL_ROOT_DIR}/examples/*.c)
file(GLOB_RECURSE DEMO_SOURCES ${LVGL_ROOT_DIR}/demos/*.c)
file(GLOB_RECURSE IT2D_SOURCES ${LVGL_ROOT_DIR}/ite_it2d/*.c)

add_library(lvgl STATIC ${SOURCES} ${IT2D_SOURCES})
add_library(lvgl_examples STATIC ${EXAMPLE_SOURCES})
add_library(lvgl_demos STATIC ${DEMO_SOURCES})

target_compile_options(lvgl
PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>
)