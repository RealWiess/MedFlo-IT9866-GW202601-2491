add_definitions(
    -DMINIZ_NO_TIME
)

include_directories(
    .
    ../inc
    ../src/vector/freetype
    ../src/vector
    ../src/lottie/zip
    ../src/lottie
    ../src/binding/c
)

add_library(rlottie STATIC
    ../src/vector/freetype/v_ft_math.cpp
    ../src/vector/freetype/v_ft_raster.cpp
    ../src/vector/freetype/v_ft_stroker.cpp
    ../src/vector/stb/stb_image.cpp
    ../src/vector/vrect.cpp
    ../src/vector/vdasher.cpp
    ../src/vector/vbrush.cpp
    ../src/vector/vbitmap.cpp
    ../src/vector/vpainter.cpp
    ../src/vector/vdrawhelper_common.cpp
    ../src/vector/vdrawhelper.cpp
    ../src/vector/vdrawhelper_sse2.cpp
    ../src/vector/vdrawhelper_neon.cpp
    ../src/vector/vrle.cpp
    ../src/vector/vpath.cpp
    ../src/vector/vpathmesure.cpp
    ../src/vector/vmatrix.cpp
    ../src/vector/velapsedtimer.cpp
    ../src/vector/vdebug.cpp
    ../src/vector/vinterpolator.cpp
    ../src/vector/vbezier.cpp
    ../src/vector/vraster.cpp
    ../src/vector/vdrawable.cpp
    ../src/vector/vimageloader.cpp
    ../src/vector/varenaalloc.cpp
    ../src/lottie/zip/zip.cpp
    ../src/lottie/lottieitem.cpp
    ../src/lottie/lottieitem_capi.cpp
    ../src/lottie/lottieloader.cpp
    ../src/lottie/lottiemodel.cpp
    ../src/lottie/lottieproxymodel.cpp
    ../src/lottie/lottieparser.cpp
    ../src/lottie/lottieanimation.cpp
    ../src/lottie/lottiekeypath.cpp
    ../src/binding/c/lottieanimation_capi.cpp
)

if (MSVC)
    add_definitions(
        -D__STDC_LIMIT_MACROS
        -DMINIZ_NO_STDIO
    )
    target_compile_options(rlottie
                        PRIVATE
                            /std:c++14
                            /GR- # disable RTTI
                            /W3
                        )
else()
    target_compile_options(rlottie PRIVATE
        -std=c++14
        -fno-exceptions
        -fno-unwind-tables
        -fno-asynchronous-unwind-tables
        -fno-rtti
        -Wall
        -fvisibility=hidden
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wno-unused-parameter>
    )
endif()