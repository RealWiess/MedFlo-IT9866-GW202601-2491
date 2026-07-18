#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "ite/itp.h"
#include "i2s/i2s.h"

#define BUF_LENGHT 64 * 1024

extern STRC_I2S_SPEC spec_ad;

typedef struct LoopBackData
{
    int       rate;
    int       nchannel;
    uint8_t * buf;
} LoopBackData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
void LoopBack_buf_set (LoopBackData * d, bool yesno)
{
    if (yesno)
    {
        if (!d->buf)
        {
            d->buf = (uint8_t *)malloc(BUF_LENGHT);
            memset(d->buf, 0, BUF_LENGHT);
        }
        i2s_loopback_set(d->buf,
                         0); /*DA play data loopback to spec_ad.base_hdmi[0]*/
    }
    else
    {
        if (d->buf)
        {
            free(d->buf);
            d->buf = NULL;
        }
        i2s_loopback_set(d->buf, 0);
    }
}

static void loopback_init (IteFilter * f)
{
    LoopBackData * d = (LoopBackData *)ite_new(LoopBackData, 1);
    if (d != NULL)
    {
        d->buf = NULL;
    }
    f->data = d;
}

static void loopback_uninit (IteFilter * f)
{
    LoopBackData * d = (LoopBackData *)f->data;
    if (d != NULL)
    {
        free(d);
    }
}

static void loopback_process (IteFilter * f)
{
    LoopBackData * d = (LoopBackData *)f->data;
    LoopBack_buf_set(d, true);
    IteQueueblk blk_output0 = {0};
    int         bytes = 20 * (2 * spec_ad.sample_rate * spec_ad.channels) /
                1000; // 10ms data byte;
    bytes *= 2;
    I2S_AD32_SET_HDMI_RP(I2S_AD32_GET_HDMI_WP());

    // i2s_pause_ADC(false);
    while (f->run)
    {
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }

        if (GET_AD2_RW_GAP < bytes)
        { // wait data
            usleep(100000);
            continue;
        }
        else
        { // get data
            mblk_ite * im = allocb_ite(bytes);

            i2s_ad2_data_get(im->b_wptr, bytes, 0);
            im->b_wptr += bytes;
            blk_output0.datap = im;
            ite_queue_put(f->output[0].Qhandle, &blk_output0);

            usleep(10000);
        }
    }
    // i2s_pause_ADC(true);
    LoopBack_buf_set(d, false);
}

// clang-format off
static IteMethodDes loopback_methods[] = {
    {0, NULL}
};

IteFilterDes FilterLoopBack = {
    ITE_FILTER_LOOPBACK_ID,
    loopback_init,
    loopback_uninit,
    loopback_process,
    loopback_methods
};
// clang-format on
