include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/flower/
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
    if (DEFINED CFG_BUILD_NS)
        ${PROJECT_SOURCE_DIR}/sdk/share/ns
    endif()
    if (DEFINED CFG_BUILD_AUDIO_PREPROCESS)
        ${PROJECT_SOURCE_DIR}/sdk/share/audio_preprocess
    endif()
    if (DEFINED CFG_BUILD_MP3DEC)
        ${PROJECT_SOURCE_DIR}/sdk/share/mp3dec
    endif()
    if (DEFINED CFG_BUILD_AACDEC)
        ${PROJECT_SOURCE_DIR}/sdk/share/aacdec
    endif
    if (DEFINED CFG_BUILD_OGG)
        ${PROJECT_SOURCE_DIR}/sdk/share/ogg
        if (DEFINED CFG_BUILD_OPUS)
        ${PROJECT_SOURCE_DIR}/sdk/share/opus
        endif()
        if (DEFINED CFG_BUILD_VORBISDEC)
        ${PROJECT_SOURCE_DIR}/sdk/share/vorbisdec
        endif()
    endif()
)

add_library(flower)

if (DEFINED CFG_GCC_LTO)
    target_compile_options(flower
    PRIVATE
        "-fno-lto"
    )
endif()

target_sources(flower
PRIVATE
    filter_chain.c
    example/filterA.c
    example/filterB.c
    example/filterC.c
    example/filterD.c
    example/filterE.c
    example/filterF.c
    filter_des.c
    ite_queue.c
    ite_buffer.c
    flower_stream.c
)

if (DEFINED CFG_NET_ENABLE)
    target_sources(flower
    PRIVATE
        network/filter_udp.c
        network/filter_tcp.c
        network/Rcurl.c
    )
    target_link_libraries(flower
    PRIVATE
        curl
        zlib
    )
endif()

if (DEFINED CFG_VIDEO_ENABLE)
    target_sources(flower
    PRIVATE
        flow_videostream.c
        video/filter_h264dec.c
        video/filter_mjpegdec.c
        video/filter_display.c
        video/filter_nv12_jpgenc.c
        video/filter_jpegwriter.c
        video/filter_filewriter.c
        video/filter_rec_avi.c
        video/filter_mjpeg_rec_avi.c
        video/ite_avienc.h
        video/ite_avienc.c
        video/filter_ipcam.h
        video/filter_ipcam.c
        video/filter_display_dual.c
    )
endif()

if (DEFINED CFG_CAPTURE_ENABLE)
    target_sources(flower
    PRIVATE
        video/filter_capture.c
    )
endif()

if (DEFINED CFG_TIME_FRAME_FUN_ENABLE)
    target_sources(flower
    PRIVATE
        video/filter_display_osd.c
    )
endif()

if (DEFINED CFG_UVC_ENABLE)
    target_sources(flower
    PRIVATE
        video/filter_uvc.c
    )
endif()

if (DEFINED CFG_AUDIO_ENABLE)
    target_sources(flower
    PRIVATE
        flow_audiostream.c
        audio/IteFlowctrl.c
        audio/audiofilterflow.c
        audio/amusefilterflow.c
        audio/adspfilterflow.c
        audio/audioqueue.c
        audio/volumeapi.c
        audio/streamapi.c
        #audio/tickapi.c
        audio/Ffileplay.c
        audio/Ffileplaymgr.c
        audio/Ffilerec.c
        audio/Fcastor3snd.c
        audio/Fvoid.c
        audio/Ftee.c
        audio/Fchanadapt.c
        audio/Fqlay.c
        #audio/Fmixer.c
        audio/Floopback.c
        audio/Fqwrite.c
        audio/Fqread.c
        audio/Fcodecmgr.c
        audio/audioparsing.c
    )
endif()

if (DEFINED CFG_BUILD_SPEEX)
    target_sources(flower
    PRIVATE
        audio/Fresample.c
        #audio/Fkissfft.c
    )

    target_link_libraries(flower
    PRIVATE
        speex
    )
endif()

if (DEFINED CFG_BUILD_AUDIO_PREPROCESS)
    target_sources(flower
    PRIVATE
        audio/Faec.c
        audio/Fdenoise.c
        audio/Fstretch.c
    )

    target_link_libraries(flower
    PRIVATE
        eigens
        voipdsp
    )
endif()

if (DEFINED CFG_BUILD_ASR)
    target_sources(flower
    PRIVATE
        audio/asrfilterflow.c
    )

    if (DEFINED CFG_BUILD_WFSTASR)
        target_sources(flower
        PRIVATE
            audio/Fasr.c
        )
        target_link_libraries(flower
        PRIVATE
            wfstasr
            chains
        )
    else()
        target_sources(flower
        PRIVATE
            audio/Fasrex.c
        )
    endif()
    target_link_libraries(flower
    PRIVATE
        asr
    )
endif()

if (DEFINED CFG_AV_MIX_FILTER)
    target_sources(flower
    PRIVATE
        audio/avfilterflow.c
        audio/Fpcmpool.c
    )
endif()

if (DEFINED CFG_BUILD_FFMPEG)
    target_sources(flower
    PRIVATE
        audio/Ffileplaym4a.c
        audio/Fffmpeggrab.c
        audio/Fdem4a.c
    )
    target_link_libraries(flower
    PRIVATE
        ffmpeg
    )
endif()

if (DEFINED CFG_BUILD_MP3DEC)
    target_sources(flower
    PRIVATE
        audio/Ffileplaymp3.c
        audio/Fcodecmp3.c
    )
    target_link_libraries(flower
    PRIVATE
        mp3dec
    )
endif()

if (DEFINED CFG_BUILD_OPUS)
    target_sources(flower
    PRIVATE
        audio/Ffileplayopus.c
        audio/Fcodecopus.c
        #audio/Fopus.c
    )
    target_link_libraries(flower
    PRIVATE
        opus
    )
endif()

if (DEFINED CFG_BUILD_VORBISDEC)
    target_sources(flower
    PRIVATE
        audio/Ffileplayvorbis.c
    )
    target_link_libraries(flower
    PRIVATE
        vorbisdec
    )
endif()

if (DEFINED CFG_I2S_HDMI_RX_CLK)
    target_sources(flower
    PRIVATE
        audio/Fcastor3trx.c
    )
endif()
if (DEFINED CFG_BUILD_AACDEC)
    target_sources(flower
    PRIVATE
    audio/Faacgrab.c
    audio/Fcodecaac.c
)
endif()

if (DEFINED CFG_RESERVE_FILTER)
    target_sources(flower
    PRIVATE
        reserve/flow_utils.c
        reserve/flow_reservstream.c
        reserve/flow_soundpool.c
        reserve/flow_multistream.c
        reserve/Zamusefilerflow.c
        reserve/Zpoolfilterflow.c
        reserve/Fmultimix.c
        reserve/Ffdplay.c
    )
endif()

if (DEFINED CFG_FADE_FILTER)
    target_sources(flower
    PRIVATE
    audio/Ffade.c
)
endif()
