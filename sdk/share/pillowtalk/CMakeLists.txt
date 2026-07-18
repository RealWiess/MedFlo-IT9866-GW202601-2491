add_library(pillowtalk STATIC)

target_include_directories(pillowtalk
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/yajl/
)

set(PILLOWTALK_MAJOR 0)
set(PILLOWTALK_MINOR 3)
set(PILLOWTALK_MICRO 0)

set(pillowtalk_SRCS pillowtalk_impl.c)
set(pillowtalk_HDRS pillowtalk.h pillowtalk_impl.h)

target_sources(pillowtalk
PRIVATE
    ${pillowtalk_SRCS}
    ${pillowtalk_HDRS}
)

#### setup shared library version number
set_target_properties(pillowtalk PROPERTIES
                      SOVERSION ${PILLOWTALK_MAJOR}
                      VERSION ${PILLOWTALK_MAJOR}.${PILLOWTALK_MINOR}.${PILLOWTALK_MICRO})

target_link_libraries(pillowtalk
PRIVATE
    ${YAJL_LIBRARY}
    ${CURL_LIBRARY}
)

# Output Paths
set (output_include ${CMAKE_CURRENT_BINARY_DIR}/../include)
file(MAKE_DIRECTORY ${output_include})

# Copy Public .h's into the output directory so the tests can get it
#SET (pillowtalk_PUB_HDRS pillowtalk.h)
#FOREACH (header ${pillowtalk_PUB_HDRS})
#  SET (header ${CMAKE_CURRENT_SOURCE_DIR}/${header})

#  EXEC_PROGRAM(${CMAKE_COMMAND} ARGS -E copy_if_different ${header} ${output_include})

#ENDFOREACH (header ${pillowtalk_PUB_HDRS})

# Installation Step
#INSTALL( TARGETS pillowtalk LIBRARY DESTINATION lib)
#INSTALL( FILES pillowtalk.h DESTINATION include)
