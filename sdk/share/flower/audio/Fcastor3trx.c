#include <stdio.h>
#include <stdlib.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
//=============================================================================
//                              Constant Definition
//=============================================================================
#define I2S_ADC_BUF_LENGHT 64 * 1024
extern STRC_I2S_SPEC spec_ad;
uint8_t *            rx_buf = NULL;

//=============================================================================
//                              struct Declaration
//=============================================================================
typedef struct TRxData
{
    int rate;
} TRxData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================
void config_rx (void)
{
    if (!rx_buf)
    {
        rx_buf = (uint8_t *)malloc(I2S_ADC_BUF_LENGHT);
        memset(rx_buf, 0, I2S_ADC_BUF_LENGHT);
    }
    spec_ad.base_hdmi[0]          = (uint8_t *)rx_buf;
    spec_ad.num_hdmi_audio_buffer = 1;

    (void)printf("rx_INIT first time\n");
}

//=============================================================================
//                          Read filter flow
//=============================================================================
static void readRx_init (IteFilter * f)
{
    TRxData * d = (TRxData *)ite_new(TRxData, 1);
    if (d != NULL)
    {
        flow_init_AD();
    }
    f->data = d;
}

static void readRx_uninit (IteFilter * f)
{
    TRxData * d = (TRxData *)f->data;
    if (d != NULL)
    {
        free(d);
    }
}

static void readRx_process (IteFilter * f)
{
    TRxData *   d           = (TRxData *)f->data;
    IteQueueblk blk_output0 = {0};
    int         bytes       = 20 * (2 * spec_ad.sample_rate * spec_ad.channels) / 1000; // 10ms data byte;
    EVEN(bytes);
    I2S_AD32_SET_HDMI_RP(I2S_AD32_GET_HDMI_WP());
    i2s_pause_ADC(false);

    while (f->run)
    {
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }
        if (GET_AD2_RW_GAP < bytes)
        { // wait data
            usleep(20000);
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
    i2s_pause_ADC(true);
    return NULL;
}

static IteMethodDes sndTRx_methods[] = {
    {0, NULL}
};

// clang-format off
IteFilterDes FilterSndRx = {
    ITE_FILTER_SNDRX_ID,
    readRx_init,
    readRx_uninit,
    readRx_process,
    sndTRx_methods
};
// clang-format on
