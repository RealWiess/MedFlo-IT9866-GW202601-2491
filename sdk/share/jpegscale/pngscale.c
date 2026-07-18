#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "png.h"
#include "jpegscale/jpegscale.h"
#include "isp/mmp_isp.h"
#include "ite/itu.h"
#include "ite/itp.h"

//=============================================================================
//                Global Data Definition
//=============================================================================
typedef enum
{
    PNG_RGB565   = 0, ///< RGB565 format
    PNG_ARGB8888 = 1, ///< ARGB8888 format
} PngFormat;

static ISP_DEVICE gIspDev = NULL;

static void       RGBScaleToYUV (float widthPercent, float heightPercent,
                                 JPEGSCALE_TEMP src, JPEGSCALE_TEMP dest,
                                 PngFormat src_format, JPEGSCALE_FORMAT format,
                                 uint32_t src_width, uint32_t src_height,
                                 uint32_t src_header_width,
                                 uint32_t src_header_height, uint32_t dst_width,
                                 uint32_t dst_height, uint32_t dst_header_width,
                                 uint32_t dst_header_height)
{
    uint32_t            result      = 0;
    MMP_ISP_OUTPUT_INFO outInfo     = {0};
    MMP_ISP_SHARE       ispInput    = {0};
    MMP_ISP_CORE_INFO   ISPCOREINFO = {0};
    int                 dispWidth, dispHeight = 0;

    dispWidth             = ((dst_header_width + 3) / 4) * 4;
    dispHeight            = ((dst_header_height + 3) / 4) * 4;

    ispInput.width        = src_width;
    ispInput.height       = src_height;
    ispInput.isAdobe_CMYK = 0;

    if (src_format == PNG_RGB565)
    {
        ispInput.format = MMP_ISP_IN_RGB565;
        ispInput.addrY  = (uint32_t)src;
        ispInput.pitchY = src_width * 2;
    }
    else if (src_format == PNG_ARGB8888)
    {
        ispInput.format = MMP_ISP_IN_RGB888;
        ispInput.addrY  = (uint32_t)src;
        ispInput.pitchY = src_width * 4;
    }

    result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
    if (result)
    {
        printf("mmpIspInitialize() error (0x%x) !!\n", (unsigned int)result);
    }

    // for VP1
    ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
    // end of for VP1.

    result                  = mmpIspSetCore(gIspDev, &ISPCOREINFO);
    if (result)
    {
        printf("mmpIspSetCore() error (0x%x) !!\n", (unsigned int)result);
    }

    result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
    if (result)
    {
        printf("mmpIspSetMode() error (0x%x) !! \n", (unsigned int)result);
    }

    outInfo.startX = 0;
    outInfo.startY = 0;
    outInfo.width  = dst_width;
    outInfo.height = dst_height;
    if (format == JPEGSCALE_FORMAT_YUV422)
    {
        outInfo.addrRGB = (uint32_t)dest;
        outInfo.addrY   = (uint32_t)dest;
        outInfo.addrU   = (uint32_t)dest + dst_width * dst_height;
        outInfo.addrV   = (uint32_t)dest + dst_width * dst_height +
                        dst_width * dst_height / 2;
        outInfo.format = MMP_ISP_OUT_YUV422;
    }
    else if (format == JPEGSCALE_FORMAT_YUV420)
    {
        outInfo.addrRGB = (uint32_t)dest;
        outInfo.addrY   = (uint32_t)dest;
        outInfo.addrU   = (uint32_t)dest + dst_width * dst_height;
        outInfo.addrV   = (uint32_t)dest + dst_width * dst_height +
                        dst_width * dst_height / 4;
        outInfo.format = MMP_ISP_OUT_YUV420;
    }
    outInfo.pitchRGB = (uint16_t)dst_width;
    outInfo.pitchY   = dst_width;
    outInfo.pitchUv  = dst_width / 2;

    mmpIspSetOutputWindow(gIspDev, &outInfo);
    mmpIspSetVideoWindow(gIspDev, 0, 0, dispWidth, dispHeight);

#if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_lock(&ISP_CORE_0_MUTEX);
#endif

    result = mmpIspPlayImageProcess(gIspDev, &ispInput);
    if (result)
    {
        printf("mmpIspPlayImageProcess() error (0x%x) !!\n",
               (unsigned int)result);
    }

    result = mmpIspWaitEngineIdle(gIspDev);
    if (result)
    {
        printf("mmpIspWaitEngineIdle() error (0x%x) !!\n",
               (unsigned int)result);
    }

#if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
#endif

    mmpIspTerminate(&gIspDev);
}

static ITUSurface * PngLoadFile (char * filepath, JPEGSCALE_TEMP * temp,
                                 uint32_t * width, uint32_t * height,
                                 uint32_t * header_width,
                                 uint32_t * header_height, PngFormat * format)
{
    ITUSurface * dest          = NULL;
    FILE *       fp            = NULL;
    uint16_t *   temp_rgb565   = NULL;
    uint32_t *   src_argb8888  = NULL;
    uint32_t *   dest_argb8888 = NULL;
    png_structp  png_ptr       = NULL;
    png_infop    info_ptr      = NULL;
    png_bytepp   row_pointers  = NULL;
    int          bit_depth = 0, color_type = 0;
    png_uint_32  temp_width = 0U, temp_height = 0U;
    uint32_t     x, y;
    png_uint_32  result;

    fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        goto end;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        printf("%s: png_create_read_struct returned 0.\n", filepath);
        goto end;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        printf("%s: png_create_info_struct returned 0.\n", filepath);
        goto end;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        printf("%s: error from libpng.\n", filepath);
        goto end;
    }

    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr,
                 PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
                     PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR,
                 NULL);
    result = png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height,
                          &bit_depth, &color_type, NULL, NULL, NULL);
    if (result == 0)
    {
        goto end;
    }

    printf("%s: %lux%lu %d\n", filepath, temp_width, temp_height,
           (unsigned int)color_type);

    if (bit_depth != 8)
    {
        printf("%s: Unsupported bit depth %d.  Must be 8.\n", filepath,
               bit_depth);
        goto end;
    }

    switch (color_type)
    {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_RGB:
            *format = PNG_RGB565;
            break;

        case PNG_COLOR_TYPE_RGB_ALPHA:
            *format = PNG_ARGB8888;
            break;

        default:
            printf("%s: Unknown libpng color type %d.\n", filepath, color_type);
            goto end;
    }

    *header_width  = temp_width;
    *header_height = temp_height;

    *width         = ((temp_width + 3) / 4) * 4;
    *height        = ((temp_height + 3) / 4) * 4;

    row_pointers   = png_get_rows(png_ptr, info_ptr);

    if (*format == PNG_RGB565)
    {
        dest = ituCreateSurface((int)(*width), (int)(*height), 0, ITU_RGB565,
                                NULL, 0);
        temp_rgb565 = (uint16_t *)ituLockSurface(dest, 0, 0, (int)(*width),
                                                 (int)(*height));

        if (color_type == PNG_COLOR_TYPE_GRAY)
        {
            for (y = 0; y < (*height); y++)
            {
                for (x = 0; x < (*width); x++)
                {
                    if (y < (*header_height) && x < (*header_width))
                    {
                        uint8_t * row = (uint8_t *)row_pointers[y];
                        temp_rgb565[x + y * (*width)] =
                            ITH_RGB565(row[x], row[x], row[x]);
                    }
                    else
                    {
                        temp_rgb565[x + y * (*width)] = 0x0;
                    }
                }
            }
        }
        else
        {
            for (y = 0; y < (*height); y++)
            {
                for (x = 0; x < (*width); x++)
                {
                    if (y < (*header_height) && x < (*header_width))
                    {
                        uint8_t * row = (uint8_t *)row_pointers[y];
                        temp_rgb565[x + y * (*width)] = ITH_RGB565(
                            row[x * 3 + 2], row[x * 3 + 1], row[x * 3 + 0]);
                    }
                    else
                    {
                        temp_rgb565[x + y * (*width)] = 0x0;
                    }
                }
            }
        }
        *temp = (JPEGSCALE_TEMP)temp_rgb565;
    }
    else
    {
        ITUColor     white = {0, 0xFF, 0xFF, 0xFF};
        ITUSurface * src   = ituCreateSurface((int)(*width), (int)(*height), 0,
                                              ITU_ARGB8888, NULL, 0);
        src_argb8888 = (uint32_t *)ituLockSurface(src, 0, 0, (int)(*width),
                                                  (int)(*height));

        for (y = 0; y < (*height); y++)
        {
            for (x = 0; x < (*width); x++)
            {
                if (y < (*header_height) && x < (*header_width))
                {
                    uint32_t * row = (uint32_t *)row_pointers[y];
                    src_argb8888[x + y * (*width)] = row[x];
                }
                else
                {
                    src_argb8888[x + y * (*width)] = 0x0;
                }
            }
        }

        dest = ituCreateSurface((int)(*width), (int)(*height), 0, ITU_ARGB8888,
                                NULL, 0);
        dest_argb8888 = (uint32_t *)ituLockSurface(dest, 0, 0, (int)(*width),
                                                   (int)(*height));
        ituColorFill(dest, 0, 0, (int)(*width), (int)(*height), &white);
        ituAlphaBlend(dest, 0, 0, (int)(*width), (int)(*height), src, 0, 0,
                      0xFF);
        *temp = (JPEGSCALE_TEMP)dest_argb8888;

        ituUnlockSurface(src);
        ituDestroySurface(src);
    }

end:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    if (fp)
    {
        fclose(fp);
    }

    return dest;
}
//=============================================================================
//                Public Function Definition
//=============================================================================
JPEGSCALE_RESULT
PngScaleToJpeg(char * inputFile, char * outputFile, float widthPercent,
               float heightPercent, JPEGSCALE_FORMAT format)
{
    JPEGSCALE_RESULT result = JPEGSCALE_SUCCESS;
    JPEGSCALE_TEMP   temp;
    ITUSurface *     surf = NULL;
    PngFormat        png_format;
    uint8_t *        dest  = NULL;
    uint32_t         width = 0, height = 0;
    uint32_t         header_width = 0, header_height = 0;
    uint32_t         scale_width = 0, scale_height = 0;
    uint32_t         scale_header_width = 0, scale_header_height = 0;

    if (!inputFile || !outputFile || widthPercent <= 0 || heightPercent <= 0 ||
        format > JPEGSCALE_FORMAT_YUV420)
    {
        printf("[%s] parameter fail\n", __func__);
        result = JPEGSCALE_ERR_INVALID_INPUT_PARAM;
        goto end;
    }

    surf = PngLoadFile(inputFile, &temp, &width, &height, &header_width,
                       &header_height, &png_format);
    if (surf == NULL)
    {
        goto end;
    }

    scale_width =
        (((uint32_t)(header_width * widthPercent / 100) + 15) / 16) * 16;
    scale_height =
        (((uint32_t)(header_height * heightPercent / 100) + 15) / 16) * 16;
    scale_header_width  = (uint32_t)(header_width * widthPercent / 100);
    scale_header_height = (uint32_t)(header_height * heightPercent / 100);

    dest = (uint8_t *)itpVmemAlignedAlloc(32, scale_width * scale_height * 2);
    if (dest == NULL)
    {
        printf("[%s] itpVmemAlignedAlloc fail\n", __func__);
        goto end;
    }
    RGBScaleToYUV(widthPercent, heightPercent, temp, dest, png_format, format,
                  width, height, header_width, header_height, scale_width,
                  scale_height, scale_header_width, scale_header_height);

    if ((result = JpegSaveFile(outputFile, format, dest, scale_width,
                               scale_height, scale_header_width,
                               scale_header_height)) != JPEGSCALE_SUCCESS)
    {
        printf("[%s] JpegSaveFile fail, result=%d\n", __func__, result);
        goto end;
    }

end:
    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    if (dest != NULL)
    {
        itpVmemFree((uint32_t)dest);
    }

    return result;
}
