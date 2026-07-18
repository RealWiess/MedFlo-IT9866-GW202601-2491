add_library(m2d STATIC)

if (DEFINED CFG_M2D_MEMPOOL_ENABLE)
    target_compile_definitions(m2d
    PRIVATE
        CFG_M2D_MEMPOOL_ENABLE
        CFG_M2D_MEMPOOL_SIZE=${CFG_M2D_MEMPOOL_SIZE}
    )
endif()

target_include_directories(m2d
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/gfx
    ${PROJECT_SOURCE_DIR}/sdk/share/nedmalloc
)

target_sources(m2d
PRIVATE
    driver.c
    surface.c
    hw.c
    gfx_mem.c
    gfx_math.c
)

if (DEFINED CFG_BUILD_DEWARP)
    target_compile_definitions(m2d
    PRIVATE
        CFG_BUILD_DEWARP
    )
    
    target_include_directories(m2d
        PRIVATE
        .
        ${PROJECT_SOURCE_DIR}/sdk/include/dewarp
    )
    
    target_link_libraries(m2d
        PRIVATE
            dewarp
    )
endif()
