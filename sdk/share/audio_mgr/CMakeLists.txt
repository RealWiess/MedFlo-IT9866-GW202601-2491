add_library(audio_mgr)

if (DEFINED CFG_GCC_LTO)
    target_compile_options(audio_mgr
    PRIVATE
        "-fno-lto"
    )
endif ()

target_include_directories(audio_mgr
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/driver/
PUBLIC
    ${PROJECT_SOURCE_DIR}/sdk/share/audio_mgr/
)

target_sources(audio_mgr
PRIVATE
    audio_mgr.c
    audio_mgr.h
)
