#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "jpegscale/jpegscale.h"
#include "jpg/ite_jpg.h"
#include "isp/mmp_isp.h"
#include "ite/itp.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define MAX_JPEG_DECODE_SIZE 36000000
#define JPEG_SOF_MARKER      0xFFC0
#define JPEG_SOS_MARKER      0xFFDA
#define JPEG_DHT_MARKER      0xFFC4
#define JPEG_DRI_MARKER      0xFFDD
#define JPEG_DQT_MARKER      0xFFDB
#define JPEG_APP00_MARKER    0xFFE0
#define JPEG_APP01_MARKER    0xFFE1
#define JPEG_APP02_MARKER    0xFFE2
#define JPEG_APP03_MARKER    0xFFE3
#define JPEG_APP04_MARKER    0xFFE4
#define JPEG_APP05_MARKER    0xFFE5
#define JPEG_APP06_MARKER    0xFFE6
#define JPEG_APP07_MARKER    0xFFE7
#define JPEG_APP08_MARKER    0xFFE8
#define JPEG_APP09_MARKER    0xFFE9
#define JPEG_APP10_MARKER    0xFFEA
#define JPEG_APP11_MARKER    0xFFEB
#define JPEG_APP12_MARKER    0xFFEC
#define JPEG_APP13_MARKER    0xFFED
#define JPEG_APP14_MARKER    0xFFEE
#define JPEG_APP15_MARKER    0xFFEF
#define JPEG_COM_MARKER      0xFFFE

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
    DATA_COLOR_CNT
} DATA_COLOR_TYPE;

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct _BASE_RECT_TAG
{
    int x;
    int y;
    int w;
    int h;
} BASE_RECT;

//=============================================================================
//                Global Data Definition
//=============================================================================
static ISP_DEVICE gIspDev = NULL;

//=============================================================================
//                Private Function Definition
//=============================================================================

/**
 * @brief This function is used to do color space conversion from YUV to RGB
 * using the ISP.
 *
 * @param srcAddr_rgby The address of the input YUV data. The Y component is
 *                     stored in the first byte, the U component is stored in
 *                     the second byte, and the V component is stored in the
 *                     third byte.
 * @param srcAddr_u    The address of the input U data.
 * @param srcAddr_v    The address of the input V data.
 * @param colorType    The type of the input data, can be DATA_COLOR_YUV444,
 *                     DATA_COLOR_YUV422, DATA_COLOR_YUV422R, DATA_COLOR_YUV420,
 *                     DATA_COLOR_NV12, DATA_COLOR_NV21, DATA_COLOR_RGB565,
 *                     DATA_COLOR_ARGB8888, or DATA_COLOR_ARGB4444.
 * @param srcRect      The rectangle of the input data.
 * @param destRect     The rectangle of the output data.
 * @param imgWidth     The width of the input image.
 * @param imgHeight    The height of the input image.
 * @param destRectBG   The rectangle of the background of the output data.
 * @param dest         The address of the output data.
 * @param format       The format of the output data, can be
 * JPEGSCALE_FORMAT_YUV422 or JPEGSCALE_FORMAT_YUV420.
 *
 * @return None.
 */
static void set_isp_colorTrans (uint8_t * srcAddr_rgby, uint8_t * srcAddr_u,
                                uint8_t * srcAddr_v, DATA_COLOR_TYPE colorType,
                                BASE_RECT * srcRect, JPG_RECT * destRect,
                                uint32_t imgWidth, uint32_t imgHeight,
                                JPG_RECT * destRectBG, JPEGSCALE_TEMP dest,
                                JPEGSCALE_FORMAT format)
{
    uint32_t            result = 0;
    uint32_t            width = 0, height = 0;
    MMP_ISP_OUTPUT_INFO outInfo  = {0};
    MMP_ISP_SHARE       ispInput = {0};
#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    MMP_ISP_CORE_INFO ISPCOREINFO = {0};
#endif

    // Set the address of the input data
    ispInput.width        = imgWidth;
    ispInput.height       = imgHeight;
    ispInput.isAdobe_CMYK = 0;

    // Set the address of the input YUV data
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
                default:
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
        default:
            return;
    }

#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    // Get the width and height of the output data
    width  = destRect->w; // dispWidth;
    height = destRect->h; // dispHeight;

    // Initialize the ISP
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
        (void)printf("mmpIspInitialize() error (0x%x) !!\n",
                     (unsigned int)result);
    }

    // Set the core information
    // for VP1
    ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
    // end of for VP1.

    result                  = mmpIspSetCore(gIspDev, &ISPCOREINFO);
    if (result)
    {
        (void)printf("mmpIspSetCore() error (0x%x) !!\n", (unsigned int)result);
    }

    result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
    if (result)
    {
        (void)printf("mmpIspSetMode() error (0x%x) !! \n",
                     (unsigned int)result);
    }

    // Set the output window
    outInfo.startX = 0;
    outInfo.startY = 0;
    outInfo.width  = destRectBG->w;
    outInfo.height = destRectBG->h;
    if (format == JPEGSCALE_FORMAT_YUV422)
    {
        outInfo.addrRGB = (uint32_t)dest;
        outInfo.addrY   = (uint32_t)dest;
        outInfo.addrU   = (uint32_t)dest + destRectBG->w * destRectBG->h;
        outInfo.addrV   = (uint32_t)dest + destRectBG->w * destRectBG->h +
                        destRectBG->w * destRectBG->h / 2;
        outInfo.format = MMP_ISP_OUT_YUV422;
    }
    else if (format == JPEGSCALE_FORMAT_YUV420)
    {
        outInfo.addrRGB = (uint32_t)dest;
        outInfo.addrY   = (uint32_t)dest;
        outInfo.addrU   = (uint32_t)dest + destRectBG->w * destRectBG->h;
        outInfo.addrV   = (uint32_t)dest + destRectBG->w * destRectBG->h +
                        destRectBG->w * destRectBG->h / 4;
        outInfo.format = MMP_ISP_OUT_YUV420;
    }
    outInfo.pitchRGB = (uint16_t)destRectBG->w;
    outInfo.pitchY   = destRectBG->w;
    outInfo.pitchUv  = destRectBG->w / 2;

    // Set the output window
    mmpIspSetOutputWindow(gIspDev, &outInfo);

    // Set the video window
    mmpIspSetVideoWindow(gIspDev, 0, 0, width, height);

    // Lock the ISP core
    #if (CFG_CHIP_FAMILY != 970)
        // #ifdef CFG_LCD_PQ_TUNING
        #if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_lock(&ISP_CORE_0_MUTEX);
        #endif
    #endif

    // Play the image process
    result = mmpIspPlayImageProcess(gIspDev, &ispInput);
    if (result)
    {
        (void)printf("mmpIspPlayImageProcess() error (0x%x) !!\n",
                     (unsigned int)result);
    }

    // Wait for the engine to idle
    result = mmpIspWaitEngineIdle(gIspDev);
    if (result)
    {
        (void)printf("mmpIspWaitEngineIdle() error (0x%x) !!\n",
                     (unsigned int)result);
    }

    // Unlock the ISP core
    #if (CFG_CHIP_FAMILY != 970)
        // #ifdef CFG_LCD_PQ_TUNING
        #if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
    pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
        #endif
    #endif

    // Terminate the ISP
    mmpIspTerminate(&gIspDev);
#endif
}

/**
 * @brief This function is used to load a JPEG image and scale it to the desired
 * size. The output data is stored in a memory buffer and the address is
 * returned in temp.
 *
 * @param widthPercent          The percentage to scale the width of the image.
 * @param heightPercent         The percentage to scale the height of the image.
 * @param data                  The JPEG image data.
 * @param size                  The size of the JPEG image data.
 * @param format                The format of the output data, can be
 *                               JPEGSCALE_FORMAT_YUV422 or
 * JPEGSCALE_FORMAT_YUV420.
 * @param temp                  The address of the output data.
 * @param width                 The width of the output data.
 * @param height                The height of the output data.
 * @param header_width          The width of the header of the output data.
 * @param header_height         The height of the header of the output data.
 *
 * @return The result of the function, JPEGSCALE_SUCCESS if successful or
 *         JPEGSCALE_ERR_JPEG_OPERATION if the JPEG operation failed.
 */
static JPEGSCALE_RESULT JpegLoad (float widthPercent, float heightPercent,
                                  uint8_t * data, int size,
                                  JPEGSCALE_FORMAT format,
                                  JPEGSCALE_TEMP * temp, uint32_t * width,
                                  uint32_t * height, uint32_t * header_width,
                                  uint32_t * header_height)
{
    JPEGSCALE_RESULT result         = JPEGSCALE_SUCCESS;
    HJPG *           pHJpeg         = 0;
    JPG_INIT_PARAM   initParam      = {0};
    JPG_STREAM_INFO  inStreamInfo   = {0};
    JPG_STREAM_INFO  outStreamInfo  = {0};
    JPG_BUF_INFO     entropyBufInfo = {0};
    JPG_USER_INFO    jpgUserInfo    = {0};
    uint8_t *        pY             = 0;
    JPG_RECT         destRect       = {0};
    JPG_RECT         destRect_BC    = {0};
    BASE_RECT        srcRect        = {0};
    DATA_COLOR_TYPE  colorType      = 0;
    uint32_t         real_width = 0, real_height = 0;
    uint32_t         imgWidth = 0, imgHeight = 0;
    uint32_t         dispWidth = 0, dispHeight = 0;
    uint32_t         H_Samp = 0, V_Samp = 0;
    uint8_t *        pStart = 0, *pCur = 0, *pEnd = 0;
    uint32_t         CurCount = 0, MarkerType = 0, GetMarkerLength = 0;
    uint32_t         p_index = 0;
    uint8_t *        t_index = 0;
    JPG_ERR          jpg_result;

    if (data[0] != 0xFF || data[1] != 0xD8)
    {
        (void)printf("[%s] jpeg read stream fail,data[0]=0x%x,data[1]=0x%x\n",
                     __func__, data[0], data[1]);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
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
    pCur       = pStart;

    *width     = (((uint32_t)(imgWidth * widthPercent / 100) + 15) / 16) * 16;
    *height    = (((uint32_t)(imgHeight * heightPercent / 100) + 15) / 16) * 16;
    dispWidth  = (((uint32_t)(imgWidth * widthPercent / 100) + 3) / 4) * 4;
    dispHeight = (((uint32_t)(imgHeight * heightPercent / 100) + 3) / 4) * 4;
    *header_width  = (uint32_t)(imgWidth * widthPercent / 100);
    *header_height = (uint32_t)(imgHeight * heightPercent / 100);
    (void)printf(
        "imgWidth:%u, imgHeight:%u, header_width:%u, header_height:%u, "
        "width:%u, height:%u, dispWidth:%u, dispHeight:%u\n",
        (unsigned int)imgWidth, (unsigned int)imgHeight,
        (unsigned int)*header_width, (unsigned int)*header_height,
        (unsigned int)*width, (unsigned int)*height, (unsigned int)dispWidth,
        (unsigned int)dispHeight);

    if (!*width || !*height || !dispWidth || !dispHeight)
    {
        (void)printf("[%s] parameter error of JPEG scaling\n", __func__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }

    *temp = (JPEGSCALE_TEMP)itpVmemAlignedAlloc(32, (*width) * (*height) * 2);
    if (!*temp)
    {
        (void)printf("[%s] memory allocation fail\n", __func__);
        result = JPEGSCALE_ERR_MEM_ALLOCATION;
        goto end;
    }

    if ((imgWidth * imgHeight >= MAX_JPEG_DECODE_SIZE) || imgWidth >= 4096 ||
        imgHeight >= 4096)
    {
        (void)printf("[%s] JPG not support this format\n", __func__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }

    initParam.codecType     = JPG_CODEC_DEC_JPG_CMD;
    initParam.decType       = JPG_DEC_PRIMARY;
    initParam.dispMode      = JPG_DISP_FIT;
    initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;
    initParam.width         = dispWidth;
    initParam.height        = dispHeight;
    jpg_result              = iteJpg_CreateHandle(&pHJpeg, &initParam, 0);
    if (jpg_result != JPG_ERR_OK)
    {
        goto end;
    }
    inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
    inStreamInfo.streamType            = JPG_STREAM_MEM;
    inStreamInfo.jpg_reset_stream_info = 0;
    inStreamInfo.validCompCnt          = 1;
    inStreamInfo.jstream.mem[0].pAddr  = data;
    inStreamInfo.jstream.mem[0].length = size;
    jpg_result = iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);
    if (jpg_result != JPG_ERR_OK)
    {
        goto end;
    }
    if (iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect) ==
        JPG_ERR_JPROG_STREAM)
    {
        (void)printf("[%s] JPG not support this format\n", __func__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }
    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    real_width  = jpgUserInfo.real_width;
    real_height = jpgUserInfo.real_height;
    pY = (uint8_t *)itpVmemAlignedAlloc(32, real_width * real_height * 3);
    if (!pY)
    {
        (void)printf("[%s] memory allocation fail\n", __func__);
        result = JPEGSCALE_ERR_MEM_ALLOCATION;
        goto end;
    }
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
    (void)printf("dec_out w:%d,h:%d\n", (unsigned int)real_width,
                 (unsigned int)real_height);
    (void)printf(
        "dec_out a1:%p,a2:%p,a3:%p\n", outStreamInfo.jstream.mem[0].pAddr,
        outStreamInfo.jstream.mem[1].pAddr, outStreamInfo.jstream.mem[2].pAddr);
    (void)printf("dec_out p1:%d,p2:%d,p3:%d\n",
                 (unsigned int)outStreamInfo.jstream.mem[0].pitch,
                 (unsigned int)outStreamInfo.jstream.mem[1].pitch,
                 (unsigned int)outStreamInfo.jstream.mem[2].pitch);
    outStreamInfo.streamIOType = JPG_STREAM_IO_WRITE;
    outStreamInfo.streamType   = JPG_STREAM_MEM;
    if (iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0) != JPG_ERR_OK)
    {
        (void)printf("[%s] jpeg err !%s [%d]\n", __func__, __FILE__, __LINE__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }
    if (iteJpg_Setup(pHJpeg, 0) != JPG_ERR_OK)
    {
        (void)printf("[%s] jpeg err !%s [%d]\n", __func__, __FILE__, __LINE__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }
    if (pY)
    {
        ithFlushDCacheRange((void *)pY, real_width * real_height * 3);
        ithFlushMemBuffer();
    }
    if (iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0) != JPG_ERR_OK)
    {
        (void)printf("[%s] jpeg err !%s [%d]\n", __func__, __FILE__, __LINE__);
        result = JPEGSCALE_ERR_JPEG_OPERATION;
        goto end;
    }
    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
    iteJpg_DestroyHandle(&pHJpeg, 0);
    srcRect.w = real_width;
    srcRect.h = real_height;
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
        default:
            break;
    }

    if (imgWidth % 4)
    {
        for (p_index = 0; p_index < real_width * real_height;
             p_index += outStreamInfo.jstream.mem[0].pitch)
        {
            t_index = outStreamInfo.jstream.mem[0].pAddr + p_index;
            memset(t_index + imgWidth, 0xff, 4 - imgWidth % 4);
        }
    }

    if (imgHeight % 4)
    {
        t_index = outStreamInfo.jstream.mem[0].pAddr +
                  imgHeight * outStreamInfo.jstream.mem[0].pitch;
        for (p_index = 0; p_index < 4 - imgHeight % 4; p_index++)
        {
            memset(t_index, 0xff, outStreamInfo.jstream.mem[0].pitch);
            t_index += outStreamInfo.jstream.mem[0].pitch;
        }
    }

    if (imgWidth % 4 || imgHeight % 4)
    {
        ithFlushDCacheRange(outStreamInfo.jstream.mem[0].pAddr,
                            real_width * real_height);
    }

    destRect_BC.w = *width;
    destRect_BC.h = *height;
    set_isp_colorTrans(outStreamInfo.jstream.mem[0].pAddr,
                       outStreamInfo.jstream.mem[1].pAddr,
                       outStreamInfo.jstream.mem[2].pAddr, colorType, &srcRect,
                       &destRect, ((imgWidth + 3) / 4) * 4,
                       ((imgHeight + 3) / 4) * 4, &destRect_BC, *temp, format);

    if (pY)
    {
        itpVmemFree((uint32_t)pY);
    }

    return result;
end:
    if (pY)
    {
        itpVmemFree((uint32_t)pY);
    }
    if (pHJpeg)
    {
        iteJpg_DestroyHandle(&pHJpeg, 0);
    }

    return result;
}

//=============================================================================
//                Public Function Definition
//=============================================================================
JPEGSCALE_RESULT
JpegScale(char * inputFile, char * outputFile, float widthPercent,
          float heightPercent, JPEGSCALE_FORMAT format)
{
    JPEGSCALE_RESULT result = JPEGSCALE_SUCCESS;
    JPEGSCALE_TEMP   temp   = NULL;
    uint32_t         width = 0, height = 0;
    uint32_t         header_width = 0, header_height = 0;

    do
    {
        if ((inputFile == NULL) || (outputFile == NULL) ||
            (widthPercent <= 0) || (heightPercent <= 0) ||
            (format > JPEGSCALE_FORMAT_YUV420))
        {
            (void)printf("[%s] parameter fail\n", __func__);
            result = JPEGSCALE_ERR_INVALID_INPUT_PARAM;
            break;
        }

        if ((result = JpegLoadFile(
                 inputFile, widthPercent, heightPercent, format, &temp, &width,
                 &height, &header_width, &header_height)) != JPEGSCALE_SUCCESS)
        {
            (void)printf("[%s:%d] JpegLoadFile fail, result=%d\n", __func__,
                         __LINE__, (unsigned int)result);
            break;
        }

        if ((result = JpegSaveFile(outputFile, format, temp, width, height,
                                   header_width, header_height)) !=
            JPEGSCALE_SUCCESS)
        {
            (void)printf("[%s:%d] JpegLoadFile fail, result=%d\n", __func__,
                         __LINE__, (unsigned int)result);
            break;
        }
    } while (false);

    if (temp != NULL)
    {
        itpVmemFree((uint32_t)temp);
    }

    return result;
}

JPEGSCALE_RESULT
JpegLoadFile(char * inputFile, float widthPercent, float heightPercent,
             JPEGSCALE_FORMAT format, JPEGSCALE_TEMP * temp, uint32_t * width,
             uint32_t * height, uint32_t * header_width,
             uint32_t * header_height)
{
    JPEGSCALE_RESULT result = JPEGSCALE_SUCCESS;
    FILE *           f      = NULL;
    int              size = 0, size_real = 0;
    uint8_t *        data = NULL;
    struct stat      sb   = {0};

    do
    {
        if (!inputFile || widthPercent <= 0 || heightPercent <= 0 ||
            format > JPEGSCALE_FORMAT_YUV420 || *temp || *width || *height ||
            *header_width || *header_height)
        {
            (void)printf("[%s] parameter fail\n", __func__);
            result = JPEGSCALE_ERR_INVALID_INPUT_PARAM;
            break;
        }

        f = fopen(inputFile, "rb");
        if (f == NULL)
        {
            (void)printf("[%s] file operation fail\n", __func__);
            result = JPEGSCALE_ERR_FILE_OPERATION;
            break;
        }

        if (fstat(fileno(f), &sb) == -1)
        {
            (void)printf("[%s] file operation fail\n", __func__);
            result = JPEGSCALE_ERR_FILE_OPERATION;
            break;
        }

        size = sb.st_size;
        data = malloc(size);
        if (data == NULL)
        {
            (void)printf("[%s] memory allocation fail\n", __func__);
            result = JPEGSCALE_ERR_MEM_ALLOCATION;
            break;
        }

        size_real = fread(data, 1, size, f);
        if (size != size_real)
        {
            (void)printf("[%s] file operation fail\n", __func__);
            result = JPEGSCALE_ERR_FILE_OPERATION;
            break;
        }

        if ((result = JpegLoad(widthPercent, heightPercent, data, size_real,
                               format, temp, width, height, header_width,
                               header_height)) != JPEGSCALE_SUCCESS)
        {
            (void)printf("[%s] JpegLoadFile fail, result=%d\n", __func__,
                         (unsigned int)result);
            break;
        }
    } while (false);

    if (data != NULL)
    {
        free(data);
    }
    if (f != NULL)
    {
        fclose(f);
    }

    return result;
}

JPEGSCALE_RESULT
JpegSaveFile(char * outputFile, JPEGSCALE_FORMAT format, JPEGSCALE_TEMP temp,
             uint32_t width, uint32_t height, uint32_t header_width,
             uint32_t header_height)
{
    JPEGSCALE_RESULT result         = JPEGSCALE_SUCCESS;
    uint8_t *        yuv_data       = NULL;
    HJPG *           pHJpeg         = NULL;
    JPG_INIT_PARAM   initParam      = {0};
    JPG_STREAM_INFO  inStreamInfo   = {0};
    JPG_STREAM_INFO  outStreamInfo  = {0};
    JPG_BUF_INFO     entropyBufInfo = {0};
    JPG_USER_INFO    jpgUserInfo    = {0};
    uint32_t         jpgEncSize     = 0;
    JPG_ERR          jpg_result;

    unsigned char *  pAddr_y = NULL, *pAddr_u = NULL, *pAddr_v = NULL;

    do
    {
        if ((outputFile == NULL) || (format > JPEGSCALE_FORMAT_YUV420) ||
            (temp == NULL) || (width == 0) || (height == 0) ||
            (header_width == 0) || (header_height == 0))
        {
            (void)printf("[%s] parameter fail\n", __func__);
            result = JPEGSCALE_ERR_INVALID_INPUT_PARAM;
            break;
        }

        (void)printf("[%s] s_w:%d, s_h:%d, h_w:%d, h_h:%d\n", __func__,
                     (int)width, (int)height, (int)header_width,
                     (int)header_height);
        yuv_data = temp;
        (void)printf("[%s] ituJpegSaveFile_QR yuv_data:%p\n", __func__,
                     yuv_data);

        pAddr_y = yuv_data;
        pAddr_u = yuv_data + (width * height);
        if (format == JPEGSCALE_FORMAT_YUV422)
        {
            pAddr_v                 = pAddr_u + (width * height / 2);
            initParam.outColorSpace = JPG_COLOR_SPACE_YUV422;
        }
        else if (format == JPEGSCALE_FORMAT_YUV420)
        {
            pAddr_v                 = pAddr_u + (width * height / 4);
            initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;
        }
        initParam.codecType     = JPG_CODEC_ENC_JPG;
        initParam.width         = width;
        initParam.height        = height;
        initParam.header_width  = header_width;
        initParam.header_height = header_height;
        initParam.encQuality    = 70; // 85;
        jpg_result              = iteJpg_CreateHandle(&pHJpeg, &initParam, 0);
        if (jpg_result != JPG_ERR_OK)
        {
            break;
        }
        inStreamInfo.streamIOType           = JPG_STREAM_IO_READ;
        inStreamInfo.streamType             = JPG_STREAM_MEM;
        // Y
        inStreamInfo.jstream.mem[0].pAddr   = (uint8_t *)pAddr_y;
        inStreamInfo.jstream.mem[0].pitch   = width;
        // U
        inStreamInfo.jstream.mem[1].pAddr   = (uint8_t *)pAddr_u;
        inStreamInfo.jstream.mem[1].pitch   = width / 2;
        // V
        inStreamInfo.jstream.mem[2].pAddr   = (uint8_t *)pAddr_v;
        inStreamInfo.jstream.mem[2].pitch   = width / 2;
        inStreamInfo.validCompCnt           = 3;
        outStreamInfo.streamType            = JPG_STREAM_FILE;
        outStreamInfo.jstream.path          = (void *)outputFile;
        outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
        outStreamInfo.jpg_reset_stream_info = 0;
        jpg_result = iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo, 0);
        if (jpg_result != JPG_ERR_OK)
        {
            break;
        }
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        jpg_result = iteJpg_Setup(pHJpeg, 0);
        if (jpg_result != JPG_ERR_OK)
        {
            break;
        }
        jpg_result = iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);
        if (jpg_result != JPG_ERR_OK)
        {
            break;
        }
        iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
        (void)printf("[%s] tresult = %d, encode size = %f KB\n", __func__,
                     jpgUserInfo.status, (float)jpgEncSize / 1024);
    } while (false);

    if (pHJpeg != NULL)
    {
        iteJpg_DestroyHandle(&pHJpeg, 0);
    }

    return result;
}
