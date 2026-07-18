add_library(uart)

target_link_libraries(uart
PRIVATE
    itp_boot
    ith
    openrtos
)

target_include_directories(uart
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/uart
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(uart
PRIVATE
    uart.c
    uart_intr.c
    uart_dma.c
    uart_fifo.c
)

if (CFG_UART0_RS485 OR CFG_UART1_RS485
 OR CFG_UART2_RS485 OR CFG_UART3_RS485
 OR CFG_UART4_RS485 OR CFG_UART5_RS485)
    target_sources(uart
    PRIVATE
        interface/rs485.c
    )
endif()
