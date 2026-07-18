add_library(jpegscale STATIC)

target_include_directories(jpegscale
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/itu/include/
)

target_sources(jpegscale
PRIVATE
    jpegscale.c
    pngscale.c
)