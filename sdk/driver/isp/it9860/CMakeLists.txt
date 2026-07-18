add_library(isp STATIC)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(isp
    PRIVATE
        DEBUG
    )
endif()

target_include_directories(isp
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/driver/ith/
    ${PROJECT_SOURCE_DIR}/sdk/include/isp
)

target_sources(isp
PRIVATE
    isp.c
    isp_hw.c
    isp_hw_op.c
    isp_queue.c
    isp_util.c
    mmp_isp.c
)
