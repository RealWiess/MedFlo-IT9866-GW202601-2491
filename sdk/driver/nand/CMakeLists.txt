if (DEFINED CFG_NAND_ENABLE)
    INCLUDE_DIRECTORIES(
        ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc
    )

    if (DEFINED CFG_PPI_NAND)
        add_library(nand STATIC
            nf_lb_hwecc.c
            nfdrv.c
            ite_nand.c
            power_down_handle.c
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/configs.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/nf_reg.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/nf_lb_hwecc.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/nfdrv.h
            ${PROJECT_SOURCE_DIR}/sdk/include/ite/ite_nand.h
            #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith_defs.h
            #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith.h
        )
    elseif (DEFINED CFG_SPI_NAND)
        add_library(spi_nand STATIC
            spinfdrv.c
            nf_spi_nand.c
            nfdrv.c
            ite_nand.c
            power_down_handle.c
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/configs.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/spinfdrv.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/nf_spi_nand.h
            ${PROJECT_SOURCE_DIR}/sdk/driver/nand/inc/nfdrv.h
            ${PROJECT_SOURCE_DIR}/sdk/include/ite/ite_nand.h
            #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith_defs.h
            #${PROJECT_SOURCE_DIR}/sdk/include/ite/ith.h
        )
        if (NOT CFG_NAND_READ_CACHE_SIZE STREQUAL 0)
            add_library(spi_nand STATIC
                cache.c
            )
        endif()
        if (DEFINED CFG_SPI_NAND_USE_AXISPI)
            target_link_libraries(spi_nand PRIVATE axispi)
        else()
            target_link_libraries(spi_nand PRIVATE spi)
        endif()
    endif()
endif()
