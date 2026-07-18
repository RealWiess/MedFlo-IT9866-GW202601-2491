add_library(i2s STATIC)

if (DEFINED CFG_HDMI_TX_ENABLE)
    target_sources(i2s
    PRIVATE
        i2s_9860_HDMITX.c
        i2s_switch_GPIO.c
        i2s_reg_9860.h
    )
else()
	if (DEFINED CFG_I2S_FOR_BT)
		target_sources(i2s
		PRIVATE
			i2s_9860_bt.c
			i2s_switch_GPIO.c
			i2s_reg_9860.h
		)
	else()
		target_sources(i2s
		PRIVATE
			i2s_9860.c
			i2s_switch_GPIO.c
			i2s_reg_9860.h
		)
	endif()
endif()