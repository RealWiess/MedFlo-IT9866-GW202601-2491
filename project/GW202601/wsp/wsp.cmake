set(WSP_INCLUDES "wsp")
file(GLOB wsp_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${WSP_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${wsp_files_c}
)
