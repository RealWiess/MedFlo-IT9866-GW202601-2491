add_library(async_file STATIC)

target_include_directories(async_file
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/async_file
)

target_sources(async_file
PRIVATE
    file.c
    init.c
    mutex.c
    msgq.c
    string.c
    thread.c
    error.c
    heap.c
)
