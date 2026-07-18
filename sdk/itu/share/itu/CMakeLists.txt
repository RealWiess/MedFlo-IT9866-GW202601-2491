add_library(itu)

# affect stdio.h 
add_definitions(
    -DITP_FAT_ACCEL
)

# affect freetype 
add_definitions(
    -DFT2_BUILD_LIBRARY
)

if (DEFINED CFG_ITU_ERR)
    add_definitions(
        -DCFG_ITU_ERR
    )
endif()

if (DEFINED CFG_ITU_WARN)
    add_definitions(
        -DCFG_ITU_WARN
    )
endif()

if (DEFINED CFG_ITU_INFO)
    add_definitions(
        -DCFG_ITU_INFO
    )
endif()

if (DEFINED CFG_ITU_DBG)
    add_definitions(
        -DCFG_ITU_DBG
    )
endif()

if (DEFINED CFG_ITU_ASSERT_THREAD)
    add_definitions(
        -DCFG_ITU_ASSERT_THREAD
    )
endif()

if (DEFINED CFG_ITU_DECOMPRESS_SURFACE_INPLACE)
    add_definitions(
        -DCFG_ITU_DECOMPRESS_SURFACE_INPLACE
    )
endif()

if (DEFINED CFG_ITU_FONT_SURFACE_CACHE)
    add_definitions(
        -DCFG_ITU_FONT_SURFACE_CACHE
    )
endif()

if (DEFINED CFG_NOR_XIP_ITU)
    add_definitions(
        -DCFG_NOR_XIP_ITU_ADDR=${CFG_NOR_XIP_ITU_ADDR}
        -DCFG_NOR_XIP_ITU
    )
endif()

if (DEFINED CFG_ITU_HARFBUZZ_SUPPORT)
    add_definitions(
        -DCFG_ITU_HARFBUZZ_SUPPORT
    )
	
	target_sources(itu
    PRIVATE
        Harfbuzz.c
    )
endif()

if (DEFINED CFG_ITU_FT_CACHE_ENABLE)
    add_definitions(
        -DCFG_ITU_FT_CACHE_SIZE=${CFG_ITU_FT_CACHE_SIZE}
    )

    target_sources(itu
    PRIVATE
        itu_freetype_cache.c
    )

else()
    target_sources(itu
    PRIVATE
        itu_freetype.c
    )
endif()

target_link_libraries(itu PRIVATE freetype)

#if (DEFINED CFG_ITU_LITE_MODE)
#    add_definitions(
#        -DCFG_ITU_LITE_MODE
#    )
#endif()

if (DEFINED CFG_ITU_UCL_ENABLE)
    add_definitions(
        -DCFG_ITU_UCL_ENABLE
    )
endif()

if (DEFINED CFG_ITU_NGPL)
    add_definitions(
        -DCFG_ITU_NGPL
    )
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/include/flower
    ${PROJECT_SOURCE_DIR}/sdk/share/speedy
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
    ${PROJECT_SOURCE_DIR}/sdk/itu/share/itu
)

# Force itu_private to be placed in front of itu in the link order
if ($ENV{CFG_PLATFORM} STREQUAL openrtos)
    add_library(itu_private INTERFACE)
    add_library(itu_private_archive STATIC IMPORTED)
    set_target_properties(itu_private_archive PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/../lib/${CFG_CPU_NAME}/libitu_private.a")
    target_link_libraries(itu_private INTERFACE itu_private_archive itu)
endif()

target_compile_definitions(itu
PRIVATE
    $<$<BOOL:${CFG_BUILD_M2D}>:CFG_BUILD_M2D>
    $<$<BOOL:${CFG_BUILD_JPEG}>:CFG_BUILD_JPEG>
)

target_sources(itu
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/itu/include/ite/itu.h
    itu_action.c
    itu_analogclock.c
    itu_animation.c
    itu_arbitraryfill.c
    itu_audio.c
    itu_background.c
    itu_backgroundbutton.c
    itu_blur.c
    itu_borderwindow.c
    itu_button.c
    itu_calendar.c
    itu_checkbox.c
    itu_cfg.h
    itu_circleprogressbar.c
    itu_clipper.c
    itu_colorpicker.c
    itu_combotable.c
    itu_container.c
    itu_coverflow.c
    itu_curve.c
    itu_effect.c
    itu_digitalclock.c
    itu_drawpen.c
    itu_fadeeffect.c
    itu_filelistbox.c
    itu_flowwindow.c
    itu_icon.c
    itu_iconlistbox.c
    itu_imagecoverflow.c
    itu_jpeg.c
    itu_keyboard.c
    itu_languagesprite.c
    itu_layer.c
    itu_listbox.c
    itu_mediafilelistbox.c
    itu_meter.c
    itu_oscilloscope.c
    itu_png.c
    itu_polygon.c
    itu_popupbutton.c
    itu_popupradiobox.c
    itu_progressbar.c
    itu_radiobox.c
    itu_ripplebackground.c
    itu_scalecoverflow.c
    itu_scaleeffect.c
    itu_scalefadeeffect.c
    itu_scene.c
    itu_scrollbar.c
    itu_scrolleffect.c
    itu_scrollfadeeffect.c
    itu_scrolliconlistbox.c
    itu_scrolllistbox.c
    itu_scrollmediafilelistbox.c
    itu_scrolltext.c
    itu_shadow.c
    itu_spinner.c
    itu_sprite.c
    itu_spritebutton.c
    itu_stepwheel.c
    itu_stopanywhere.c
    itu_stringset.c
    itu_surface.c
    itu_tablebar.c
    itu_tablegrid.c
    itu_tablelistbox.c
    itu_tableiconlistbox.c
    itu_text.c
    itu_textbox.c
    itu_trackbar.c
    itu_utility.c
    itu_wavebackground.c
    itu_wheel.c
    itu_wheelbackground.c
    itu_widget.c
    itu_wipeeffect.c
    lang_str_tab.c
)

if (NOT DEFINED CFG_ITU_NGPL)
    target_sources(itu
    PRIVATE
        itu_video.c
    )
endif()

if (NOT DEFINED WIN32 AND DEFINED CFG_CPU_RISCV)
    set_source_files_properties(
        itu_freetype_cache.c
        PROPERTIES COMPILE_FLAGS -Og)
endif()

target_link_libraries(itu PRIVATE itc itu_renderer png redblack)

target_link_libraries_ifdef(CFG_BUILD_M2D itu PRIVATE m2d)
target_link_libraries_ifdef(CFG_BUILD_FLOWER itu PRIVATE flower)
target_link_libraries_ifdef(CFG_JPEG_HW_ENABLE itu PRIVATE jpg)
target_link_libraries_ifdef(CFG_BUILD_JPEG itu PRIVATE jpeg)
target_link_libraries_ifdef(CFG_VIDEO_ENABLE itu PRIVATE ffmpeg)
if (DEFINED WIN32)
    target_link_libraries(itu PRIVATE ucl)
endif()