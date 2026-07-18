#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "ite/ith.h"

/*
This file is primarily used with the capture filter to achieve
the simultaneous display of two camera feeds with different zoom levels.

The configuration option CFG_ITV_VP_DUAL_WINDOWS needs to be enabled.
*/
//=============================================================================
//                              Constant Definition
//=============================================================================
static VideoInputWindowsInfo * WinInfo    = NULL;
static VideoInputWindowsInfo * WinInfo2nd = NULL;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static bool                    check_parameter (VideoInputWindowsInfo * WinInfo,
                                                IteSensorStream *       sensorinfo)
{
    bool result = true;

    if ((WinInfo->x_offset + WinInfo->width) > sensorinfo->Width)
    {
        printf("WinInfo x_offset + width error\n");
        result = false;
    }

    if ((WinInfo->y_offset + WinInfo->height) > sensorinfo->Height)
    {
        printf("WinInfo y_offset + height error\n");
        result = false;
    }
    if (result == false)
    {
        DEBUG_PRINT("[%s] Filter(%d) ERROR \n", __FUNCTION__, f->filterDes.id);
    }
    return result;
}

static void displaydual_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    itv_set_pb_mode(1);
}

static void displaydual_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    itv_set_pb_mode(0);
}

static void displaydual_process (IteFilter * f)
{
    IteQueueblk       rec_blk0   = {0};
    IteSensorStream * sensorinfo = NULL;
    ITV_DBUF_PROPERTY dispProp   = {0};
    uint8_t *         dbuf       = NULL;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        // printf("[%s] Filter(%d). thread run\n", __FUNCTION__,
        // f->filterDes.id);
        if (ite_queue_get(f->input[0].Qhandle, &rec_blk0) == 0)
        {

            sensorinfo = (IteSensorStream *)rec_blk0.private1;

            dbuf       = itv_get_dbuf_anchor();

            if (dbuf != NULL && sensorinfo != NULL)
            {
                if (check_parameter(WinInfo, sensorinfo))
                {
                    dispProp.src_w = WinInfo->width;
                    dispProp.src_h = WinInfo->height;
                    dispProp.ya =
                        (uint8_t *)(sensorinfo->DataAddrY +
                                    (sensorinfo->PitchY * WinInfo->y_offset) +
                                    WinInfo->x_offset);
                    dispProp.ua = (uint8_t *)(sensorinfo->DataAddrU +
                                              (sensorinfo->PitchUV / 2 *
                                               WinInfo->y_offset) +
                                              WinInfo->x_offset);
                    dispProp.va = (uint8_t *)(sensorinfo->DataAddrV +
                                              (sensorinfo->PitchUV / 2 *
                                               WinInfo->y_offset) +
                                              WinInfo->x_offset);
                }
                else
                {
                    dispProp.src_w = sensorinfo->Width;
                    dispProp.src_h = sensorinfo->Height;
                    dispProp.ya    = (uint8_t *)sensorinfo->DataAddrY;
                    dispProp.ua    = (uint8_t *)sensorinfo->DataAddrU;
                    dispProp.va    = (uint8_t *)sensorinfo->DataAddrV;
                }
                dispProp.pitch_y  = sensorinfo->PitchY;
                dispProp.pitch_uv = sensorinfo->PitchUV;
                dispProp.format   = MMP_ISP_IN_NV12;
#ifdef CFG_ITV_VP_DUAL_WINDOWS
                if (WinInfo2nd != NULL)
                {
                    if (check_parameter(WinInfo2nd, sensorinfo))
                    {
                        dispProp.src_w_2nd = WinInfo2nd->width;
                        dispProp.src_h_2nd = WinInfo2nd->height;
                        dispProp.ya_2nd    = (uint8_t *)(sensorinfo->DataAddrY +
                                                      (sensorinfo->PitchY *
                                                       WinInfo2nd->y_offset) +
                                                      WinInfo2nd->x_offset);
                        dispProp.ua_2nd    = (uint8_t *)(sensorinfo->DataAddrU +
                                                      (sensorinfo->PitchUV / 2 *
                                                       WinInfo2nd->y_offset) +
                                                      WinInfo2nd->x_offset);
                        dispProp.va_2nd    = (uint8_t *)(sensorinfo->DataAddrV +
                                                      (sensorinfo->PitchUV / 2 *
                                                       WinInfo2nd->y_offset) +
                                                      WinInfo2nd->x_offset);
                    }
                    else
                    {
                        dispProp.src_w_2nd = sensorinfo->Width;
                        dispProp.src_h_2nd = sensorinfo->Height;
                        dispProp.ya_2nd    = (uint8_t *)sensorinfo->DataAddrY;
                        dispProp.ua_2nd    = (uint8_t *)sensorinfo->DataAddrU;
                        dispProp.va_2nd    = (uint8_t *)sensorinfo->DataAddrV;
                    }
                }
#endif
                itv_update_dbuf_anchor(&dispProp);
            }
            free(sensorinfo);
        }
        usleep(1000);
    }

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);

    // return NULL;
}

static void f_displaydual_setwindows (IteFilter * f, void * arg)
{
    WinInfo = (VideoInputWindowsInfo *)arg;
    printf("1st(%d, %d, %d, %d)\n", WinInfo->x_offset, WinInfo->y_offset,
           WinInfo->width, WinInfo->height);
    if (WinInfo->nextptr != NULL)
    {
        WinInfo2nd = (VideoInputWindowsInfo *)WinInfo->nextptr;
        printf("2nd(%d, %d, %d, %d)\n", WinInfo2nd->x_offset,
               WinInfo2nd->y_offset, WinInfo2nd->width, WinInfo2nd->height);
    }
}

static IteMethodDes FilterDisplayDual_methods[] = {
    {ITE_FILTER_DISPLAY_SET_INPUT_WINDOWS, f_displaydual_setwindows},
    {                                   0,                     NULL},
};

// clang-format off
IteFilterDes FilterDisplayDual = {
    ITE_FILTER_DISPLAY_DUAL_ID,
    displaydual_init,
    displaydual_uninit,
    displaydual_process,
    FilterDisplayDual_methods
};
// clang-format on
