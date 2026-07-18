add_subdirectory(driver)
if (DEFINED CFG_CPU_RISCV)
    add_subdirectory(cpu)
endif()

add_subdirectory(share)
add_subdirectory(itu/share)
