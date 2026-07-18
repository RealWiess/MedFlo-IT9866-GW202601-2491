add_library(sdl_ttf STATIC)

target_include_directories(sdl_ttf
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/SDL
)

target_sources(sdl_ttf
PRIVATE
    SDL_ttf.c
)