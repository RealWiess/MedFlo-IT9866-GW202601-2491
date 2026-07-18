set(LAYER_INCLUDES "layer")
file(GLOB layer_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${LAYER_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${layer_files_c}
)
