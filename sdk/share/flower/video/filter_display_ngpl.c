#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "filter_mjpegdec_ngpl.h"

static void display_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void display_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void display_process (IteFilter * f)
{
    YUV_FRAME *       picture  = NULL;
    IteQueueblk       blk      = {0};
    mblk_ite *        im       = NULL;
    ITV_DBUF_PROPERTY dispProp = {0};

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            uint8_t * dbuf = NULL;
            im             = (mblk_ite *)blk.datap;
            picture        = (YUV_FRAME *)im->b_rptr;
            dbuf           = itv_get_dbuf_anchor();
            if (dbuf)
            {
                dispProp.src_w    = picture->width;
                dispProp.src_h    = picture->height;
                dispProp.ya       = picture->data[0];
                dispProp.ua       = picture->data[1];
                dispProp.va       = picture->data[2];
                dispProp.pitch_y  = picture->linesize[0];
                dispProp.pitch_uv = picture->linesize[1];
                dispProp.format   = picture->format;

                itv_update_dbuf_anchor(&dispProp);
                freemsg_ite(im);
            }
            else
            {
                freemsg_ite(im);
            }
        }
        usleep(1000);
    }
}

// clang-format off
IteFilterDes FilterDisplay = {
    ITE_FILTER_DISPLAY_ID,
    display_init,
    display_uninit,
    display_process,
};
// clang-format on
