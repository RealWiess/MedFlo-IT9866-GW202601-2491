project($ENV{CFG_PROJECT})
cmake_minimum_required(VERSION 3.1)
enable_language(C CXX ASM)

# Generate firmware build timestamp for version tracking
string(TIMESTAMP FW_BUILD_TIME "%Y%m%d_%H%M%S")
add_definitions(-DFW_BUILD_TIME="${FW_BUILD_TIME}")
message(STATUS "Firmware Build Version: ${FW_BUILD_TIME}")

cmake_policy(SET CMP0002 OLD)
cmake_policy(SET CMP0014 OLD)
cmake_policy(SET CMP0079 NEW)

# for supporting non-official cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/tool/share")

include(extensions)
include(global_func)

if (DEFINED CFG_CPU_FA626)
    set(CFG_CPU_NAME fa626)
elseif (DEFINED CFG_CPU_SM32)
    set(CFG_CPU_NAME sm32)
elseif (DEFINED CFG_CPU_RISCV)
    set(CFG_CPU_NAME riscv)
endif()

if (EXISTS ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/config.cmake)
    include(project/${CMAKE_PROJECT_NAME}/config.cmake)
else()
    include(build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/config.cmake)
endif()

if (EXISTS ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/project.cmake)
    include(project/${CMAKE_PROJECT_NAME}/project.cmake)
endif()

include(sdk/def.cmake)
include($ENV{CFG_PLATFORM}/def.cmake)

if (DEFINED CFG_CPU_RISCV)
    include_directories(${PROJECT_SOURCE_DIR}/sdk/cpu/${CFG_CPU_NAME}/inc)
endif()
include_directories(${PROJECT_SOURCE_DIR}/sdk/chip/it${CFG_CHIP_FAMILY}/include)
include_directories(${PROJECT_SOURCE_DIR}/sdk/include)

if ($ENV{CFG_PROJECT} STREQUAL alt_cpu
 OR $ENV{CFG_PROJECT} STREQUAL codec
 OR $ENV{CFG_PROJECT} STREQUAL arm_lite_codec
 OR $ENV{CFG_PROJECT} STREQUAL arm_lite_dev)
    add_subdirectory(project/$ENV{CFG_PROJECT})
else()
    add_subdirectory(sdk)
    add_subdirectory($ENV{CFG_PLATFORM})
    add_subdirectory(project)
endif()
