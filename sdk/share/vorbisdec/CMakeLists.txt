add_library(vorbisdec)

target_include_directories(vorbisdec
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/ogg
)

target_compile_definitions(vorbisdec
PRIVATE
    _ARM_ASSEM_
    _LOW_ACCURACY_
)

target_include_directories(vorbisdec
PRIVATE
    .
)

target_sources(vorbisdec
PRIVATE
    #bitwise.c
    #framing.c
    block.c
    codebook.c
    floor0.c
    floor1.c
    info.c
    mapping0.c
    mdct.c
    registry.c
    res012.c
    sharedbook.c
    synthesis.c
    vorbisfile.c
    window.c
)

target_link_libraries(vorbisdec
PRIVATE
    ogg
)
