#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "ite/ith.h"

/*

*/
//=============================================================================
//                              Constant Definition
//=============================================================================
static VideoInputWindowsInfo * WinInfo    = NULL;
static VideoInputWindowsInfo * WinInfo2nd = NULL;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static void                    displayosd_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    itv_set_pb_mode(1);
}

static void displayosd_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    itv_stop_vidSurf_anchor();
    itv_flush_dbuf();
    itv_set_pb_mode(0);
}

static void displayosd_process (IteFilter * f)
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

            if (dbuf != NULL)
            {
                dispProp.src_w    = sensorinfo->Width;
                dispProp.src_h    = sensorinfo->Height;
                dispProp.ya       = (uint8_t *)sensorinfo->DataAddrY;
                dispProp.ua       = (uint8_t *)sensorinfo->DataAddrU;
                dispProp.va       = (uint8_t *)sensorinfo->DataAddrV;
                dispProp.pitch_y  = sensorinfo->PitchY;
                dispProp.pitch_uv = sensorinfo->PitchUV;
                dispProp.format   = MMP_ISP_IN_NV12;
                itv_update_osddbuf_anchor(&dispProp);
            }
            free(sensorinfo);
        }
        usleep(1000);
    }

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes FilterDisplayOSD_methods[] = {
    {0, NULL},
};

// clang-format off
IteFilterDes FilterDisplayOSD = {
    ITE_FILTER_DISPLAY_OSD_ID,
    displayosd_init,
    displayosd_uninit,
    displayosd_process,
    FilterDisplayOSD_methods
};
// clang-format on
