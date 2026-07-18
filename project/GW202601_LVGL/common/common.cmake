set(COMMON_INCLUDES "common")
file(GLOB common_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${COMMON_INCLUDES}/*.c")
file(GLOB common_files_h "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${COMMON_INCLUDES}/*.h")
add_executable(${CMAKE_PROJECT_NAME}
${common_files_c}
${common_files_h}
)