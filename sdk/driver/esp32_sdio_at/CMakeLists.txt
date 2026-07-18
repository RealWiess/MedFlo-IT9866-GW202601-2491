add_library(esp32_sdio_at STATIC)

if (NOT DEFINED CFG_MMC_ENABLE)
    target_include_directories(esp32_sdio_at
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include
    )
endif()

target_sources(esp32_sdio_at
PRIVATE
    esp32_io.c
)
