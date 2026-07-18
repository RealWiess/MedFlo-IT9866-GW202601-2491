add_library(audio)

target_include_directories(audio
PRIVATE
    ${CMAKE_BINARY_DIR}/lib/sm32
)

if (DEFINED CFG_AUDIO_ENABLE AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    target_sources(audio
    PRIVATE
        ite_audio.c
        main_processor_message_queue.c
    )

    target_sources_ifdef(CFG_RISC_TS_DEMUX_PLUGIN audio
    PRIVATE
        ite_risc_ts_demux.c
    )

    if (CFG_CHIP_FAMILY EQUAL 9860)
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/arm_lite_codec/lib/fa626/
            DESTINATION ${CMAKE_BINARY_DIR}/lib/sm32
            FILES_MATCHING
            PATTERN "*.hex"
            PATTERN "*.codecs"
        )
    elseif (CFG_CHIP_FAMILY EQUAL 9830)
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/codec/lib/riscv/
            DESTINATION ${CMAKE_BINARY_DIR}/lib/sm32
            FILES_MATCHING
            PATTERN "*.hex"
            PATTERN "*.codecs"
        )
    else()
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/codec/lib/sm32
            DESTINATION ${CMAKE_BINARY_DIR}/lib
            FILES_MATCHING
            PATTERN "*.hex"
            PATTERN "*.codecs"
        )
    endif()

    file(COPY
        ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/codec_ex/lib/sm32
        DESTINATION ${CMAKE_BINARY_DIR}/lib
        FILES_MATCHING PATTERN "*.hex"
    )

else()
    target_sources(audio
    PRIVATE
        ite_audio_null.c
    )
endif()
