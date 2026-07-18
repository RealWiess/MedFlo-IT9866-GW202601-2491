#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef CFG_BUILD_JPEG
    #include "jpeglib.h"
#endif
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#include "ite/itp.h"
#include "ite/itv.h" //test

#if defined(CFG_JPEG_HW_ENABLE)

    #include "jpg/ite_jpg.h"
    #if (CFG_CHIP_FAMILY != 9830) && (CFG_CHIP_FAMILY != 9870) && (CFG_CHIP_FAMILY != 9880)
        #include "isp/mmp_isp.h"
    #endif
    #define MAX_JPEG_DECODE_SIZE 36000000
    #define JPEG_SOF_MARKER      0xFFC0U
    #define JPEG_SOS_MARKER      0xFFDAU
    #define JPEG_DHT_MARKER      0xFFC4U
    #define JPEG_DRI_MARKER      0xFFDDU
    #define JPEG_DQT_MARKER      0xFFDBU
    #define JPEG_APP00_MARKER    0xFFE0U
    #define JPEG_APP01_MARKER    0xFFE1U
    #define JPEG_APP02_MARKER    0xFFE2U
    #define JPEG_APP03_MARKER    0xFFE3U
    #define JPEG_APP04_MARKER    0xFFE4U
    #define JPEG_APP05_MARKER    0xFFE5U
    #define JPEG_APP06_MARKER    0xFFE6U
    #define JPEG_APP07_MARKER    0xFFE7U
    #define JPEG_APP08_MARKER    0xFFE8U
    #define JPEG_APP09_MARKER    0xFFE9U
    #define JPEG_APP10_MARKER    0xFFEAU
    #define JPEG_APP11_MARKER    0xFFEBU
    #define JPEG_APP12_MARKER    0xFFECU
    #define JPEG_APP13_MARKER    0xFFEDU
    #define JPEG_APP14_MARKER    0xFFEEU
    #define JPEG_APP15_MARKER    0xFFEFU
    #define JPEG_COM_MARKER      0xFFFEU

//=============================================================================
//				  Constant Definition
//=============================================================================
typedef enum DATA_COLOR_TYPE_TAG
{
    DATA_COLOR_YUV444,
    DATA_COLOR_YUV422,
    DATA_COLOR_YUV422R,
    DATA_COLOR_YUV420,
    DATA_COLOR_ARGB8888,
    DATA_COLOR_ARGB4444,
    DATA_COLOR_RGB565,
    DATA_COLOR_NV12,
    DATA_COLOR_NV21,

    DATA_COLOR_CNT,
} DATA_COLOR_TYPE;

//=============================================================================
//				  Macro Definition
//=============================================================================

//=============================================================================
//				  Structure Definition
//=============================================================================
typedef struct _BASE_RECT_TAG
{
    int x;
    int y;
    int w;
    int h;
} BASE_RECT;

//=============================================================================
//				  Global Data Definition
//=============================================================================

    #if (CFG_CHIP_FAMILY != 9830) && (CFG_CHIP_FAMILY != 9870) && (CFG_CHIP_FAMILY != 9880)
static ISP_DEVICE gIspDev;
    #endif
extern ITUSurface * VideoSurf[3];

//=============================================================================
//				  Private Function Definition
//=============================================================================
    #if (CFG_CHIP_FAMILY == 9830) || (CFG_CHIP_FAMILY == 9870) || (CFG_CHIP_FAMILY == 9880)
static void argb8888toyuv420 (char * yuv_dst, char * argb_src, int width,
                              int height)
{
    uint8_t *     yuvBuf  = yuv_dst;
    int           nWidth  = width;
    int           nHeight = height;

    int           i, j;
    uint8_t *     bufY = yuvBuf;
    uint8_t *     bufU = yuvBuf + nWidth * nHeight;
    uint8_t *     bufV = bufU + (nWidth * nHeight * 1 / 4);

    uint8_t *     bufRGB;
    unsigned char y, u, v, r, g, b;

    if (NULL == argb_src)
    {
        return;
    }

    for (j = 0; j < nHeight; j++)
    {
        bufRGB = argb_src + nWidth * 4 * j;
        for (i = 0; i < nWidth; i++)
        {
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);
            bufRGB++; // alpha
            y = (unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            v = (unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            u = (unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            *(bufY++) = ITH_MAX(0, ITH_MIN(y, 255));

            if (j % 2 == 0 && i % 2 == 0)
            {
                if (u > 255)
                {
                    u = 255;
                }
                if (u < 0)
                {
                    u = 0;
                }
                *(bufU++) = u;
            }
            else
            {
                if (i % 2 == 0)
                {
                    if (v > 255)
                    {
                        v = 255;
                    }
                    if (v < 0)
                    {
                        v = 0;
                    }
                    *(bufV++) = v;
                }
            }
        }
    }
}

static void rgb888toyuv420 (char * yuv_dst, char * rgb_src, int width,
                            int height)
{
    uint8_t *     yuvBuf  = yuv_dst;
    int           nWidth  = width;
    int           nHeight = height;

    int           i, j;
    uint8_t *     bufY = yuvBuf;
    uint8_t *     bufU = yuvBuf + nWidth * nHeight;
    uint8_t *     bufV = bufU + (nWidth * nHeight * 1 / 4);

    uint8_t *     bufRGB;
    unsigned char y, u, v, r, g, b;

    if (NULL == rgb_src)
    {
        return;
    }

    for (j = 0; j < nHeight; j++)
    {
        bufRGB = rgb_src + nWidth * 3 * j;
        for (i = 0; i < nWidth; i++)
        {
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);
            y = (unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            v = (unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            u = (unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            *(bufY++) = ITH_MAX(0, ITH_MIN(y, 255));

            if (j % 2 == 0 && i % 2 == 0)
            {
                if (u > 255)
                {
                    u = 255;
                }
                if (u < 0)
                {
                    u = 0;
                }
                *(bufU++) = u;
            }
            else
            {
                if (i % 2 == 0)
                {
                    if (v > 255)
                    {
                        v = 255;
                    }
                    if (v < 0)
                    {
                        v = 0;
                    }
                    *(bufV++) = v;
                }
            }
        }
    }
}
    #else
static void ISP_RGBtoYUV420 (uint8_t * dst, uint8_t * src, int width,
                             int height, ITUPixelFormat format)
{
    int                 result      = 0;
    MMP_ISP_OUTPUT_INFO outInfo     = {0};
    MMP_ISP_SHARE       ispInput    = {0};
    MMP_ISP_CORE_INFO   ISPCOREINFO = {0};

    ispInput.width                  = width;
    ispInput.height                 = height;
    ispInput.addrY                  = (uint32_t)src;
    if (format == ITU_ARGB8888)
    {
        ispInput.pitchY = width * 4;
        ispInput.format = MMP_ISP_IN_RGB888;
    }
    else if (format == ITU_RGB565)
    {
        ispInput.pitchY = width * 2;
        ispInput.format = MMP_ISP_IN_RGB565;
    }

    outInfo.addrRGB  = (uint32_t)dst;
    outInfo.addrY    = (uint32_t)dst;
    outInfo.addrU    = (uint32_t)dst + width * height;
    outInfo.addrV    = (uint32_t)dst + width * height + width * height / 4;
    outInfo.width    = width;
    outInfo.height   = height;
    outInfo.pitchRGB = width;
    outInfo.pitchY   = width;
    outInfo.pitchUv  = width / 2;
    outInfo.format   = MMP_ISP_OUT_YUV420;

    result           = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
    if (result)
    {
        (void)printf("mmpIspInitialize() error (0x%x) !!\n", result);
    }

    ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
    result                  = mmpIspSetCore(gIspDev, &ISPCOREINFO);
    if (result)
    {
        printf("mmpIspSetCore() error (0x%x) !!\n", result);
    }
    result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
    if (result)
    {
        printf("mmpIspSetMode() error (0x%x) !! \n", result);
    }
    mmpIspSetOutputWindow(gIspDev, &outInfo);
    mmpIspSetVideoWindow(gIspDev, 0, 0, outInfo.width, outInfo.height);

    result = mmpIspPlayImageProcess(gIspDev, &ispInput);
    if (result)
    {
        printf("mmpIspPlayImageProcess() error (0x%x) !!\n", result);
    }
    result = mmpIspWaitEngineIdle(gIspDev);
    if (result)
    {
        printf("mmpIspWaitEngineIdle() error (0x%x) !!\n", result);
    }
    mmpIspTerminate(&gIspDev);
}

static void set_isp_colorTrans (uint8_t * srcAddr_rgby, uint8_t * srcAddr_u,
                         uint8_t * srcAddr_v, DATA_COLOR_TYPE colorType,
                         BASE_RECT * srcRect, JPG_RECT * destRect,
                         int imgWidth, int imgHeight,
                         int M2dPitch, uint8_t * dest, MMP_BOOL bVPOutputRGB888)
{
    int                 result = 0;
    int                 width = 0, height = 0;
    MMP_ISP_OUTPUT_INFO outInfo  = {0};
    MMP_ISP_SHARE       ispInput = {0};
        #if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    MMP_ISP_CORE_INFO ISPCOREINFO = {0};
        #endif

    // ispInput.width        = (imgWidth >> 3) << 3;  //srcRect->w;
    // ispInput.height       = (imgHeight >> 3) << 3; //srcRect->h;
    ispInput.width        = imgWidth;
    ispInput.height       = imgHeight;
    ispInput.isAdobe_CMYK = 0;

    switch (colorType)
    {
        case DATA_COLOR_YUV444:
        case DATA_COLOR_YUV422:
        case DATA_COLOR_YUV422R:
        case DATA_COLOR_YUV420:
            ispInput.addrY = (uint32_t)srcAddr_rgby;
            ispInput.addrU = (uint32_t)srcAddr_u;
            ispInput.addrV = (uint32_t)srcAddr_v;
            switch (colorType)
            {
                case DATA_COLOR_YUV444:
                    ispInput.format  = MMP_ISP_IN_YUV444;
                    ispInput.pitchY  = srcRect->w;
                    ispInput.pitchUv = srcRect->w;
                    break;
                case DATA_COLOR_YUV422:
                    ispInput.format  = MMP_ISP_IN_YUV422;
                    ispInput.pitchY  = srcRect->w;
                    ispInput.pitchUv = (srcRect->w >> 1);
                    break;
                case DATA_COLOR_YUV422R:
                    ispInput.format  = MMP_ISP_IN_YUV422R;
                    ispInput.pitchY  = srcRect->w;
                    ispInput.pitchUv = srcRect->w;
                    break;
                case DATA_COLOR_YUV420:
                    ispInput.format  = MMP_ISP_IN_YUV420;
                    ispInput.pitchY  = srcRect->w;
                    ispInput.pitchUv = (srcRect->w >> 1);
                    break;
            }
            break;
        case DATA_COLOR_NV12:
        case DATA_COLOR_NV21:
            ispInput.addrY   = (uint32_t)srcAddr_rgby;
            ispInput.addrU   = (uint32_t)srcAddr_u;
            ispInput.pitchY  = srcRect->w;
            ispInput.pitchUv = srcRect->w;
            ispInput.format  = (colorType == DATA_COLOR_NV12) ? MMP_ISP_IN_NV12
                                                              : MMP_ISP_IN_NV21;
            break;

        case DATA_COLOR_RGB565:
        case DATA_COLOR_ARGB8888:
        case DATA_COLOR_ARGB4444:
            return;
    }

        #if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    // mmpIspTerminate(&gIspDev);
    width  = destRect->w; // dispWidth;
    height = destRect->h; // dispHeight;

            #if (CFG_CHIP_FAMILY == 970)
                #ifdef CFG_LCD_PQ_TUNING
    result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_1);
                #else
    result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
                #endif
            #else
    result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
            #endif

    if (result)
    {
        (void)printf("mmpIspInitialize() error (0x%x) !!\n", result);
    }

    // for VP1
    ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
    // end of for VP1.

    result                  = mmpIspSetCore(gIspDev, &ISPCOREINFO);
    if (result)
    {
        (void)printf("mmpIspSetCore() error (0x%x) !!\n", result);
    }

    result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
    if (result)
    {
        (void)printf("mmpIspSetMode() error (0x%x) !! \n", result);
    }

    outInfo.startX   = 0;
    outInfo.startY   = 0;
    outInfo.width    = width;
    outInfo.height   = height;
    outInfo.addrRGB  = (uint32_t)dest;
    outInfo.pitchRGB = (uint16_t)M2dPitch;
    if (bVPOutputRGB888)
    {
        outInfo.format = MMP_ISP_OUT_RGB888;
    }
    else
    {
        outInfo.format = MMP_ISP_OUT_DITHER565A;
    }

    mmpIspSetOutputWindow(gIspDev, &outInfo);
    mmpIspSetVideoWindow(gIspDev, 0, 0, outInfo.width, outInfo.height);

            #if (CFG_CHIP_FAMILY != 970)
                // #ifdef CFG_LCD_PQ_TUNING
                #if defined(CFG_LCD_PQ_TUNING) ||                              \
                    defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_lock(&ISP_CORE_0_MUTEX);
                #endif
            #endif

    result = mmpIspPlayImageProcess(gIspDev, &ispInput);
    if (result)
    {
        (void)printf("mmpIspPlayImageProcess() error (0x%x) !!\n", result);
    }

    result = mmpIspWaitEngineIdle(gIspDev);
    if (result)
    {
        (void)printf("mmpIspWaitEngineIdle() error (0x%x) !!\n", result);
    }

            #if (CFG_CHIP_FAMILY != 970)
                // #ifdef CFG_LCD_PQ_TUNING
                #if defined(CFG_LCD_PQ_TUNING) ||                              \
                    defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
                #endif
            #endif

    mmpIspTerminate(&gIspDev);
        #endif
}
    #endif
    //=============================================================================
    //				  Public Function Definition
    //=============================================================================
int ituJpegLoadEx (int width, int height, uint8_t * data, int size)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint32_t        SmallPicWidth = 1280U, SmallPicHeight = 720U;
    uint8_t *       pY = NULL, *pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;
    uint8_t *       dbuf        = NULL;
    ITV_DBUF_PROPERTY dbufprop  = {0};
    int               new_index = 0;

    // malloc_stats();

    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return 0;
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while ((pCur < pEnd))
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }

    if (width == 0 || height == 0)
    {
        width  = (int)imgWidth;
        height = (int)imgHeight;
    }

    new_index = itv_get_vidSurf_index();
    while (new_index == -1)
    {
        // (void)printf("wait to get new_index!\n");
        (void)usleep(1000);
        new_index = itv_get_vidSurf_index();
    }

    switch (new_index)
    {
        case 0:
            new_index = 1;
            break;

        case 1:
        case -2:
            new_index = 0;
            break;
    }

    surf = VideoSurf[new_index];
    dest = (uint8_t *)ituLockSurface(surf, 0, 55, width, height);
    // ituColorFill(surf, 0, 55, width, height, &black);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        return 0;
    }
    else if ((imgWidth * imgHeight) <= (SmallPicWidth * SmallPicHeight))
    {
        initParam.codecType = JPG_CODEC_DEC_JPG_CMD;
        initParam.decType   = JPG_DEC_PRIMARY;
        initParam.dispMode  = JPG_DISP_CENTER; // JPG_DISP_FIT;
        initParam.outColorSpace =
            JPG_COLOR_SPACE_YUV420; // just a initial value. not really the
                                    // picture`s  format.JPG CMD mode back will
                                    // modify it.
        initParam.width  = width;
        initParam.height = height;
        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

        inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
        inStreamInfo.streamType            = JPG_STREAM_MEM;
        inStreamInfo.jpg_reset_stream_info = 0;

        inStreamInfo.validCompCnt          = 1;
        inStreamInfo.jstream.mem[0].pAddr  = data;
        inStreamInfo.jstream.mem[0].length = size;
        iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
        result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
        if (result == JPG_ERR_JPROG_STREAM)
        {
            (void)printf("JPG not support this format\n");
            goto end;
        }

        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        (void)printf("memory mode  (%d, %d) %ldx%ld, dispMode=%d\r\n",
                     jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
                     jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
                     initParam.dispMode);

        real_width  = jpgUserInfo.real_width;
        real_height = jpgUserInfo.real_height;

        if (initParam.outColorSpace == JPG_COLOR_SPACE_RGB565)
        {
            outStreamInfo.jstream.mem[0].pAddr =
                (uint8_t *)dest; // get output buf;
            outStreamInfo.jstream.mem[0].pitch  = surf->pitch;
            outStreamInfo.jstream.mem[0].length = surf->pitch * surf->height;
            outStreamInfo.validCompCnt          = 1;
        }
        else
        {
            pY = malloc(real_width * real_height * 3);
            // (void)memset(pY, 0x0, real_width * real_height * 3);

            // Y
            outStreamInfo.jstream.mem[0].pAddr  = (uint8_t *)pY;
            outStreamInfo.jstream.mem[0].pitch  = jpgUserInfo.comp1Pitch;
            outStreamInfo.jstream.mem[0].length = real_width * real_height;
            // U
            outStreamInfo.jstream.mem[1].pAddr =
                (uint8_t *)(outStreamInfo.jstream.mem[0].pAddr +
                            outStreamInfo.jstream.mem[0].length);
            outStreamInfo.jstream.mem[1].pitch = jpgUserInfo.comp23Pitch;
            outStreamInfo.jstream.mem[1].length =
                outStreamInfo.jstream.mem[1].pitch * real_height;
            // V
            outStreamInfo.jstream.mem[2].pAddr =
                (uint8_t *)(outStreamInfo.jstream.mem[1].pAddr +
                            outStreamInfo.jstream.mem[1].length);
            outStreamInfo.jstream.mem[2].pitch = jpgUserInfo.comp23Pitch;
            outStreamInfo.jstream.mem[2].length =
                outStreamInfo.jstream.mem[2].pitch * real_height;
            outStreamInfo.validCompCnt = 3;
        }
    }
    else
    {
        initParam.codecType     = JPG_CODEC_DEC_JPG;
        initParam.decType       = JPG_DEC_PRIMARY;
        initParam.outColorSpace = JPG_COLOR_SPACE_RGB565;
        initParam.width         = width;
        initParam.height        = height;
        initParam.dispMode      = JPG_DISP_CENTER; // JPG_DISP_FIT;
        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

        inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
        inStreamInfo.streamType            = JPG_STREAM_MEM;
        inStreamInfo.jpg_reset_stream_info = 0;

        inStreamInfo.validCompCnt          = 1;
        inStreamInfo.jstream.mem[0].pAddr  = data;
        inStreamInfo.jstream.mem[0].length = size;

        result = iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
        if (result != JPG_ERR_OK)
        {
            (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
            goto end;
        }
        result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
        if (result == JPG_ERR_JPROG_STREAM)
        {
            (void)printf("JPG not support this format\n");
            goto end;
        }
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        /*
           (void)printf("handshake mode  (%d, %d) %dx%d, dispMode=%d\r\n",
               jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
               jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
               initParam.dispMode);
         */
        outStreamInfo.jstream.mem[0].pAddr = (uint8_t *)dest; // get output buf;
        outStreamInfo.jstream.mem[0].pitch = surf->pitch;
        outStreamInfo.jstream.mem[0].length =
            surf->pitch * surf->height; // outStreamInfo.jstream.mem[0].pitch *
                                        // jpgUserInfo.jpgRect.h;
        outStreamInfo.validCompCnt = 1;
    }
    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    if (pY != NULL)
    {
        ithFlushDCacheRange((void *)pY, real_width * real_height * 3);
        ithFlushMemBuffer();
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    iteJpg_DestroyHandle(&pHJpeg, 0);

    if ((imgWidth * imgHeight) <= (SmallPicWidth * SmallPicHeight))
    {
        srcRect.w            = real_width;
        srcRect.h            = real_height;

        switch (jpgUserInfo.colorFormate)
        {
            case JPG_COLOR_SPACE_YUV411:
                colorType = DATA_COLOR_YUV422;
                break;
            case JPG_COLOR_SPACE_YUV444:
                colorType = DATA_COLOR_YUV444;
                break;
            case JPG_COLOR_SPACE_YUV422:
                colorType = DATA_COLOR_YUV422;
                break;
            case JPG_COLOR_SPACE_YUV420:
                colorType = DATA_COLOR_YUV420;
                break;
            case JPG_COLOR_SPACE_YUV422R:
                colorType = DATA_COLOR_YUV422R;
                break;
            case JPG_COLOR_SPACE_RGB565:
                colorType = DATA_COLOR_RGB565;
                break;
        }

    #if (CFG_CHIP_FAMILY != 9830) && (CFG_CHIP_FAMILY != 9870) && (CFG_CHIP_FAMILY != 9880)
        set_isp_colorTrans(outStreamInfo.jstream.mem[0].pAddr,
                           outStreamInfo.jstream.mem[1].pAddr,
                           outStreamInfo.jstream.mem[2].pAddr, colorType,
                           &srcRect, &destRect, imgWidth, imgHeight,
                           surf->pitch, dest, 0);
    #endif
    }

    while ((dbuf = itv_get_dbuf_anchor()) == NULL)
    {
        // (void)printf("wait to get dbuf!\n");
        (void)usleep(1000);
    }

    {
        // ------------------------------------
        // Just through itv driver to Flip LCD ,both handshake mode or command
        // trigger mode run this setting (when command trigger
        // ,MMP_ISP_IN_RGB565 is not really format.).
        dbufprop.src_w    = 0;
        dbufprop.src_h    = 0;
        dbufprop.pitch_y  = 0;
        dbufprop.pitch_uv = 0;
    #if (CFG_CHIP_FAMILY != 9830) && (CFG_CHIP_FAMILY != 9870) && (CFG_CHIP_FAMILY != 9880)
        dbufprop.format = MMP_ISP_IN_RGB565;
    #endif
        dbufprop.ya   = 0;
        dbufprop.ua   = 0;
        dbufprop.va   = 0;
        dbufprop.bidx = 0;
        // (void)printf("dbufprop.ya=0x%x,dbufprop.ua=0x%x,dbufprop.va=0x%x,dbufprop.src_w=%d,dbufprop.src_h=%d,dbufprop.pitch_y=%d,dbufprop.pitch_uv=%d,dbufprop.format=%d\n",dbufprop.ya,dbufprop.ua,dbufprop.va,dbufprop.src_w,dbufprop.src_h,dbufprop.pitch_y,dbufprop.pitch_uv,dbufprop.format);
        itv_update_dbuf_anchor(&dbufprop);
    }

    if (pY != NULL)
    {
        free(pY);
    }

    // (void)printf("jpeg decode end\n");
    ituUnlockSurface(surf);
    return 1;

end:
    iteJpg_DestroyHandle(&pHJpeg, 0);
    ituUnlockSurface(surf);
    return 1;
}

static uint8_t * _ituReadFileToBuffer (const char * filepath, int * outSize)
{
    FILE *      f      = NULL;
    uint8_t *   buffer = NULL;
    struct stat sb;
    int         fileSize = 0;

    if ((filepath == NULL) || (outSize == NULL))
    {
        return NULL;
    }

    f = fopen(filepath, "rb");
    if (f == NULL)
    {
        perror("fopen failed");
        return NULL;
    }

    if (fstat(fileno(f), &sb) == -1)
    {
        perror("fstat failed");
        fclose(f);
        return NULL;
    }

    fileSize = sb.st_size;
    if (fileSize < 2)
    { // Basic check
        fprintf(stderr, "File too small or empty: %s\n", filepath);
        fclose(f);
        return NULL;
    }

    buffer = malloc(fileSize);
    if (buffer == NULL)
    {
        perror("malloc failed");
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, fileSize, f);
    if (read_size != (size_t)fileSize)
    {
        perror("fread failed or read incomplete");
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *outSize = fileSize;
    return buffer;
}

int ituJpegLoadFileEx (int width, int height, char * filepath)
{
    int       result = 0;
    int       size   = 0;
    uint8_t * data   = NULL;

    assert(filepath != NULL);

    do
    {
        data = _ituReadFileToBuffer(filepath, &size);
        if (data == NULL)
        {
            break;
        }

        /*
         * Static analysis tools may report a warning at the call to
         * ituJpegLoadEx(), stating that the input buffer 'data' contains
         * uninitialized values. This is a false positive.
         *
         * In the code above, 'data' is allocated using malloc(size), and then
         * filled immediately using fread() which reads exactly 'size' bytes
         * from a file opened in binary mode. The code verifies that fread()
         * returns the expected size before using the data, ensuring that all
         * bytes in the buffer are properly initialized with file content.
         *
         * Therefore, at the point where 'data' is passed to ituJpegLoadEx(),
         * the memory is guaranteed to be fully initialized, and the static
         * analysis warning can be safely ignored.
         */
        /* coverity[UNINIT: FALSE] */
        result = ituJpegLoadEx(width, height, data, size);
    } while (false);

    if (data != NULL)
    {
        free(data);
    }

    return result;
}

    #if (CFG_CHIP_FAMILY == 9830) || (CFG_CHIP_FAMILY == 9870) || (CFG_CHIP_FAMILY == 9880)
ITUSurface * ituJpegLoad (int width, int height, uint8_t * data, int size,
                          unsigned int flags)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint32_t        decWidth = 0U, decHeight = 0U;
    uint32_t        dispWidth = 0U, dispHeight = 0U;
    uint32_t        H_Samp = 0U, V_Samp = 0U, widthUnit = 0U, heightUnit = 0U;
    uint32_t        SmallPicWidth = 1280U, SmallPicHeight = 720U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while ((pCur < pEnd))
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 4;
                H_Samp = pStart[CurCount] >> 4;
                V_Samp = pStart[CurCount] & 0xF;
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }
    pCur = pStart;

    if (width == 0 || height == 0)
    {
        width  = (int)imgWidth;
        height = (int)imgHeight;
    }

    if (width % 16)
    {
        width = (width / 16 + 1) * 16;
    }

    if (height % 16)
    {
        height = (height / 16 + 1) * 16;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(width, height, 0, ITU_RGB565, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != width) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != height))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], width, height,
                          0, ITU_RGB565);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (surf == NULL)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }

    dest = (uint8_t *)ituLockSurface(surf, 0, 0, width, height);
    // ituColorFill(surf, 0, 0, width, height, &black);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    initParam.codecType = JPG_CODEC_DEC_JPG_CMD;
    initParam.decType   = JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                   \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
    initParam.bUsingCmdq = true;
        #endif
    initParam.dispMode      = JPG_DISP_CENTER;
    initParam.outColorSpace = JPG_COLOR_SPACE_RGB565;

    initParam.width         = width;
    initParam.height        = height;
    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jpg_reset_stream_info = 0;

    inStreamInfo.validCompCnt          = 1;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
    result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
    if (result == JPG_ERR_JPROG_STREAM)
    {
        (void)printf("JPG not support this format\n");
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

    /*
       (void)printf("memory mode  (%d, %d) %dx%d, dispMode=%d\r\n",
           jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
           jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
           initParam.dispMode);
     */

    real_width  = jpgUserInfo.real_width;
    real_height = jpgUserInfo.real_height;

    if (initParam.outColorSpace == JPG_COLOR_SPACE_RGB565)
    {
        outStreamInfo.jstream.mem[0].pAddr = (uint8_t *)dest; // get output buf;
        outStreamInfo.jstream.mem[0].pitch = surf->pitch;
        outStreamInfo.jstream.mem[0].length = surf->pitch * surf->height;
        outStreamInfo.validCompCnt          = 1;
        ITU_LOG_DBG("[JpegLoad][565][%d][%d][Addr: 0x%X]\n", surf->width,
                    surf->height, outStreamInfo.jstream.mem[0].pAddr);
    }

    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    // (void)printf("jpeg decode end\n");
    ituUnlockSurface(surf);
    return surf;

end:
    iteJpg_DestroyHandle(&pHJpeg, 0);
    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}

ITUSurface * ituJpegAlphaLoad (int width, int height, uint8_t * alpha,
                               uint8_t * data, int size)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while ((pCur < pEnd))
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }
    pCur = pStart;

    if (width == 0 || height == 0)
    {
        width  = (int)imgWidth;
        height = (int)imgHeight;
    }

    if (width % 16)
    {
        width = (width / 16 + 1) * 16;
    }

    if (height % 16)
    {
        height = (height / 16 + 1) * 16;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(width, height, 0, ITU_ARGB8888, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != width) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != height))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], width, height,
                          0, ITU_ARGB8888);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (!surf)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }
    dest = (uint8_t *)ituLockSurface(surf, 0, 0, width, height);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    initParam.codecType = JPG_CODEC_DEC_JPG_CMD; // JPG_CODEC_DEC_JPG;
    initParam.decType =
        JPG_DEC_PRIMARY; // JPG_DEC_SMALL_THUMB; //JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
    initParam.bUsingCmdq = true;
        #endif
    initParam.outColorSpace =
        JPG_COLOR_SPACE_ARGB8888; // JPG_COLOR_SPACE_ARGB4444;//JPG_COLOR_SPACE_ARGB8888;
    initParam.width    = width;
    initParam.height   = height;
    initParam.dispMode = JPG_DISP_CENTER;
    if (initParam.outColorSpace == JPG_COLOR_SPACE_ARGB4444 ||
        initParam.outColorSpace == JPG_COLOR_SPACE_ARGB8888)
    {
        initParam.alphaPlane.bEnConstAlpha = false;
        if (initParam.alphaPlane.bEnConstAlpha)
        {
            initParam.alphaPlane.ConstAlpha = 128; // 0 ~ 255
        }
        else
        {
            initParam.alphaPlane.AlphaPlaneAddr  = alpha; // alpha plane address
            initParam.alphaPlane.AlphaPlanePitch = width;
        }
    }

    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    inStreamInfo.validCompCnt          = 1;

    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);

    result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
    if (result == JPG_ERR_JPROG_STREAM)
    {
        (void)printf("JPG not support this format\n");
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    (void)printf("  disp(%lux%lu), dispMode=%d, real(%lux%lu), img(%lux%lu), "
                 "slice=%lu, pitch(%lu, %lu)\r\n",
                 jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
                 initParam.dispMode, jpgUserInfo.real_width,
                 jpgUserInfo.real_height, jpgUserInfo.imgWidth,
                 jpgUserInfo.imgHeight, jpgUserInfo.slice_num,
                 jpgUserInfo.comp1Pitch, jpgUserInfo.comp23Pitch);

    real_width                 = jpgUserInfo.real_width;
    real_height                = jpgUserInfo.real_height;

    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    switch (initParam.outColorSpace)
    {
        case JPG_COLOR_SPACE_ARGB8888:
            outStreamInfo.jstream.mem[0].pAddr =
                (uint8_t *)dest; // get output buf;
            outStreamInfo.jstream.mem[0].pitch = surf->pitch;
            outStreamInfo.jstream.mem[0].length =
                outStreamInfo.jstream.mem[0].pitch * surf->height;
            outStreamInfo.validCompCnt = 1;
            break;
    }

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    ituUnlockSurface(surf);
    return surf;

end:

    iteJpg_DestroyHandle(&pHJpeg, 0);
    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}
    #else
ITUSurface * ituJpegLoad (int width, int height, uint8_t * data, int size,
                          unsigned int flags)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    uint8_t *       pY             = NULL;
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint32_t        decWidth = 0U, decHeight = 0U;
    uint32_t        dispWidth = 0U, dispHeight = 0U;
    uint32_t        H_Samp = 0U, V_Samp = 0U, widthUnit = 0U, heightUnit = 0U;
    uint32_t        SmallPicWidth = 1280U, SmallPicHeight = 720U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while ((pCur < pEnd))
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 4;
                H_Samp = pStart[CurCount] >> 4;
                V_Samp = pStart[CurCount] & 0xF;
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }

        #if 1
    widthUnit  = H_Samp << 3;
    heightUnit = V_Samp << 3;
    decWidth   = (imgWidth + (widthUnit - 1)) & ~(widthUnit - 1);
    decHeight  = (imgHeight + (heightUnit - 1)) & ~(heightUnit - 1);
        #endif
    if (width == 0 || height == 0)
    {
        width      = (int)decWidth;
        height     = (int)decHeight;
        dispWidth  = imgWidth;
        dispHeight = imgHeight;
    }
    else
    {
        dispWidth  = width;
        dispHeight = height;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(dispWidth, dispHeight, 0, ITU_RGB565, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != dispWidth) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != dispHeight))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], dispWidth,
                          dispHeight, 0, ITU_RGB565);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (surf == NULL)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }
    dest = (uint8_t *)ituLockSurface(surf, 0, 0, dispWidth, dispHeight);
    // ituColorFill(surf, 0, 0, width, height, &black);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }
    else if ((imgWidth * imgHeight) <= (SmallPicWidth * SmallPicHeight))
    {
        initParam.codecType  = JPG_CODEC_DEC_JPG_CMD;
        initParam.decType    = JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
        initParam.bUsingCmdq = true;
        #endif
        if (flags == ITU_FIT_TO_RECT)
        {
            initParam.dispMode = JPG_DISP_FIT;
        }
        else if (flags == ITU_CUT_BY_RECT)
        {
            initParam.dispMode = JPG_DISP_CUT_BY_RECT;
        }
        else
        {
            initParam.dispMode = JPG_DISP_CENTER;
        }

        initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

        initParam.width         = dispWidth;
        initParam.height        = dispHeight;
        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

        inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
        inStreamInfo.streamType            = JPG_STREAM_MEM;
        inStreamInfo.jpg_reset_stream_info = 0;

        inStreamInfo.validCompCnt          = 1;
        inStreamInfo.jstream.mem[0].pAddr  = data;
        inStreamInfo.jstream.mem[0].length = size;
        iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
        result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
        if (result == JPG_ERR_JPROG_STREAM)
        {
            (void)printf("JPG not support this format\n");
            goto end;
        }

        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

        /*
           (void)printf("memory mode  (%d, %d) %dx%d, dispMode=%d\r\n",
               jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
               jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
               initParam.dispMode);
         */

        real_width  = jpgUserInfo.real_width;
        real_height = jpgUserInfo.real_height;

        if (initParam.outColorSpace == JPG_COLOR_SPACE_RGB565)
        {
            outStreamInfo.jstream.mem[0].pAddr =
                (uint8_t *)dest; // get output buf;
            outStreamInfo.jstream.mem[0].pitch  = surf->pitch;
            outStreamInfo.jstream.mem[0].length = surf->pitch * surf->height;
            outStreamInfo.validCompCnt          = 1;
            ITU_LOG_DBG("[JpegLoad][565][%d][%d][Addr: 0x%X]\n", surf->width,
                        surf->height, outStreamInfo.jstream.mem[0].pAddr);
        }
        else
        {
            pY = malloc(real_width * real_height * 3);
            // (void)memset(pY, 0x0, real_width * real_height * 3);

            // Y
            outStreamInfo.jstream.mem[0].pAddr  = (uint8_t *)pY;
            outStreamInfo.jstream.mem[0].pitch  = jpgUserInfo.comp1Pitch;
            outStreamInfo.jstream.mem[0].length = real_width * real_height;
            // U
            outStreamInfo.jstream.mem[1].pAddr =
                (uint8_t *)(outStreamInfo.jstream.mem[0].pAddr +
                            outStreamInfo.jstream.mem[0].length);
            outStreamInfo.jstream.mem[1].pitch = jpgUserInfo.comp23Pitch;
            outStreamInfo.jstream.mem[1].length =
                outStreamInfo.jstream.mem[1].pitch * real_height;
            // V
            outStreamInfo.jstream.mem[2].pAddr =
                (uint8_t *)(outStreamInfo.jstream.mem[1].pAddr +
                            outStreamInfo.jstream.mem[1].length);
            outStreamInfo.jstream.mem[2].pitch = jpgUserInfo.comp23Pitch;
            outStreamInfo.jstream.mem[2].length =
                outStreamInfo.jstream.mem[2].pitch * real_height;
            outStreamInfo.validCompCnt = 3;
            ITU_LOG_DBG("[JpegLoad][YUV][pY size: %d][js length: %d]\n",
                        outStreamInfo.jstream.mem[0].length +
                            outStreamInfo.jstream.mem[1].length +
                            outStreamInfo.jstream.mem[2].length);
        }
    }
    else
    {
        initParam.codecType     = JPG_CODEC_DEC_JPG;
        initParam.decType       = JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
        initParam.bUsingCmdq    = true;
        #endif
        initParam.outColorSpace = JPG_COLOR_SPACE_RGB565;
        initParam.width         = dispWidth;
        initParam.height        = dispHeight;

        if (flags == ITU_FIT_TO_RECT)
        {
            initParam.dispMode = JPG_DISP_FIT;
        }
        else if (flags == ITU_CUT_BY_RECT)
        {
            initParam.dispMode = JPG_DISP_CUT_BY_RECT;
        }
        else
        {
            initParam.dispMode = JPG_DISP_CENTER;
        }

        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

        inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
        inStreamInfo.streamType            = JPG_STREAM_MEM;
        inStreamInfo.jpg_reset_stream_info = 0;

        inStreamInfo.validCompCnt          = 1;
        inStreamInfo.jstream.mem[0].pAddr  = data;
        inStreamInfo.jstream.mem[0].length = size;

        result = iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
        if (result != JPG_ERR_OK)
        {
            (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
            goto end;
        }
        result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
        if (result == JPG_ERR_JPROG_STREAM)
        {
            (void)printf("JPG not support this format\n");
            goto end;
        }
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        /*
           (void)printf("handshake mode  (%d, %d) %dx%d, dispMode=%d\r\n",
               jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
               jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
               initParam.dispMode);
         */
        outStreamInfo.jstream.mem[0].pAddr = (uint8_t *)dest; // get output buf;
        outStreamInfo.jstream.mem[0].pitch = surf->pitch;
        outStreamInfo.jstream.mem[0].length = surf->pitch * surf->height;
        outStreamInfo.validCompCnt          = 1;
    }
    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    if (pY != NULL)
    {
        ithFlushDCacheRange((void *)pY, real_width * real_height * 3);
        ithFlushMemBuffer();
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        if (pY != NULL)
        {
            free(pY);
        }
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    if ((imgWidth * imgHeight) <= (SmallPicWidth * SmallPicHeight))
    {
        srcRect.w            = real_width;
        srcRect.h            = real_height;

        switch (jpgUserInfo.colorFormate)
        {
            case JPG_COLOR_SPACE_YUV411:
                colorType = DATA_COLOR_YUV422;
                break;
            case JPG_COLOR_SPACE_YUV444:
                colorType = DATA_COLOR_YUV444;
                break;
            case JPG_COLOR_SPACE_YUV422:
                colorType = DATA_COLOR_YUV422;
                break;
            case JPG_COLOR_SPACE_YUV420:
                colorType = DATA_COLOR_YUV420;
                break;
            case JPG_COLOR_SPACE_YUV422R:
                colorType = DATA_COLOR_YUV422R;
                break;
            case JPG_COLOR_SPACE_RGB565:
                colorType = DATA_COLOR_RGB565;
                break;
        }

        set_isp_colorTrans(outStreamInfo.jstream.mem[0].pAddr,
                           outStreamInfo.jstream.mem[1].pAddr,
                           outStreamInfo.jstream.mem[2].pAddr, colorType,
                           &srcRect, &destRect, imgWidth, imgHeight,
                           surf->pitch, dest, 0);
    }
    if (pY != NULL)
    {
        free(pY);
    }

    // (void)printf("jpeg decode end\n");
    ituUnlockSurface(surf);
    return surf;

end:
    iteJpg_DestroyHandle(&pHJpeg, 0);

    if (pY != NULL)
    {
        free(pY);
    }

    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}

ITUSurface * ituJpegLoad_QR (float percentage, uint8_t * data, int size,
                             unsigned int flags, int * header_width,
                             int * header_height)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        white          = {0, 255, 255, 255};
    uint8_t *       pY             = NULL;
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint32_t        dispWidth = 0U, dispHeight = 0U;
    uint32_t        H_Samp = 0U, V_Samp = 0U, widthUnit = 0U, heightUnit = 0U;
    uint32_t        SmallPicWidth = 1280U, SmallPicHeight = 720U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;
    uint32_t        c_index = 0U, p_index = 0U, width = 0U, height = 0U;
    uint8_t *       t_index = NULL;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while (pCur < pEnd)
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 4;
                H_Samp = pStart[CurCount] >> 4;
                V_Samp = pStart[CurCount] & 0xF;
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }

    width         = (((uint32_t)(imgWidth * percentage / 100) + 15) / 16) * 16;
    height        = (((uint32_t)(imgHeight * percentage / 100) + 15) / 16) * 16;
    dispWidth     = (((uint32_t)(imgWidth * percentage / 100) + 3) / 4) * 4;
    dispHeight    = (((uint32_t)(imgHeight * percentage / 100) + 3) / 4) * 4;
    *header_width = (int)(imgWidth * percentage / 100);
    *header_height = (int)(imgHeight * percentage / 100);
    (void)printf("imgWidth:%d, imgHeight:%d, header_width:%d, "
                 "header_height:%d, width:%d, height:%d, dispWidth:%d, "
                 "dispHeight:%d\n",
                 imgWidth, imgHeight, *header_width, *header_height, width,
                 height, dispWidth, dispHeight);

    if (!width || !height || !dispWidth || !dispHeight)
    {
        (void)printf("parameter error of JPEG scaling\n");
        return NULL;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(width, height, 0, ITU_RGB565, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != dispWidth) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != dispHeight))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], dispWidth,
                          dispHeight, 0, ITU_RGB565);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (!surf)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }
    dest = (uint8_t *)ituLockSurface(surf, 0, 0, width, height);
    ituColorFill(surf, 0, 0, width, height, &white);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    initParam.codecType  = JPG_CODEC_DEC_JPG_CMD;
    initParam.decType    = JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
    initParam.bUsingCmdq = true;
        #endif
    if (flags == ITU_FIT_TO_RECT)
    {
        initParam.dispMode = JPG_DISP_FIT;
    }
    else if (flags == ITU_CUT_BY_RECT)
    {
        initParam.dispMode = JPG_DISP_CUT_BY_RECT;
    }
    else
    {
        initParam.dispMode = JPG_DISP_CENTER;
    }

    initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

    initParam.width         = dispWidth;
    initParam.height        = dispHeight;
    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jpg_reset_stream_info = 0;

    inStreamInfo.validCompCnt          = 1;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
    result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
    if (result == JPG_ERR_JPROG_STREAM)
    {
        (void)printf("JPG not support this format\n");
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

    /*
       (void)printf("memory mode  (%d, %d) %dx%d, dispMode=%d\r\n",
           jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
           jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
           initParam.dispMode);
     */

    real_width  = jpgUserInfo.real_width;
    real_height = jpgUserInfo.real_height;

    if (initParam.outColorSpace == JPG_COLOR_SPACE_RGB565)
    {
        outStreamInfo.jstream.mem[0].pAddr = (uint8_t *)dest; // get output buf;
        outStreamInfo.jstream.mem[0].pitch = surf->pitch;
        outStreamInfo.jstream.mem[0].length = surf->pitch * surf->height;
        outStreamInfo.validCompCnt          = 1;
        ITU_LOG_DBG("[JpegLoad][565][%d][%d][Addr: 0x%X]\n", surf->width,
                    surf->height, outStreamInfo.jstream.mem[0].pAddr);
    }
    else
    {
        pY = malloc(real_width * real_height * 3);
        // (void)memset(pY, 0x0, real_width * real_height * 3);

        // Y
        outStreamInfo.jstream.mem[0].pAddr  = (uint8_t *)pY;
        outStreamInfo.jstream.mem[0].pitch  = jpgUserInfo.comp1Pitch;
        outStreamInfo.jstream.mem[0].length = real_width * real_height;
        // U
        outStreamInfo.jstream.mem[1].pAddr =
            (uint8_t *)(outStreamInfo.jstream.mem[0].pAddr +
                        outStreamInfo.jstream.mem[0].length);
        outStreamInfo.jstream.mem[1].pitch = jpgUserInfo.comp23Pitch;
        outStreamInfo.jstream.mem[1].length =
            outStreamInfo.jstream.mem[1].pitch * real_height;
        // V
        outStreamInfo.jstream.mem[2].pAddr =
            (uint8_t *)(outStreamInfo.jstream.mem[1].pAddr +
                        outStreamInfo.jstream.mem[1].length);
        outStreamInfo.jstream.mem[2].pitch = jpgUserInfo.comp23Pitch;
        outStreamInfo.jstream.mem[2].length =
            outStreamInfo.jstream.mem[2].pitch * real_height;
        outStreamInfo.validCompCnt = 3;
        ITU_LOG_DBG("[JpegLoad][YUV][pY size: %d][js length: %d]\n",
                    outStreamInfo.jstream.mem[0].length +
                        outStreamInfo.jstream.mem[1].length +
                        outStreamInfo.jstream.mem[2].length);
    }

    (void)printf("dec_out w:%d,h:%d\n", real_width, real_height);
    (void)printf(
        "dec_out a1:%p,a2:%p,a3:%p\n", outStreamInfo.jstream.mem[0].pAddr,
        outStreamInfo.jstream.mem[1].pAddr, outStreamInfo.jstream.mem[2].pAddr);
    (void)printf(
        "dec_out p1:%d,p2:%d,p3:%d\n", outStreamInfo.jstream.mem[0].pitch,
        outStreamInfo.jstream.mem[1].pitch, outStreamInfo.jstream.mem[2].pitch);
    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    if (pY != NULL)
    {
        ithFlushDCacheRange((void *)pY, real_width * real_height * 3);
        ithFlushMemBuffer();
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        if (pY != NULL)
        {
            free(pY);
        }
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    srcRect.w            = real_width;
    srcRect.h            = real_height;

    switch (jpgUserInfo.colorFormate)
    {
        case JPG_COLOR_SPACE_YUV411:
            colorType = DATA_COLOR_YUV422;
            break;
        case JPG_COLOR_SPACE_YUV444:
            colorType = DATA_COLOR_YUV444;
            break;
        case JPG_COLOR_SPACE_YUV422:
            colorType = DATA_COLOR_YUV422;
            break;
        case JPG_COLOR_SPACE_YUV420:
            colorType = DATA_COLOR_YUV420;
            break;
        case JPG_COLOR_SPACE_YUV422R:
            colorType = DATA_COLOR_YUV422R;
            break;
        case JPG_COLOR_SPACE_RGB565:
            colorType = DATA_COLOR_RGB565;
            break;
    }

    if (imgWidth % 4)
    {
        for (p_index = 0; p_index < real_width * real_height;
             p_index += outStreamInfo.jstream.mem[0].pitch)
        {
            t_index = outStreamInfo.jstream.mem[0].pAddr + p_index;
            (void)memset(t_index + imgWidth, 0xff, 4 - imgWidth % 4);
        }
    }

    if (imgHeight % 4)
    {
        t_index = outStreamInfo.jstream.mem[0].pAddr +
                  imgHeight * outStreamInfo.jstream.mem[0].pitch;
        for (p_index = 0; p_index < 4 - imgHeight % 4; p_index++)
        {
            (void)memset(t_index, 0xff, outStreamInfo.jstream.mem[0].pitch);
            t_index += outStreamInfo.jstream.mem[0].pitch;
        }
    }

    if (imgWidth % 4 || imgHeight % 4)
    {
        ithFlushDCacheRange(outStreamInfo.jstream.mem[0].pAddr,
                            real_width * real_height);
    }

    set_isp_colorTrans(outStreamInfo.jstream.mem[0].pAddr,
                       outStreamInfo.jstream.mem[1].pAddr,
                       outStreamInfo.jstream.mem[2].pAddr, colorType,
                       &srcRect, &destRect, ((imgWidth + 3) / 4) * 4,
                       ((imgHeight + 3) / 4) * 4, surf->pitch, dest, 0);

    if (pY != NULL)
    {
        free(pY);
    }

    // (void)printf("jpeg decode end\n");
    ituUnlockSurface(surf);
    return surf;

end:
    iteJpg_DestroyHandle(&pHJpeg, 0);

    if (pY != NULL)
    {
        free(pY);
    }

    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}

ITUSurface * ituJpegAlphaLoad (int width, int height, uint8_t * alpha,
                               uint8_t * data, int size)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }

    pStart = pCur = data;
    pEnd          = pCur + size;

    while (pCur < pEnd)
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }

    if (width == 0 || height == 0)
    {
        width  = (int)imgWidth;
        height = (int)imgHeight;
    }

    if (width % 16)
    {
        width = (width / 16 + 1) * 16;
    }

    if (height % 16)
    {
        height = (height / 16 + 1) * 16;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(width, height, 0, ITU_ARGB8888, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != width) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != height))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], width, height,
                          0, ITU_ARGB8888);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (surf == NULL)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }
    dest = (uint8_t *)ituLockSurface(surf, 0, 0, width, height);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    initParam.codecType = JPG_CODEC_DEC_JPG_CMD; // JPG_CODEC_DEC_JPG;
    initParam.decType =
        JPG_DEC_PRIMARY; // JPG_DEC_SMALL_THUMB; //JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
    initParam.bUsingCmdq = true;
        #endif
    initParam.outColorSpace =
        JPG_COLOR_SPACE_ARGB8888; // JPG_COLOR_SPACE_ARGB4444;//JPG_COLOR_SPACE_ARGB8888;
    initParam.width    = width;
    initParam.height   = height;
    initParam.dispMode = JPG_DISP_CENTER;
    if (initParam.outColorSpace == JPG_COLOR_SPACE_ARGB4444 ||
        initParam.outColorSpace == JPG_COLOR_SPACE_ARGB8888)
    {
        initParam.alphaPlane.bEnConstAlpha = false;
        if (initParam.alphaPlane.bEnConstAlpha)
        {
            initParam.alphaPlane.ConstAlpha = 128; // 0 ~ 255
        }
        else
        {
            initParam.alphaPlane.AlphaPlaneAddr  = alpha; // alpha plane address
            initParam.alphaPlane.AlphaPlanePitch = width;
        }
    }

    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    inStreamInfo.validCompCnt          = 1;

    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);

    result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
    if (result == JPG_ERR_JPROG_STREAM)
    {
        (void)printf("JPG not support this format\n");
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    (void)printf("  disp(%ux%u), dispMode=%d, real(%ux%u), img(%ux%u), "
                 "slice=%u, pitch(%u, %u)\r\n",
                 jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
                 initParam.dispMode, jpgUserInfo.real_width,
                 jpgUserInfo.real_height, jpgUserInfo.imgWidth,
                 jpgUserInfo.imgHeight, jpgUserInfo.slice_num,
                 jpgUserInfo.comp1Pitch, jpgUserInfo.comp23Pitch);

    real_width                 = jpgUserInfo.real_width;
    real_height                = jpgUserInfo.real_height;

    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    switch (initParam.outColorSpace)
    {
        case JPG_COLOR_SPACE_ARGB8888:
            outStreamInfo.jstream.mem[0].pAddr =
                (uint8_t *)dest; // get output buf;
            outStreamInfo.jstream.mem[0].pitch = surf->pitch;
            outStreamInfo.jstream.mem[0].length =
                outStreamInfo.jstream.mem[0].pitch * surf->height;
            outStreamInfo.validCompCnt = 1;
            break;
    }

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    {
        BASE_RECT       srcRect   = {0};
        DATA_COLOR_TYPE colorType = 0;

        srcRect.w                 = real_width;
        srcRect.h                 = real_height;

        // (void)printf("  ** jpgUserInfo.colorFormate=0x%x\n",
        // jpgUserInfo.colorFormate);
        switch (jpgUserInfo.colorFormate)
        {
            case JPG_COLOR_SPACE_YUV444:
                colorType = DATA_COLOR_YUV444;
                break;
            case JPG_COLOR_SPACE_YUV422:
                colorType = DATA_COLOR_YUV422;
                break;
            case JPG_COLOR_SPACE_YUV420:
                colorType = DATA_COLOR_YUV420;
                break;
            case JPG_COLOR_SPACE_YUV422R:
                colorType = DATA_COLOR_YUV422R;
                break;
            case JPG_COLOR_SPACE_RGB565:
                colorType = DATA_COLOR_RGB565;
                break;
            case JPG_COLOR_SPACE_ARGB8888:
                colorType = DATA_COLOR_ARGB8888;
                break;
            case JPG_COLOR_SPACE_ARGB4444:
                colorType = DATA_COLOR_ARGB4444;
                break;
        }

        set_isp_colorTrans(outStreamInfo.jstream.mem[0].pAddr,
                           outStreamInfo.jstream.mem[1].pAddr,
                           outStreamInfo.jstream.mem[2].pAddr, colorType,
                           &srcRect, &destRect, imgWidth, imgHeight,
                           surf->pitch, dest, 0);
    }
    ituUnlockSurface(surf);
    return surf;

end:

    iteJpg_DestroyHandle(&pHJpeg, 0);
    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}

ITUSurface * ituJpegLoadToARGB8888 (int width, int height, uint8_t * data,
                                    int size, unsigned int flags)
{
    ITUSurface *    surf           = NULL;
    uint8_t *       dest           = NULL;
    HJPG *          pHJpeg         = NULL;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    JPG_ERR         result         = JPG_ERR_OK;
    ITUColor        black          = {0, 0, 0, 0};
    uint8_t *       pY             = NULL;
    JPG_RECT        destRect       = {0};
    BASE_RECT       srcRect        = {0};
    DATA_COLOR_TYPE colorType      = 0;
    uint32_t        real_width = 0U, real_height = 0U, real_height_ForTile = 0U;
    uint32_t        imgWidth = 0U, imgHeight = 0U;
    uint32_t        decWidth = 0U, decHeight = 0U;
    uint32_t        dispWidth = 0U, dispHeight = 0U;
    uint32_t        H_Samp = 0U, V_Samp = 0U, widthUnit = 0U, heightUnit = 0U;
    uint32_t        SmallPicWidth = 1280U, SmallPicHeight = 720U;
    uint8_t *       pStart = NULL, *pCur = NULL, *pEnd = NULL;
    uint32_t        CurCount = 0U, MarkerType = 0U, GetMarkerLength = 0U;

    // malloc_stats();
    if (data[0] != 0xFFU || data[1] != 0xD8U)
    {
        (void)printf("jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     data[0], data[1]);
        return NULL; // return NULL to tell AP, if JPEG reading fail!.
    }
    pStart = pCur = data;
    pEnd          = pCur + size;

    while (pCur < pEnd)
    {
        MarkerType = (*(pCur) << 8 | *(pCur + 1));
        switch (MarkerType)
        {
            case JPEG_SOF_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                CurCount        = pCur - pStart;
                CurCount += 3; // pass 3 byte
                imgHeight = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 2;
                imgWidth = pStart[CurCount] << 8 | pStart[CurCount + 1];
                CurCount += 4;
                H_Samp = pStart[CurCount] >> 4;
                V_Samp = pStart[CurCount] & 0xF;
                pCur += GetMarkerLength;
                break;
            case JPEG_SOS_MARKER:
            case JPEG_DHT_MARKER:
            case JPEG_DRI_MARKER:
            case JPEG_DQT_MARKER:
            case JPEG_APP00_MARKER:
            case JPEG_APP01_MARKER:
            case JPEG_APP02_MARKER:
            case JPEG_APP03_MARKER:
            case JPEG_APP04_MARKER:
            case JPEG_APP05_MARKER:
            case JPEG_APP06_MARKER:
            case JPEG_APP07_MARKER:
            case JPEG_APP08_MARKER:
            case JPEG_APP09_MARKER:
            case JPEG_APP10_MARKER:
            case JPEG_APP11_MARKER:
            case JPEG_APP12_MARKER:
            case JPEG_APP13_MARKER:
            case JPEG_APP14_MARKER:
            case JPEG_APP15_MARKER:
            case JPEG_COM_MARKER:
                pCur += 2;
                GetMarkerLength = (*(pCur) << 8 | *(pCur + 1));
                pCur += GetMarkerLength;
                break;
            default:
                pCur++;
                break;
        }
    }

        #if 1
    widthUnit  = H_Samp << 3;
    heightUnit = V_Samp << 3;
    decWidth   = (imgWidth + (widthUnit - 1)) & ~(widthUnit - 1);
    decHeight  = (imgHeight + (heightUnit - 1)) & ~(heightUnit - 1);
        #endif
    if (width == 0 || height == 0)
    {
        width      = (int)decWidth;
        height     = (int)decHeight;
        dispWidth  = imgWidth;
        dispHeight = imgHeight;
    }
    else
    {
        dispWidth  = width;
        dispHeight = height;
    }

        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(dispWidth, dispHeight, 0, ITU_ARGB8888, NULL, 0);
        #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != dispWidth) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != dispHeight))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], dispWidth,
                          dispHeight, 0, ITU_ARGB8888);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
        #endif

    if (surf == NULL)
    {
        (void)printf("Jpeg Create Surface fail !!\n");
        return NULL;
    }
    dest = (uint8_t *)ituLockSurface(surf, 0, 0, dispWidth, dispHeight);
    // ituColorFill(surf, 0, 0, width, height, &black);

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("JPG not support this format\n");
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    initParam.codecType  = JPG_CODEC_DEC_JPG_CMD;
    initParam.decType    = JPG_DEC_PRIMARY;
        #if defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) &&                     \
            !defined(ITU_SURFACE_DEBUG_JPEGBUFF)
    initParam.bUsingCmdq = true;
        #endif
    if (flags == ITU_FIT_TO_RECT)
    {
        initParam.dispMode = JPG_DISP_FIT;
    }
    else if (flags == ITU_CUT_BY_RECT)
    {
        initParam.dispMode = JPG_DISP_CUT_BY_RECT;
    }
    else
    {
        initParam.dispMode = JPG_DISP_CENTER;
    }

    initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

    initParam.width         = dispWidth;
    initParam.height        = dispHeight;
    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jpg_reset_stream_info = 0;

    inStreamInfo.validCompCnt          = 1;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
    result = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);
    if (result == JPG_ERR_JPROG_STREAM)
    {
        (void)printf("JPG not support this format\n");
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

    /*
       (void)printf("memory mode  (%d, %d) %dx%d, dispMode=%d\r\n",
           jpgUserInfo.jpgRect.x, jpgUserInfo.jpgRect.y,
           jpgUserInfo.jpgRect.w, jpgUserInfo.jpgRect.h,
           initParam.dispMode);
     */

    real_width                          = jpgUserInfo.real_width;
    real_height                         = jpgUserInfo.real_height;

    pY                                  = malloc(real_width * real_height * 3);
    // (void)memset(pY, 0x0, real_width * real_height * 3);

    // Y
    outStreamInfo.jstream.mem[0].pAddr  = (uint8_t *)pY;
    outStreamInfo.jstream.mem[0].pitch  = jpgUserInfo.comp1Pitch;
    outStreamInfo.jstream.mem[0].length = real_width * real_height;
    // U
    outStreamInfo.jstream.mem[1].pAddr =
        (uint8_t *)(outStreamInfo.jstream.mem[0].pAddr +
                    outStreamInfo.jstream.mem[0].length);
    outStreamInfo.jstream.mem[1].pitch = jpgUserInfo.comp23Pitch;
    outStreamInfo.jstream.mem[1].length =
        outStreamInfo.jstream.mem[1].pitch * real_height;
    // V
    outStreamInfo.jstream.mem[2].pAddr =
        (uint8_t *)(outStreamInfo.jstream.mem[1].pAddr +
                    outStreamInfo.jstream.mem[1].length);
    outStreamInfo.jstream.mem[2].pitch = jpgUserInfo.comp23Pitch;
    outStreamInfo.jstream.mem[2].length =
        outStreamInfo.jstream.mem[2].pitch * real_height;
    outStreamInfo.validCompCnt = 3;
    ITU_LOG_DBG("[JpegLoad][YUV][pY size: %d][js length: %d]\n",
                outStreamInfo.jstream.mem[0].length +
                    outStreamInfo.jstream.mem[1].length +
                    outStreamInfo.jstream.mem[2].length);

    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;

    result = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    result = iteJpg_Setup(pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    if (pY != NULL)
    {
        ithFlushDCacheRange((void *)pY, real_width * real_height * 3);
        ithFlushMemBuffer();
    }

    result = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        goto end;
    }

    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    // (void)printf("\n\tresult = %d\n", jpgUserInfo.status);

    result = iteJpg_DestroyHandle(&pHJpeg, 0);
    if (result != JPG_ERR_OK)
    {
        (void)printf(" jpeg err ! %s [%d]\r\n", __FILE__, __LINE__);
        if (pY != NULL)
        {
            free(pY);
        }
        ituUnlockSurface(surf);
        ituDestroySurface(surf);
        return NULL;
    }

    srcRect.w            = real_width;
    srcRect.h            = real_height;

    switch (jpgUserInfo.colorFormate)
    {
        case JPG_COLOR_SPACE_YUV411:
            colorType = DATA_COLOR_YUV422;
            break;
        case JPG_COLOR_SPACE_YUV444:
            colorType = DATA_COLOR_YUV444;
            break;
        case JPG_COLOR_SPACE_YUV422:
            colorType = DATA_COLOR_YUV422;
            break;
        case JPG_COLOR_SPACE_YUV420:
            colorType = DATA_COLOR_YUV420;
            break;
        case JPG_COLOR_SPACE_YUV422R:
            colorType = DATA_COLOR_YUV422R;
            break;
    }

    set_isp_colorTrans(
        outStreamInfo.jstream.mem[0].pAddr, outStreamInfo.jstream.mem[1].pAddr,
        outStreamInfo.jstream.mem[2].pAddr, colorType, &srcRect,
        &destRect, imgWidth, imgHeight, surf->pitch, dest, 1);

    if (pY != NULL)
    {
        free(pY);
    }

    // (void)printf("jpeg decode end\n");
    ituUnlockSurface(surf);
    return surf;

end:
    iteJpg_DestroyHandle(&pHJpeg, 0);

    if (pY != NULL)
    {
        free(pY);
    }

    ituUnlockSurface(surf);
    ituDestroySurface(surf);
    return NULL;
}
    #endif

ITUSurface * ituJpegLoadFile (int width, int height, char * filepath,
                              unsigned int flags)
{
    ITUSurface * surf = NULL;
    int          size = 0;
    uint8_t *    data = NULL;

    assert(filepath != NULL);

    do
    {
        data = _ituReadFileToBuffer(filepath, &size);
        if (data == NULL)
        {
            break;
        }

        (void)printf("+ituJpegLoad(%d,%d,%p,%d)\n", width, height, data, size);
        /*
         * Static analysis tools may report a warning at the call to
         * ituJpegLoad(), stating that the input buffer 'data' contains
         * uninitialized values. This is a false positive.
         *
         * In the code above, 'data' is allocated using malloc(size), and then
         * filled immediately using fread() which reads exactly 'size' bytes
         * from a file opened in binary mode. The code verifies that fread()
         * returns the expected size before using the data, ensuring that all
         * bytes in the buffer are properly initialized with file content.
         *
         * Therefore, at the point where 'data' is passed to ituJpegLoad(), the
         * memory is guaranteed to be fully initialized, and the static analysis
         * warning can be safely ignored.
         */
        /* coverity[UNINIT: FALSE] */
        surf = ituJpegLoad(width, height, data, size, flags);
        (void)printf("-ituJpegLoad\n");
    } while (false);

    if (data != NULL)
    {
        free(data);
    }

    return surf;
}

void ituJpegSaveBuf (ITUSurface * surf, char * buf, int bufsize, int * jpgSize)
{
    uint8_t * src      = NULL;
    uint8_t * yuv_data = NULL;
    src                = ituLockSurface(surf, 0, 0, surf->width, surf->height);
    assert(src != NULL);

    if (surf == ituGetDisplaySurface())
    {
        uint32_t addr;

        switch (ithLcdGetFlip())
        {
            case 0:
                addr = ithLcdGetBaseAddrA();
                break;

            case 1:
                addr = ithLcdGetBaseAddrB();
                break;

            default:
                addr = ithLcdGetBaseAddrC();
                break;
        }
        src = (uint8_t *)ithMapVram(addr, surf->lockSize, ITH_VRAM_READ);
    }

    yuv_data = (char *)malloc(surf->width * surf->height * 3 / 2);
    if (yuv_data != NULL)
    {
    #if (CFG_CHIP_FAMILY == 9830) || (CFG_CHIP_FAMILY == 9870) || (CFG_CHIP_FAMILY == 9880)
        if (surf->format == ITU_ARGB8888)
        {
            argb8888toyuv420(yuv_data, src, surf->width, surf->height);
        }
        else if (surf->format == ITU_RGB565)
        {
            int       h;
            int       size   = surf->width * surf->height * 3;
            uint8_t * rgb888 = malloc(size);
            if (rgb888 != NULL)
            {
                for (h = 0; h < surf->height; h++)
                {
                    int       i, j;
                    uint8_t * ptr = src + surf->width * 2 * h;

                    // color trasform from RGB565 to RGB888
                    for (i = (surf->width - 1) * 2, j = (surf->width - 1) * 3;
                         i >= 0 && j >= 0; i -= 2, j -= 3)
                    {
                        rgb888[surf->width * h * 3 + j + 0] =
                            ((ptr[i + 1]) & 0xf8) + ((ptr[i + 1] >> 5) & 0x07);
                        rgb888[surf->width * h * 3 + j + 1] =
                            ((ptr[i + 0] >> 3) & 0x1c) +
                            ((ptr[i + 1] << 5) & 0xe0) +
                            ((ptr[i + 1] >> 1) & 0x3);
                        rgb888[surf->width * h * 3 + j + 2] =
                            ((ptr[i + 0] << 3) & 0xf8) +
                            ((ptr[i + 0] >> 2) & 0x07);
                    }
                }

                rgb888toyuv420(yuv_data, rgb888, surf->width, surf->height);
                free(rgb888);
            }
        }
    #else
        if (surf->format == ITU_ARGB8888)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_ARGB8888);
        }
        else if (surf->format == ITU_RGB565)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_RGB565);
        }
    #endif

        HJPG *          pHJpeg         = NULL;
        JPG_INIT_PARAM  initParam      = {0};
        JPG_STREAM_INFO inStreamInfo   = {0};
        JPG_STREAM_INFO outStreamInfo  = {0};
        JPG_BUF_INFO    entropyBufInfo = {0};
        JPG_USER_INFO   jpgUserInfo    = {0};
        uint32_t        jpgEncSize     = 0;

        unsigned char * pAddr_y = NULL, *pAddr_u = NULL, *pAddr_v = NULL;
        pAddr_y                 = yuv_data;
        pAddr_u                 = yuv_data + (surf->width * surf->height);
        pAddr_v                 = pAddr_u + (surf->width * surf->height / 4);

        initParam.codecType     = JPG_CODEC_ENC_JPG;
        initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

        initParam.width         = surf->width;
        initParam.height        = surf->height;

        initParam.encQuality    = 70; // 85;

        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);
        inStreamInfo.streamIOType         = JPG_STREAM_IO_READ;
        inStreamInfo.streamType           = JPG_STREAM_MEM;
        // Y
        inStreamInfo.jstream.mem[0].pAddr = (uint8_t *)pAddr_y; // YUV_Save;
        inStreamInfo.jstream.mem[0].pitch = surf->width;        // src_w_out;

        // U
        inStreamInfo.jstream.mem[1].pAddr = (uint8_t *)pAddr_u;
        inStreamInfo.jstream.mem[1].pitch = surf->width / 2; // src_w_out/2;

        // V
        inStreamInfo.jstream.mem[2].pAddr = (uint8_t *)pAddr_v;
        inStreamInfo.jstream.mem[2].pitch = surf->width / 2; // src_w_out/2;

        inStreamInfo.validCompCnt         = 3;

        if (buf != NULL)
        {
            outStreamInfo.streamType            = JPG_STREAM_MEM;
            outStreamInfo.jstream.mem[0].pAddr  = buf;
            outStreamInfo.jstream.mem[0].pitch  = surf->width;
            outStreamInfo.jstream.mem[0].length = bufsize;
        }
        outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
        outStreamInfo.jpg_reset_stream_info = 0; //  _reset_stream_info;
        outStreamInfo.validCompCnt          = 1;

        iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo, 0);
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

        iteJpg_Setup(pHJpeg, 0);

        iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        printf("\n\tresult = %d, encode size = %d Bytes\n", jpgUserInfo.status,
               jpgEncSize);
        *jpgSize = jpgEncSize;

        iteJpg_DestroyHandle(&pHJpeg, 0);
        free(yuv_data);
    }

    ituUnlockSurface(surf);
}

void ituJpegSaveFile (ITUSurface * surf, char * filepath)
{
    uint8_t * src;
    uint8_t * yuv_data = NULL;
    src                = ituLockSurface(surf, 0, 0, surf->width, surf->height);
    assert(src != NULL);

    if (surf == ituGetDisplaySurface())
    {
        uint32_t addr;

        switch (ithLcdGetFlip())
        {
            case 0:
                addr = ithLcdGetBaseAddrA();
                break;

            case 1:
                addr = ithLcdGetBaseAddrB();
                break;

            default:
                addr = ithLcdGetBaseAddrC();
                break;
        }
        src = (uint8_t *)ithMapVram(addr, surf->lockSize, ITH_VRAM_READ);
    }

    yuv_data = (char *)malloc(surf->width * surf->height * 3 / 2);
    if (yuv_data != NULL)
    {
    #if (CFG_CHIP_FAMILY == 9830) || (CFG_CHIP_FAMILY == 9870) || (CFG_CHIP_FAMILY == 9880)
        if (surf->format == ITU_ARGB8888)
        {
            argb8888toyuv420(yuv_data, src, surf->width, surf->height);
        }
        else if (surf->format == ITU_RGB565)
        {
            int       h;
            int       size   = surf->width * surf->height * 3;
            uint8_t * rgb888 = malloc(size);
            if (rgb888 != NULL)
            {
                for (h = 0; h < surf->height; h++)
                {
                    int       i, j;
                    uint8_t * ptr = src + surf->width * 2 * h;

                    // color trasform from RGB565 to RGB888
                    for (i = (surf->width - 1) * 2, j = (surf->width - 1) * 3;
                         i >= 0 && j >= 0; i -= 2, j -= 3)
                    {
                        rgb888[surf->width * h * 3 + j + 0] =
                            ((ptr[i + 1]) & 0xf8) + ((ptr[i + 1] >> 5) & 0x07);
                        rgb888[surf->width * h * 3 + j + 1] =
                            ((ptr[i + 0] >> 3) & 0x1c) +
                            ((ptr[i + 1] << 5) & 0xe0) +
                            ((ptr[i + 1] >> 1) & 0x3);
                        rgb888[surf->width * h * 3 + j + 2] =
                            ((ptr[i + 0] << 3) & 0xf8) +
                            ((ptr[i + 0] >> 2) & 0x07);
                    }
                }

                rgb888toyuv420(yuv_data, rgb888, surf->width, surf->height);
                free(rgb888);
            }
        }
    #else
        if (surf->format == ITU_ARGB8888)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_ARGB8888);
        }
        else if (surf->format == ITU_RGB565)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_RGB565);
        }
    #endif

        HJPG *          pHJpeg         = NULL;
        JPG_INIT_PARAM  initParam      = {0};
        JPG_STREAM_INFO inStreamInfo   = {0};
        JPG_STREAM_INFO outStreamInfo  = {0};
        JPG_BUF_INFO    entropyBufInfo = {0};
        JPG_USER_INFO   jpgUserInfo    = {0};
        uint32_t        jpgEncSize     = 0;

        unsigned char * pAddr_y = NULL, *pAddr_u = NULL, *pAddr_v = NULL;
        pAddr_y                 = yuv_data;
        pAddr_u                 = yuv_data + (surf->width * surf->height);
        pAddr_v                 = pAddr_u + (surf->width * surf->height / 4);

        initParam.codecType     = JPG_CODEC_ENC_JPG;
        initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

        initParam.width         = surf->width;
        initParam.height        = surf->height;

        initParam.encQuality    = 70; // 85;

        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);
        inStreamInfo.streamIOType         = JPG_STREAM_IO_READ;
        inStreamInfo.streamType           = JPG_STREAM_MEM;
        // Y
        inStreamInfo.jstream.mem[0].pAddr = (uint8_t *)pAddr_y; // YUV_Save;
        inStreamInfo.jstream.mem[0].pitch = surf->width;        // src_w_out;

        // U
        inStreamInfo.jstream.mem[1].pAddr = (uint8_t *)pAddr_u;
        inStreamInfo.jstream.mem[1].pitch = surf->width / 2; // src_w_out/2;

        // V
        inStreamInfo.jstream.mem[2].pAddr = (uint8_t *)pAddr_v;
        inStreamInfo.jstream.mem[2].pitch = surf->width / 2; // src_w_out/2;

        inStreamInfo.validCompCnt         = 3;

        if (filepath != NULL)
        {
            outStreamInfo.streamType   = JPG_STREAM_FILE;
            outStreamInfo.jstream.path = (void *)filepath;
        }
        outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
        outStreamInfo.jpg_reset_stream_info = 0; //  _reset_stream_info;

        iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo, 0);
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

        iteJpg_Setup(pHJpeg, 0);

        iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);

        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        (void)printf("\n\tresult = %d, encode size = %f KB\n",
                     jpgUserInfo.status, (float)jpgEncSize / 1024);

        iteJpg_DestroyHandle(&pHJpeg, 0);

        free(yuv_data);
    }

    ituUnlockSurface(surf);
}

ITUSurface * ituJpegLoadFile_QR (float percentage, char * filepath,
                                 unsigned int flags, int * header_width,
                                 int * header_height)
{
    ITUSurface * surf = NULL;
    int          size = 0;
    uint8_t *    data = NULL;

    assert(filepath != NULL);

    do
    {
        data = _ituReadFileToBuffer(filepath, &size);
        if (data == NULL)
        {
            break;
        }

        (void)printf("+ituJpegLoad(%f,%p,%d)\n", percentage, data, size);
        /*
         * Static analysis tools may report a warning at the call to
         * ituJpegLoad_QR(), stating that the input buffer 'data' contains
         * uninitialized values. This is a false positive.
         *
         * In the code above, 'data' is allocated using malloc(size), and then
         * filled immediately using fread() which reads exactly 'size' bytes
         * from a file opened in binary mode. The code verifies that fread()
         * returns the expected size before using the data, ensuring that all
         * bytes in the buffer are properly initialized with file content.
         *
         * Therefore, at the point where 'data' is passed to ituJpegLoad_QR(),
         * the memory is guaranteed to be fully initialized, and the static
         * analysis warning can be safely ignored.
         */
        /* coverity[UNINIT: FALSE] */
        surf = ituJpegLoad_QR(percentage, data, size, flags, header_width,
                              header_height);
        (void)printf("-ituJpegLoad\n");
    } while (false);

    if (data != NULL)
    {
        free(data);
    }

    return surf;
}

void ituJpegSaveFile_QR (ITUSurface * surf, char * filepath, int header_width,
                         int header_height)
{
    uint8_t * src      = NULL;
    uint8_t * yuv_data = NULL;
    (void)printf("ituJpegSaveFile_QR s_w:%d,s_h:%d,h_w:%d,h_h:%d\n",
                 surf->width, surf->height, header_width, header_height);
    src = ituLockSurface(surf, 0, 0, surf->width, surf->height);
    assert(src != NULL);

    if (surf == ituGetDisplaySurface())
    {
        uint32_t addr;

        switch (ithLcdGetFlip())
        {
            case 0:
                addr = ithLcdGetBaseAddrA();
                break;

            case 1:
                addr = ithLcdGetBaseAddrB();
                break;

            default:
                addr = ithLcdGetBaseAddrC();
                break;
        }
        src = (uint8_t *)ithMapVram(addr, surf->lockSize, ITH_VRAM_READ);
    }

    yuv_data = (char *)malloc(surf->width * surf->height * 3 / 2);

    if (yuv_data != NULL)
    {
        (void)printf("ituJpegSaveFile_QR yuv_data:%p\n", yuv_data);
    #if (CFG_CHIP_FAMILY == 9830) || (CFG_CHIP_FAMILY == 9870) || (CFG_CHIP_FAMILY == 9880)
        if (surf->format == ITU_ARGB8888)
        {
            argb8888toyuv420(yuv_data, src, surf->width, surf->height);
        }
        else if (surf->format == ITU_RGB565)
        {
            int       h;
            int       size   = surf->width * surf->height * 3;
            uint8_t * rgb888 = malloc(size);
            if (rgb888)
            {
                for (h = 0; h < surf->height; h++)
                {
                    int       i, j;
                    uint8_t * ptr = src + surf->width * 2 * h;

                    // color trasform from RGB565 to RGB888
                    for (i = (surf->width - 1) * 2, j = (surf->width - 1) * 3;
                         i >= 0 && j >= 0; i -= 2, j -= 3)
                    {
                        rgb888[surf->width * h * 3 + j + 0] =
                            ((ptr[i + 1]) & 0xf8) + ((ptr[i + 1] >> 5) & 0x07);
                        rgb888[surf->width * h * 3 + j + 1] =
                            ((ptr[i + 0] >> 3) & 0x1c) +
                            ((ptr[i + 1] << 5) & 0xe0) +
                            ((ptr[i + 1] >> 1) & 0x3);
                        rgb888[surf->width * h * 3 + j + 2] =
                            ((ptr[i + 0] << 3) & 0xf8) +
                            ((ptr[i + 0] >> 2) & 0x07);
                    }
                }

                rgb888toyuv420(yuv_data, rgb888, surf->width, surf->height);
                free(rgb888);
            }
        }
    #else
        if (surf->format == ITU_ARGB8888)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_ARGB8888);
        }
        else if (surf->format == ITU_RGB565)
        {
            ISP_RGBtoYUV420(yuv_data, src, surf->width, surf->height,
                            ITU_RGB565);
        }
    #endif

        HJPG *          pHJpeg         = NULL;
        JPG_INIT_PARAM  initParam      = {0};
        JPG_STREAM_INFO inStreamInfo   = {0};
        JPG_STREAM_INFO outStreamInfo  = {0};
        JPG_BUF_INFO    entropyBufInfo = {0};
        JPG_USER_INFO   jpgUserInfo    = {0};
        uint32_t        jpgEncSize     = 0;

        unsigned char * pAddr_y = NULL, *pAddr_u = NULL, *pAddr_v = NULL;
        pAddr_y                 = yuv_data;
        pAddr_u                 = yuv_data + (surf->width * surf->height);
        pAddr_v                 = pAddr_u + (surf->width * surf->height / 4);

        initParam.codecType     = JPG_CODEC_ENC_JPG;
        initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

        initParam.width         = surf->width;
        initParam.height        = surf->height;
        initParam.header_width  = header_width;
        initParam.header_height = header_height;
        // initParam.header_width            = surf->width;
        // initParam.header_height           = surf->height;

        initParam.encQuality    = 70; // 85;

        iteJpg_CreateHandle(&pHJpeg, &initParam, 0);
        inStreamInfo.streamIOType         = JPG_STREAM_IO_READ;
        inStreamInfo.streamType           = JPG_STREAM_MEM;
        // Y
        inStreamInfo.jstream.mem[0].pAddr = (uint8_t *)pAddr_y; // YUV_Save;
        inStreamInfo.jstream.mem[0].pitch = surf->width;        // src_w_out;

        // U
        inStreamInfo.jstream.mem[1].pAddr = (uint8_t *)pAddr_u;
        inStreamInfo.jstream.mem[1].pitch = surf->width / 2; // src_w_out/2;

        // V
        inStreamInfo.jstream.mem[2].pAddr = (uint8_t *)pAddr_v;
        inStreamInfo.jstream.mem[2].pitch = surf->width / 2; // src_w_out/2;

        inStreamInfo.validCompCnt         = 3;

        if (filepath != NULL)
        {
            outStreamInfo.streamType   = JPG_STREAM_FILE;
            outStreamInfo.jstream.path = (void *)filepath;
        }
        outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
        outStreamInfo.jpg_reset_stream_info = 0; //  _reset_stream_info;

        iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo, 0);
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

        iteJpg_Setup(pHJpeg, 0);

        iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);

        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        (void)printf("\n\tresult = %d, encode size = %f KB\n",
                     jpgUserInfo.status, (float)jpgEncSize / 1024);

        iteJpg_DestroyHandle(&pHJpeg, 0);

        free(yuv_data);
    }

    ituUnlockSurface(surf);
}

void ituJpegFileScale_QR (char * inputfile, char * outputfile, float percentage)
{
    ITUSurface * surf         = NULL;
    int          header_width = 0, header_height = 0;
    surf = ituJpegLoadFile_QR(percentage, inputfile, ITU_FIT_TO_RECT,
                              &header_width, &header_height);
    if (surf != NULL)
    {
        ituJpegSaveFile_QR(surf, outputfile, header_width, header_height);
        ituDestroySurface(surf);
    }
}

#else
ITUSurface * ituJpegLoad (int width, int height, uint8_t * data, int size,
                          unsigned int flags)
{
    uint8_t *                     src  = NULL;
    uint16_t *                    dest = NULL;
    ITUSurface *                  surf = NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;
    int                           w, h, x, y;

    assert(data != NULL);
    assert(size > 0);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);
    jpeg_read_header(&cinfo, TRUE);

    if (width && height)
    {
        cinfo.scale_num = 1;
        cinfo.scale_denom =
            ITH_MAX(cinfo.image_width / width, cinfo.image_height / height);
        if (cinfo.scale_denom == 0)
        {
            cinfo.scale_denom = 1;
        }
    }

    jpeg_start_decompress(&cinfo);

    if (cinfo.output_components != 3)
    {
        goto end;
    }

    src = malloc(cinfo.output_width * cinfo.output_components);
    if (src == NULL)
    {
        goto end;
    }

    y = 0;

    if (width == 0 || height == 0)
    {
        w = (int)cinfo.output_width;
        h = (int)cinfo.output_height;
    }
    else
    {
        w = width < (int)cinfo.output_width ? width : cinfo.output_width;
        h = height < (int)cinfo.output_height ? height : cinfo.output_height;
    }

    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(w, h, 0, ITU_RGB565, NULL, 0);
    #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != w) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != h))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], w, h, 0,
                          ITU_RGB565);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
    #endif
    if (surf == NULL)
    {
        goto end;
    }

    dest = (uint16_t *)ituLockSurface(surf, 0, 0, w, h);
    assert(dest != NULL);

    while ((int)cinfo.output_scanline < h)
    {
        jpeg_read_scanlines(&cinfo, &src, 1);

        for (x = 0; x < w; x++)
        {
            dest[x + y * w] =
                ITH_RGB565(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
        }
        y++;
    }

    jpeg_destroy_decompress(&cinfo);
    ituUnlockSurface(surf);

end:
    if (src != NULL)
    {
        free(src);
    }

    return surf;
}

ITUSurface * ituJpegAlphaLoad (int width, int height, uint8_t * alpha,
                               uint8_t * data, int size)
{
    uint8_t *                     src  = NULL;
    uint32_t *                    dest = NULL;
    ITUSurface *                  surf = NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;
    int                           w, h, x, y;

    assert(data != NULL);
    assert(size > 0);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    if (cinfo.output_components != 3)
    {
        goto end;
    }

    src = malloc(cinfo.output_width * cinfo.output_components);
    if (src == NULL)
    {
        goto end;
    }

    y    = 0;
    w    = width;
    h    = height;

    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    surf = ituCreateSurface(w, h, 0, ITU_ARGB8888, NULL, 0);
    #else
    if ((ituScene->surfpl[ituScene->dbuffIDX]->width != w) ||
        (ituScene->surfpl[ituScene->dbuffIDX]->height != h))
    {
        ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], w, h, 0,
                          ITU_ARGB8888);
    }
    surf = ituScene->surfpl[ituScene->dbuffIDX];
    #endif
    if (surf == NULL)
    {
        goto end;
    }

    dest = (uint32_t *)ituLockSurface(surf, 0, 0, w, h);
    assert(dest != NULL);

    while ((int)cinfo.output_scanline < h)
    {
        jpeg_read_scanlines(&cinfo, &src, 1);

        for (x = 0; x < w; x++)
        {
            dest[x + y * w] = ITH_ARGB8888(*alpha++, src[x * 3], src[x * 3 + 1],
                                           src[x * 3 + 2]);
        }
        y++;
    }

    jpeg_destroy_decompress(&cinfo);
    ituUnlockSurface(surf);

end:
    if (src != NULL)
    {
        free(src);
    }

    return surf;
}

/**
 * Loads a JPEG file into an ITUSurface (No hardware acceleration).
 *
 * This function reads a JPEG file from the specified file path, decompresses
 * it, and creates an ITUSurface with the specified width and height. If the
 * specified width and height are zero, the function uses the image's original
 * dimensions. The image is scaled to fit the specified dimensions while
 * maintaining its aspect ratio.
 *
 * @param width The desired width of the output surface. Can be 0 to use the
 * original width.
 * @param height The desired height of the output surface. Can be 0 to use the
 * original height.
 * @param filepath The file path of the JPEG file to load.
 * @param flags Flags for additional options (currently unused).
 * @return A pointer to the created ITUSurface, or NULL if an error occurs.
 */

ITUSurface * ituJpegLoadFile (int width, int height, char * filepath,
                              unsigned int flags)
{
    ITUSurface * surf = NULL;
    uint8_t *    src  = NULL;
    FILE *       f    = NULL;

    assert(filepath != NULL);

    do
    {
        uint16_t *                    dest = NULL;
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr         jerr;
        int                           w, h, x, y;

        f = fopen(filepath, "rb");
        if (f == NULL)
        {
            break;
        }

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, f);
        jpeg_read_header(&cinfo, TRUE);

        if (width && height)
        {
            cinfo.scale_num = 1;
            cinfo.scale_denom =
                ITH_MAX(cinfo.image_width / width, cinfo.image_height / height);
            if (cinfo.scale_denom == 0)
            {
                cinfo.scale_denom = 1;
            }
        }

        jpeg_start_decompress(&cinfo);

        if (cinfo.output_components != 3)
        {
            break;
        }

        src = malloc(cinfo.output_width * cinfo.output_components *
                     cinfo.output_height);
        if (src == NULL)
        {
            break;
        }

        while (cinfo.output_scanline < cinfo.output_height)
        {
            uint8_t * rowp[1];
            rowp[0] = src + cinfo.output_scanline * cinfo.output_width *
                                cinfo.output_components;
            jpeg_read_scanlines(&cinfo, rowp, 1);
        }

        if (width == 0 || height == 0)
        {
            w = (int)cinfo.output_width;
            h = (int)cinfo.output_height;
        }
        else
        {
            w = width < (int)cinfo.output_width ? width : cinfo.output_width;
            h = height < (int)cinfo.output_height ? height
                                                  : cinfo.output_height;
        }

        surf = ituCreateSurface(w, h, 0, ITU_RGB565, NULL, 0);
        if (surf == NULL)
        {
            break;
        }

        dest = (uint16_t *)ituLockSurface(surf, 0, 0, w, h);
        assert(dest != NULL);

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                int xx          = x * cinfo.output_width / w;
                int yy          = y * cinfo.output_height / h;
                int index       = cinfo.output_width * yy + xx;
                dest[w * y + x] = ITH_RGB565(src[index * 3], src[index * 3 + 1],
                                             src[index * 3 + 2]);
            }
        }

        jpeg_destroy_decompress(&cinfo);
        ituUnlockSurface(surf);
    } while (false);

    if (src != NULL)
    {
        free(src);
    }

    if (f != NULL)
    {
        fclose(f);
    }

    return surf;
}

void ituJpegSaveFile (ITUSurface * surf, char * filepath)
{
    FILE *                      fp = NULL;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;
    JSAMPROW                    row_pointer[1];
    JSAMPLE *                   image_buffer = NULL;
    int                         h;
    uint8_t *                   src = NULL;

    do
    {
        fp = fopen(filepath, "wb");
        if (fp == NULL)
        {
            ITU_LOG_ERR("open %s fail.\n", filepath);
            break;
        }

        image_buffer = malloc(surf->width * surf->height * 3);
        if (image_buffer == NULL)
        {
            break;
        }

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, fp);
        cinfo.image_width      = surf->width;
        cinfo.image_height     = surf->height;
        cinfo.input_components = 3;
        cinfo.in_color_space   = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);

        src = ituLockSurface(surf, 0, 0, surf->width, surf->height);
        assert(src != NULL);

        if (surf == ituGetDisplaySurface())
        {
            uint32_t addr;

            switch (ithLcdGetFlip())
            {
                case 0:
                    addr = ithLcdGetBaseAddrA();
                    break;

                case 1:
                    addr = ithLcdGetBaseAddrB();
                    break;

                default:
                    addr = ithLcdGetBaseAddrC();
                    break;
            }
            src = (uint8_t *)ithMapVram(addr, surf->lockSize, ITH_VRAM_READ);
        }

        if (surf->format == ITU_ARGB8888)
        {
            for (h = 0; h < surf->height; h++)
            {
                int       i, j;
                uint8_t * ptr = src + surf->width * 4 * h;

                // color trasform from ARGB8888 to RGB888
                for (i = (surf->width - 1) * 4, j = (surf->width - 1) * 3;
                     i >= 0 && j >= 0; i -= 4, j -= 3)
                {
                    image_buffer[surf->width * h * 3 + j + 0] = ptr[i + 2];
                    image_buffer[surf->width * h * 3 + j + 1] = ptr[i + 1];
                    image_buffer[surf->width * h * 3 + j + 2] = ptr[i + 0];
                }
            }
        }
        else if (surf->format == ITU_RGB565)
        {
            for (h = 0; h < surf->height; h++)
            {
                int       i, j;
                uint8_t * ptr = src + surf->width * 2 * h;

                // color trasform from RGB565 to RGB888
                for (i = (surf->width - 1) * 2, j = (surf->width - 1) * 3;
                     i >= 0 && j >= 0; i -= 2, j -= 3)
                {
                    image_buffer[surf->width * h * 3 + j + 0] =
                        ((ptr[i + 1]) & 0xf8) + ((ptr[i + 1] >> 5) & 0x07);
                    image_buffer[surf->width * h * 3 + j + 1] =
                        ((ptr[i + 0] >> 3) & 0x1c) +
                        ((ptr[i + 1] << 5) & 0xe0) + ((ptr[i + 1] >> 1) & 0x3);
                    image_buffer[surf->width * h * 3 + j + 2] =
                        ((ptr[i + 0] << 3) & 0xf8) + ((ptr[i + 0] >> 2) & 0x07);
                }
            }
        }

        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] =
                &image_buffer[cinfo.next_scanline * surf->width * 3];
            (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        ituUnlockSurface(surf);
    } while (false);

    if (image_buffer != NULL)
    {
        free(image_buffer);
    }

    if (fp != NULL)
    {
        fclose(fp);
    }
}

#endif // !defined(_WIN32) && defined(CFG_JPEG_HW_ENABLE)
