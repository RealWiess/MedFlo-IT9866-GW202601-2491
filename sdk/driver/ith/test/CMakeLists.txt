add_executable(test_ith_utility)

target_include_directories(test_ith_utility
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include
)

target_link_libraries(test_ith_utility
PRIVATE
    GTest::gtest_main
)

# Detect if the host system is linux
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    #target_compile_options(test_ith_utility PRIVATE -fsanitize=address)
    #target_link_options(test_ith_utility PRIVATE -fsanitize=address)
endif()

target_sources(test_ith_utility
PRIVATE
    test_ith_list.cc
    test_ith_memset.cc
    test_ith_u32tob.cc
    ../ith_list.c
    ../ith_utility.c
    fake_ithprint.c
)

gtest_discover_tests(test_ith_utility)
