add_library(secure_debug)

target_include_directories(secure_debug
PRIVATE
    src
	${PROJECT_SOURCE_DIR}/sdk/include/
PUBLIC
    inc
)

target_sources(secure_debug
PRIVATE
    src/secure_debug.c
)
