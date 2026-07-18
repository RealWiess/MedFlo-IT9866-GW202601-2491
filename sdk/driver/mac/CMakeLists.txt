add_library(mac)

target_include_directories(mac
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/driver/mac/inc/
    ${PROJECT_SOURCE_DIR}/sdk/driver/mac/gmac/
)

target_sources(mac
PRIVATE
    mii.c
    gmac/skb.c
    gmac/ite_gmac.c
)
