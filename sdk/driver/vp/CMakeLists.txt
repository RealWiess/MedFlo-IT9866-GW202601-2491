add_library(vp STATIC)

target_include_directories(vp
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/vp
    ${PROJECT_SOURCE_DIR}/sdk/include/encoder
)

target_sources(vp
PRIVATE
    vp.c
    vp_hw.c
    vp_util.c
    mmp_vp.c
)