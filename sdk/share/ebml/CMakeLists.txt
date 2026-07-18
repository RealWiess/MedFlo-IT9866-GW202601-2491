add_library(ebml STATIC)

target_include_directories(ebml
PRIVATE
    .
)

target_sources(ebml
PRIVATE
    ebmlbinary.c
    ebmlcrc.c
    ebmlcrc.h
    ebmldate.c
    ebmlelement.c
    ebmlmain.c
    ebmlmaster.c
    ebmlnumber.c
    ebmlstring.c
    ebmlvoid.c
)