set(WSS_INCLUDES "wss")
file(GLOB wss_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${WSS_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${wss_files_c}
)
