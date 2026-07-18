add_library(iniparser STATIC)

target_include_directories(iniparser
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include
    ${PROJECT_SOURCE_DIR}/sdk/include/iniparser
)

target_sources(iniparser
PRIVATE
    dictionary.c
    iniparser.c
)