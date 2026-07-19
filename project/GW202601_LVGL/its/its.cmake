set(ITS_INCLUDES "its")
file(GLOB its_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${ITS_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${its_files_c}
)
