#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "jpg/ite_jpg.h"
#include "filter_mjpegdec_ngpl.h"

typedef struct JPG_DECODER_TAG
{
    uint32_t        framePitchY;
    uint32_t        framePitchUV;
    uint32_t        frameWidth;
    uint32_t        frameHeight;
    uint32_t        frameBufCount;
    uint32_t        currDisplayFrameBufIndex;
    uint32_t        OutAddrY[2];
    uint32_t        OutAddrU[2];
    uint32_t        OutAddrV[2];
    uint8_t *       DisplayAddrY;
    uint8_t *       DisplayAddrU;
    uint8_t *       DisplayAddrV;
    JPG_COLOR_SPACE colorFmt;
} JPG_DECODER;

static HJPG *        pHJpeg         = 0;
static JPG_DECODER * gptJPG_DECODER = NULL;
static uint32_t      Jbuf_vram_addr = 0;
static uint8_t *     Jbuf_sys_addr  = NULL;

static void          _mjpeg_decode_display (void)
{
    uint32_t frame_width, frame_height, frame_PitchY, frame_PitchUV;

    frame_width   = gptJPG_DECODER->frameWidth;
    frame_height  = gptJPG_DECODER->frameHeight;
    frame_PitchY  = gptJPG_DECODER->framePitchY;
    frame_PitchUV = gptJPG_DECODER->framePitchUV;

    if (!Jbuf_sys_addr)
    {
        Jbuf_vram_addr = itpVmemAlignedAlloc(
            32, (frame_PitchY * frame_height * 4)); // for YUV422
        if (!Jbuf_vram_addr)
        {
            printf("Jbuf_sys_addr Alloc Buffer Fail!!\n");
        }

        Jbuf_sys_addr = (uint8_t *)ithMapVram(
            Jbuf_vram_addr, (frame_PitchY * frame_height * 4), ITH_VRAM_WRITE);
        gptJPG_DECODER->frameBufCount            = 0;
        gptJPG_DECODER->currDisplayFrameBufIndex = 0;
    }

    if (!gptJPG_DECODER->frameBufCount)
    {
        gptJPG_DECODER->OutAddrY[0] = Jbuf_sys_addr;
        gptJPG_DECODER->OutAddrU[0] =
            gptJPG_DECODER->OutAddrY[0] + (frame_PitchY * frame_height);
        gptJPG_DECODER->OutAddrV[0] =
            gptJPG_DECODER->OutAddrU[0] + (frame_PitchUV * frame_height);

        gptJPG_DECODER->OutAddrY[1] =
            gptJPG_DECODER->OutAddrV[0] + (frame_PitchUV * frame_height);
        gptJPG_DECODER->OutAddrU[1] =
            gptJPG_DECODER->OutAddrY[1] + (frame_PitchY * frame_height);
        gptJPG_DECODER->OutAddrV[1] =
            gptJPG_DECODER->OutAddrU[1] + (frame_PitchUV * frame_height);
        gptJPG_DECODER->frameBufCount = 2;
    }

    if (gptJPG_DECODER->frameBufCount == 1)
    {
        gptJPG_DECODER->currDisplayFrameBufIndex = 0;
    }

    switch (gptJPG_DECODER->currDisplayFrameBufIndex)
    {
        case 0:
            gptJPG_DECODER->DisplayAddrY = gptJPG_DECODER->OutAddrY[0];
            gptJPG_DECODER->DisplayAddrU = gptJPG_DECODER->OutAddrU[0];
            gptJPG_DECODER->DisplayAddrV = gptJPG_DECODER->OutAddrV[0];
            break;

        case 1:
            gptJPG_DECODER->DisplayAddrY = gptJPG_DECODER->OutAddrY[1];
            gptJPG_DECODER->DisplayAddrU = gptJPG_DECODER->OutAddrU[1];
            gptJPG_DECODER->DisplayAddrV = gptJPG_DECODER->OutAddrV[1];
            break;
    }

    if (gptJPG_DECODER->currDisplayFrameBufIndex >= 1)
    {
        gptJPG_DECODER->currDisplayFrameBufIndex = 0;
    }
    else
    {
        gptJPG_DECODER->currDisplayFrameBufIndex++;
    }
}

static void mjpeg_dec_init (IteFilter * f)
{
    JPG_ERR        jpgRst    = JPG_ERR_OK;
    JPG_INIT_PARAM initParam = {0};

    if (NULL == gptJPG_DECODER)
    {
        gptJPG_DECODER = (JPG_DECODER *)calloc(
            sizeof(char), sizeof(JPG_DECODER)); // for jpg engine
    }

    initParam.codecType     = JPG_CODEC_DEC_MJPG;
    initParam.decType       = JPG_DEC_PRIMARY;
    initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;

    initParam.dispMode      = JPG_DISP_CENTER;

    initParam.width         = 320;
    initParam.height        = 240;

    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

    itv_set_pb_mode(1);
}

static void mjpeg_dec_uninit (IteFilter * f)
{
    JPG_ERR result = JPG_ERR_OK;

    itv_set_pb_mode(0);

    iteJpg_DestroyHandle(&pHJpeg, 0);
    pHJpeg = 0;
    if (Jbuf_sys_addr)
    {
        itpVmemFree(Jbuf_vram_addr);
        Jbuf_sys_addr  = NULL;
        Jbuf_vram_addr = 0;
    }
    if (gptJPG_DECODER)
    {
        free(gptJPG_DECODER);
        gptJPG_DECODER = NULL;
    }
}

static void mjpeg_dec_process (IteFilter * f)
{
    YUV_FRAME   orig        = {0};
    IteQueueblk blk         = {0};
    IteQueueblk blk_output0 = {0};
    IteQueueblk blk_output1 = {0};
    mblk_ite *  m           = NULL;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            JPG_ERR         jpgRst             = JPG_ERR_OK;
            JPG_RECT        destRect           = {0};

            JPG_STREAM_INFO inStreamInfo       = {0};
            JPG_STREAM_INFO outStreamInfo      = {0};
            JPG_BUF_INFO    entropyBufInfo     = {0};
            JPG_USER_INFO   jpgUserInfo        = {0};

            m                                  = (mblk_ite *)blk.datap;

            // ------------------------------------
            // set src type
            inStreamInfo.streamIOType          = JPG_STREAM_IO_READ;
            inStreamInfo.streamType            = JPG_STREAM_MEM;
            inStreamInfo.jstream.mem[0].pAddr  = m->b_rptr;
            inStreamInfo.jstream.mem[0].length = m->b_wptr - m->b_rptr;
            inStreamInfo.validCompCnt          = 1;
            iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, 0, 0);

            // ------------------------------------
            // parsing Header
            jpgRst = iteJpg_Parsing(pHJpeg, &entropyBufInfo, (void *)&destRect);

            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            // ----------------------------------------
            // get output YUV plan buffer
            iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

            gptJPG_DECODER->frameHeight  = jpgUserInfo.real_height;
            gptJPG_DECODER->frameWidth   = jpgUserInfo.real_width;
            gptJPG_DECODER->framePitchY  = jpgUserInfo.comp1Pitch;
            gptJPG_DECODER->framePitchUV = jpgUserInfo.comp23Pitch;
            gptJPG_DECODER->colorFmt     = jpgUserInfo.colorFormate;

            outStreamInfo.streamIOType   = JPG_STREAM_IO_WRITE;
            outStreamInfo.streamType     = JPG_STREAM_MEM;

            {
                _mjpeg_decode_display();

                outStreamInfo.jstream.mem[0].pAddr =
                    gptJPG_DECODER->DisplayAddrY; // get output buf;
                outStreamInfo.jstream.mem[0].pitch =
                    gptJPG_DECODER->framePitchY;
                outStreamInfo.jstream.mem[0].length =
                    gptJPG_DECODER->framePitchY * gptJPG_DECODER->frameHeight;
                // U
                outStreamInfo.jstream.mem[1].pAddr =
                    gptJPG_DECODER->DisplayAddrU;
                outStreamInfo.jstream.mem[1].pitch =
                    gptJPG_DECODER->framePitchUV;
                outStreamInfo.jstream.mem[1].length =
                    gptJPG_DECODER->framePitchUV * gptJPG_DECODER->frameHeight;
                // V
                outStreamInfo.jstream.mem[2].pAddr =
                    gptJPG_DECODER->DisplayAddrV;
                outStreamInfo.jstream.mem[2].pitch =
                    gptJPG_DECODER->framePitchUV;
                outStreamInfo.jstream.mem[2].length =
                    gptJPG_DECODER->framePitchUV * gptJPG_DECODER->frameHeight;
            }

            // printf("\n\tY=0x%x, u=0x%x, v=0x%x\n",
            // outStreamInfo.jstream.mem[0].pAddr,
            // outStreamInfo.jstream.mem[1].pAddr,
            // outStreamInfo.jstream.mem[2].pAddr);

            outStreamInfo.validCompCnt = 3;
            jpgRst = iteJpg_SetStreamInfo(pHJpeg, 0, &outStreamInfo, 0);
            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            // ------------------------------
            // setup jpg
            jpgRst = iteJpg_Setup(pHJpeg, 0);
            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            // ------------------------------
            // fire H/W jpg
            jpgRst = iteJpg_Process(pHJpeg, &entropyBufInfo, 0, 0);
            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
            // printf("\n\tresult = %d\n", jpgUserInfo.status);

            jpgRst = iteJpg_WaitIdle(pHJpeg, 0);
            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            jpgRst = iteJpg_Reset(pHJpeg, 0);
            if (jpgRst != JPG_ERR_OK)
            {
                printf(" err (0x%x) !! %s [%d]\n", jpgRst, __FILE__, __LINE__);
            }

            orig.width         = gptJPG_DECODER->frameWidth;
            orig.height        = gptJPG_DECODER->frameHeight;
            orig.data[0]       = outStreamInfo.jstream.mem[0].pAddr;
            orig.data[1]       = outStreamInfo.jstream.mem[1].pAddr;
            orig.data[2]       = outStreamInfo.jstream.mem[2].pAddr;
            orig.linesize[0]   = outStreamInfo.jstream.mem[0].pitch;
            orig.linesize[1]   = outStreamInfo.jstream.mem[1].pitch;
            orig.format        = MMP_ISP_IN_YUV422;

            // printf("w = %d, h = %d, pitchY = %d, pitchUV = %d\n", orig.width,
            // orig.height, orig.linesize[0], orig.linesize[1]);

            mblk_ite * yuv_msg = NULL;
            yuv_msg            = allocb_ite(sizeof(YUV_FRAME));
            memcpy(yuv_msg->b_rptr, &orig, sizeof(YUV_FRAME));
            yuv_msg->b_wptr += sizeof(YUV_FRAME);
            blk_output0.datap = yuv_msg;
            ite_queue_put(f->output[0].Qhandle, &blk_output0);

            if (f->output[1].Qhandle)
            {
                mblk_ite * yuv_data = NULL;
                yuv_data            = allocb_ite(sizeof(YUV_FRAME));
                memcpy(yuv_data->b_rptr, &orig, sizeof(YUV_FRAME));
                yuv_data->b_wptr += sizeof(YUV_FRAME);
                blk_output1.datap = yuv_data;
                ite_queue_put(f->output[1].Qhandle, &blk_output1);
            }
            freemsg_ite(m);
            m = NULL;
        }
        usleep(1000);
    }
}

// clang-format off
IteFilterDes FilterMJpegDEC = {
    ITE_FILTER_MJPEGDEC_ID,
    mjpeg_dec_init,
    mjpeg_dec_uninit,
    mjpeg_dec_process,
};
// clang-format on
