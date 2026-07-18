add_library(onvif STATIC)

target_compile_options_ifdef(CFG_GCC_LTO onvif
PRIVATE
    "-fno-lto"
)

target_include_directories(onvif
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/onvif/
    ${PROJECT_SOURCE_DIR}/sdk/include/
)

target_sources(onvif
PRIVATE
    onvif.c
)
