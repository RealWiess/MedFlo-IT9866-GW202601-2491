add_definitions(
    -DLV_CONF_INCLUDE_SIMPLE
)

include_directories(
	${PROJECT_SOURCE_DIR}/sdk/driver/i2s
	${PROJECT_SOURCE_DIR}/sdk/share/flower
	${PROJECT_SOURCE_DIR}/sdk/include
    ${PROJECT_SOURCE_DIR}/sdk/include/flower
	${LVGL_ROOT_DIR}/ite_itaudio
	${LVGL_ROOT_DIR}/src
	${LVGL_ROOT_DIR}
)

add_library(lvgl_audio)

if (DEFINED CFG_AUDIO_ENABLE)
    if(DEFINED CFG_MIXENABLE)
    target_sources(lvgl_audio PRIVATE
        lvgl_ite_multiaudio.c
    )    
    else()
    target_sources(lvgl_audio PRIVATE
        lvgl_ite_itaudio.c
    )    
    endif()


    target_link_libraries(lvgl_audio
    PRIVATE
        flower
    )
endif()

#file(GLOB_RECURSE ITAUDIO_SOURCES ${LVGL_ROOT_DIR}/ite_itaudio/*.c)
#add_library(lvgl_audio STATIC ${SOURCES} ${ITAUDIO_SOURCES})
