if (DEFINED CFG_ARMLITE_ENABLE)

    add_subdirectory_ifdef(CFG_ARMLITE_OPUS_CODEC   opuscodec)
    add_subdirectory_ifdef(CFG_ARMLITE_HUD_BG_FAST  hudBgFast)
    add_subdirectory_ifdef(CFG_ARMLITE_CANBUS       can_bus_lite)
    add_subdirectory_ifdef(CFG_ARMLITE_LCD_CRC_CAL  lcdCrcCal)

endif()
