add_library(sdl_image STATIC)

target_compile_definitions(sdl_image
PRIVATE
    LOAD_BMP
    LOAD_JPG
    LOAD_PNG
)

target_include_directories(sdl_image
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/SDL
    ${PROJECT_SOURCE_DIR}/sdk/include
)

target_sources(sdl_image
PRIVATE
    IMG.c
    IMG_bmp.c
    IMG_gif.c
    IMG_ite_jpg.c
    IMG_jpg.c
    IMG_lbm.c
    IMG_pcx.c
    IMG_png.c
    IMG_pnm.c
    IMG_tga.c
    IMG_tif.c
    IMG_xcf.c
    IMG_xpm.c
    IMG_xv.c
    IMG_webp.c
)