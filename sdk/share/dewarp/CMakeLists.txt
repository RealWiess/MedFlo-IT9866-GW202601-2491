add_library(dewarp)

if (DEFINED CFG_GCC_LTO)
    target_compile_options(dewarp
    PRIVATE
        "-fno-lto"
    )
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/include/dewarp
    )
file(COPY libhudmath.a DESTINATION ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME})

target_sources(dewarp
PRIVATE
    dewarp.c
)

target_link_libraries(dewarp
    PRIVATE
        hudmath
    )