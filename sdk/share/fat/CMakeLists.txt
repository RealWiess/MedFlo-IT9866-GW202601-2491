include_directories(
    ${CMAKE_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/sdk/include/fat
    .
)

if (NOT DEFINED WIN32)
    add_library(fat INTERFACE)
    add_library(fat_archive STATIC IMPORTED)
    set_target_properties(fat_archive PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/../lib/${CFG_CPU_NAME}/libfat.a")
    target_link_libraries(fat INTERFACE fat_archive)
endif()

if (DEFINED CFG_SD0_ENABLE OR DEFINED CFG_SD1_ENABLE)
    add_library(fat_sd STATIC
        mmcsd/ite_mmcsd.c
        mmcsd/mmcsd_cache2.c
    )
    target_include_directories(fat_sd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mmcsd)
endif()

if (DEFINED CFG_MS_ENABLE)
    add_library(fat_mspro STATIC
        mspro/ite_mspro_fat.c
    )
endif()

if (DEFINED CFG_MSC_ENABLE)
    add_library(fat_msc STATIC
        msc/ite_msc_fat.c
    )
endif()

if (DEFINED CFG_XD_ENABLE)
    add_library(fat_xd STATIC
        xd/fat_xddrv.c
    )
endif()

if (DEFINED CFG_NOR_ENABLE)
    add_library(fat_nor STATIC
        nor/cache2.c
        nor/cache2.h
        nor/nordrv_f.c
        nor/nordrv_f.h
    )
endif()

if (DEFINED CFG_RAMDISK_ENABLE)
    add_library(fat_ramdisk STATIC
        ram/ramdrv_f.c
        ram/ramdrv_f.h
    )
endif()
