add_library(itu_renderer)

target_compile_definitions(itu_renderer
PRIVATE
    $<$<BOOL:${CFG_ITU_ERR}>:CFG_ITU_ERR>
    $<$<BOOL:${CFG_ITU_WARN}>:CFG_ITU_WARN>
    $<$<BOOL:${CFG_ITU_INFO}>:CFG_ITU_INFO>
    $<$<BOOL:${CFG_ITU_DBG}>:CFG_ITU_DBG>
    $<$<BOOL:${CFG_BUILD_M2D}>:CFG_BUILD_M2D>
)

target_include_directories(itu_renderer
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/itu/share/itu_renderer
)

target_sources(itu_renderer
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/itu/include/ite/itu.h
    itu_base.c
    itu_cfg.h
    itu_surface.c
    itu_utility.c
)

if (DEFINED CFG_CHIP_FAMILY_IT970)
    set(dirname it970)
elseif (DEFINED CFG_CHIP_FAMILY_IT9860)
    set(dirname it9860)
elseif (DEFINED CFG_CHIP_FAMILY_IT9830)
    set(dirname it9830)
    target_link_libraries_ifdef(CFG_BUILD_M2D itu_renderer PRIVATE m2d)
elseif (DEFINED CFG_CHIP_FAMILY_IT9870)
    set(dirname it9870)
    target_link_libraries_ifdef(CFG_BUILD_M2D itu_renderer PRIVATE m2d)
elseif (DEFINED CFG_CHIP_FAMILY_IT9880)
    set(dirname it9880)
    target_link_libraries_ifdef(CFG_BUILD_M2D itu_renderer PRIVATE m2d)    
endif()

if (DEFINED CFG_VIDEO_ENABLE OR DEFINED CFG_MJPEG_DEC_ENABLE)
    target_include_directories(itu_renderer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/driver/itv
    )
    if (CFG_VIDEO_FRAME_FUN_ENABLE)
        target_sources(itu_renderer
        PRIVATE
            ${dirname}/itu_framefunc_ff.c
        )
    else()
        if (DEFINED CFG_ITV_VP_HIGH_QUALITY AND DEFINED CFG_CHIP_FAMILY_IT9860)
            target_sources(itu_renderer
            PRIVATE
                ${dirname}/itu_framefunc_hq.c
            )
        else()
            target_sources(itu_renderer
            PRIVATE
                ${dirname}/itu_framefunc.c
            )
        endif()
    endif()
endif()

if (DEFINED CFG_LCD_ENABLE)
    target_sources(itu_renderer
    PRIVATE
        ${dirname}/itu_lcd.c
    )
endif()

if (DEFINED CFG_M2D_ENABLE)
    target_sources(itu_renderer
    PRIVATE
        ${dirname}/itu_m2d.c
        ${dirname}/itu_private.h
    )
else()
    target_sources(itu_renderer
    PRIVATE
        itu_sw.c
    )
endif()

if (DEFINED CFG_ITV_VP_HIGH_QUALITY AND DEFINED CFG_CHIP_FAMILY_IT970)
    target_sources(itu_renderer
    PRIVATE
        ${dirname}/itu_timer_ff.c
    )
endif()

if (DEFINED CFG_TIME_FRAME_FUN_ENABLE AND DEFINED CFG_CHIP_FAMILY_IT9860)
    target_sources(itu_renderer
    PRIVATE
        ${dirname}/itu_timer_ff.c
    )
endif()

if (DEFINED CFG_STNLCD_ENABLE)
    target_sources(itu_renderer
    PRIVATE
        itu_stnlcd.c
   )
endif()
