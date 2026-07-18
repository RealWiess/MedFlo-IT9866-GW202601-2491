add_library(freetype STATIC)

target_compile_definitions(freetype
PRIVATE
    FT2_BUILD_LIBRARY
    ITP_FAT_ACCEL
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(freetype
    PRIVATE
        DEBUG
    )
endif()

target_include_directories(freetype
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/share/brotli/c/include
)

target_sources(freetype
PRIVATE
    #autofit/autofit.c
    base/ftbase.c
    base/ftbbox.c
    #base/ftbdf.c
    base/ftbitmap.c
    base/ftcolor.c
    base/ftdebug.c
    base/fterrors.c
    #base/ftgasp.c
    base/ftgloadr.c
    base/ftglyph.c
    #base/ftgxval.c
    base/ftinit.c
    base/ftlcdfil.c
    base/ftfntfmt.c
    base/ftmm.c
    #base/ftotval.c
    #base/ftpatent.c
    #base/ftpfr.c
    base/ftstroke.c
    base/ftsynth.c
    base/ftsystem.c
    base/fttype1.c
    #base/ftwinfnt.c
    #bdf/bdf.c
    cache/ftcache.c
    cff/cff.c
    #cid/type1cid.c
    #gxvalid/gxvalid.c
    gzip/ftgzip.c
    #lzw/ftlzw.c
    #otvalid/otvalid.c
    #pcf/pcf.c
    #pfr/pfr.c
    psaux/psaux.c
    pshinter/pshinter.c
    #psnames/psnames.c
    raster/raster.c
    sfnt/sfnt.c
    smooth/smooth.c
    truetype/truetype.c
    #type1/type1.c
    #type42/type42.c
    #winfonts/winfnt.c
)