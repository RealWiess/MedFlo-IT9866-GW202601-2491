add_library(openrtos)

target_link_libraries(openrtos PRIVATE malloc)

target_include_directories(openrtos
PRIVATE
    ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/include/openrtos
    ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/include/openrtos/${CFG_CPU_NAME}
)

if (DEFINED CFG_GCC_LTO)
    set_source_files_properties( croutine.c
                                 event_groups.c
                                 list.c
                                 queue.c
                                 tasks.c
                                 timers.c
                                 stream_buffer.c
                                 PROPERTIES COMPILE_FLAGS -fno-lto )
endif ()

target_sources(openrtos
PRIVATE
    croutine.c
    event_groups.c
    list.c
    queue.c
    tasks.c
    timers.c
    stream_buffer.c
)

if (DEFINED CFG_CPU_FA626)

    if (DEFINED CFG_GCC_LTO)
        set_source_files_properties( fa626/portISR.c PROPERTIES COMPILE_FLAGS -fno-lto )
    endif ()

    target_sources(openrtos
    PRIVATE
        fa626/port.c
        fa626/portISR.c
    )

elseif (DEFINED CFG_CPU_SM32)

    if (DEFINED CFG_GCC_LTO)
        set_source_files_properties( sm32/portasm.S PROPERTIES COMPILE_FLAGS -fno-lto )
    endif ()

    if (DEFINED CFG_CPU_LITTLE_ENDIAN)
        set_source_files_properties( sm32/portasm.S PROPERTIES COMPILE_FLAGS -mlittle-endian )
    endif ()

    target_sources(openrtos
    PRIVATE
        sm32/port.c
        sm32/portasm.S
    )
elseif (DEFINED CFG_CPU_RISCV)

    if (DEFINED CFG_GCC_LTO)
        set_source_files_properties( riscv/portasm.S PROPERTIES COMPILE_FLAGS "-mcpu=d25f -fno-lto" )
    else()
        set_source_files_properties( riscv/portasm.S PROPERTIES COMPILE_FLAGS "-mcpu=d25f" )
    endif()

    target_sources(openrtos
    PRIVATE
        riscv/freertos_tick_config.c
        riscv/port.c
        riscv/portasm.S
        riscv/trap.c
    )
endif()

if (${CFG_OPENRTOS_HEAP_SIZE} STRGREATER "0")
    target_sources(openrtos
    PRIVATE
        heap_4.c
    )
else()
    target_sources(openrtos
    PRIVATE
        heap_3.c
    )
endif()

if (DEFINED CFG_DBG_TRACE_ANALYZER AND DEFINED CFG_DBG_TRACE)
    target_sources(openrtos
    PRIVATE
        trcKernelPort.c
        trcSnapshotRecorder.c
        trcStreamingRecorder.c
    )
endif()

if (DEFINED CFG_OPENRTOS_CLI OR DEFINED CFG_ENABLE_UART_CLI)
    target_sources(openrtos
    PRIVATE
        FreeRTOS_CLI.c
    )
endif()
