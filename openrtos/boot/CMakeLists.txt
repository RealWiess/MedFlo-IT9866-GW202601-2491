add_library(boot)

target_sources(boot
PRIVATE
    init.c
)

target_link_libraries(boot
PRIVATE
    itp
)

ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BUILD_STL)
if (DEFINED CFG_BUILD_STL)
    target_link_libraries(boot
    PRIVATE
        stl
    )
endif()

if (DEFINED CFG_BUILD_SDL)
    target_link_libraries(boot
    PRIVATE
        sdl_main
    )
endif()

if (DEFINED CFG_LCD_ENABLE)
    if (DEFINED CFG_VIDEO_ENABLE OR DEFINED CFG_LCD_TRIPLE_BUFFER OR (DEFINED CFG_BUILD_LVGL AND DEFINED CFG_ENABLE_ROTATE))
       add_definitions(
           -DCFG_LCDC_HEIGHT=${CFG_LCD_HEIGHT}
           -DCFG_LCDC_PITCH=${CFG_LCD_PITCH}
       )
    else()
        add_definitions(
            -DCFG_LCDC_HEIGHT=0
            -DCFG_LCDC_PITCH=0
            -DCFG_GAP_SIZE=0
        )
   endif()
else()
    add_definitions(
        -DCFG_LCDC_HEIGHT=0
        -DCFG_LCDC_PITCH=0
        -DCFG_GAP_SIZE=0
        -DCFG_LCD_PITCH=0
        -DCFG_LCD_HEIGHT=0
    )
endif()

if (DEFINED CFG_GCC_LTO)
    add_definitions(-fno-lto)
endif()

if (DEFINED CFG_BUILD_IT2D)
    add_definitions(-DCFG_BUILD_IT2D)
endif()

get_directory_property(defs COMPILE_DEFINITIONS)

foreach (def ${defs})
    set(defargs ${defargs} -D${def})
endforeach()

set(defargs ${defargs} -g)

add_custom_target(
    default.ld ALL
    DEPENDS ${LIBRARY_OUTPUT_PATH}/default.ld
)

target_compile_options(boot
PRIVATE
    -g3
)

if (DEFINED CFG_CPU_FA626)

    target_sources(boot
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/fa626/startup.S
        ${CMAKE_CURRENT_LIST_DIR}/fa626/tlb.c
        fa626/basic_op2.s
        fa626/argb2yuv.s
        fa626/atan.s
        fa626/fdsr.s
        fa626/HowlingCtrl.s
        fa626/cfft_64.s
        fa626/cfft_radix_256.s
        fa626/SBST.s
        fa626/mats_algo.s
        fa626/sync.c
    )

    add_custom_command(
        OUTPUT ${LIBRARY_OUTPUT_PATH}/default.ld
        COMMAND ${CPP}
            ${defargs}
            -E
            -P
            -o ${LIBRARY_OUTPUT_PATH}/default.ld
            ${CMAKE_CURRENT_SOURCE_DIR}/fa626/default.lds
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fa626/default.lds
    )
elseif (DEFINED CFG_CPU_RISCV)
    target_sources(boot
    PRIVATE
        riscv/board.c
        riscv/initfini.c
        riscv/reset.c
        riscv/sync.c
        riscv/startup.S
    )

    add_custom_command(
        OUTPUT ${LIBRARY_OUTPUT_PATH}/default.ld
        COMMAND ${CPP}
            ${defargs}
            -I${PROJECT_SOURCE_DIR}/sdk/include
            -E
            -P
            -o ${LIBRARY_OUTPUT_PATH}/default.ld
            ${CMAKE_CURRENT_SOURCE_DIR}/riscv/default.lds
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv/default.lds
    )
endif()
