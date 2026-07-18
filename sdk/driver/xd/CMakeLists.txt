add_library(xd STATIC)

target_include_directories(xd
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/xd
)

target_sources(xd
PRIVATE
    xd.c
    xd_ecc.c
    xd_ecc.c
    xd_binaryheap.c
    xd_mlayer.c
    ite_xd.c
    ${PROJECT_SOURCE_DIR}/sdk/include/xd/xd.h
    ${PROJECT_SOURCE_DIR}/sdk/include/xd/xd_ecc.h
    ${PROJECT_SOURCE_DIR}/sdk/include/xd/xd_binaryheap.h
    ${PROJECT_SOURCE_DIR}/sdk/include/xd/xd_mlayer.h
    #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith.h
    #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith_defs.h
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/ite_xd.h
)