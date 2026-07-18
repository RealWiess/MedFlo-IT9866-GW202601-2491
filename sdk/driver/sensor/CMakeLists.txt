add_library(sensor STATIC)

target_include_directories(sensor
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/sensor
)

target_sources(sensor
PRIVATE
    mmp_sensor.c
    adv7180.c
    adv7280a.c
    adv7181c.c
    TVP5150AM1.c
    NT99141.c
    RN6752.c
    RN6854M.c
    TP9950.c
    TP2825B.c
    TP2860.c
    PR2000K.c    
    MAX9276.c
    DS90UB926Q.c
    userdefine.c
)