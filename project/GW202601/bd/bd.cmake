set(BD_INCLUDES "bd")
file(GLOB bd_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${BD_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${bd_files_c}
)
