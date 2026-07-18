INCLUDE_DIRECTORIES(
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg/
    ${PROJECT_SOURCE_DIR}/sdk/share/flower
)

LINK_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
)

if (CFG_CHIP_FAMILY STREQUAL 9860)
    if (DEFINED CFG_ITV_VP_HIGH_QUALITY)
        add_library(itv STATIC
            ${PROJECT_SOURCE_DIR}/sdk/include/ite/itv.h
            it9860/itv_high_quality.c
            fc_sync.h
            fc_sync.c
        )
    elseif(DEFINED CFG_TIME_FRAME_FUN_ENABLE)
        add_library(itv STATIC
            ${PROJECT_SOURCE_DIR}/sdk/include/ite/itv.h
            it9860/itv_osd.c
            fc_sync.h
            fc_sync.c
        )
    else()
        add_library(itv STATIC
            ${PROJECT_SOURCE_DIR}/sdk/include/ite/itv.h
            it9860/itv.c
            fc_sync.h
            fc_sync.c
        )
        if (DEFINED CFG_TWO_WAY_VIDEO_ENABLE)
            add_library(itv STATIC
                ${PROJECT_SOURCE_DIR}/sdk/include/ite/itv_2way.h
                it9860/itv_2way.c
            )
        endif()    
    endif()
endif()

target_link_libraries(itv
    isp
)
