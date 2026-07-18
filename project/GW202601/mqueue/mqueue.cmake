set(MQUEUE_INCLUDES "mqueue")
file(GLOB mqueue_files_c "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${MQUEUE_INCLUDES}/*.c")
add_executable(${CMAKE_PROJECT_NAME}
    ${mqueue_files_c}
)
