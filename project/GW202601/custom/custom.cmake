set(CUSTOM_INCLUDES "custom")
file(GLOB custom_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${CUSTOM_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${custom_files_c}
)
