#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "ite/itp.h"
#include "i2s/i2s.h"

#define I2S_DAC_BUF_LENGHT  128 * 1024
#define I2S_ADC_BUF_LENGHT  64 * 1024

STRC_I2S_SPEC   spec_da         = {0};
STRC_I2S_SPEC   spec_ad         = {0};
int             gbytes          = 128;

static uint8_t  * gp_da_buf_    = NULL;
static uint8_t  * gp_ad_buf_    = NULL;

void *
autostop_task (
    IteFlower * f);

// =============================================================================
//                              struct Declaration
// =============================================================================
typedef struct SndData
{
    int         autorelease;
    cb_sound_t  fn_cb;
} SndData;

// =============================================================================
//                       global func filter AD DA param Init
// =============================================================================
void
flow_init_DA (
    void)
{
    if (NULL == gp_da_buf_)
    {
        memset((void *)&spec_da, 0, sizeof(STRC_I2S_SPEC));
        gp_da_buf_ = (uint8_t *)malloc(I2S_DAC_BUF_LENGHT);
        if (NULL != gp_da_buf_)
        {
            memset(gp_da_buf_, 0, I2S_DAC_BUF_LENGHT);
        }
        assert(gp_da_buf_);
    }
    else
    {
        return;
    }
    spec_da.channels                    = 1;
    spec_da.sample_rate                 = 8000;
    spec_da.buffer_size                 = I2S_DAC_BUF_LENGHT;
    spec_da.is_big_endian               = false;
    spec_da.base_i2s                    = gp_da_buf_;

    spec_da.sample_size                 = 16;
    spec_da.num_hdmi_audio_buffer       = 1;
    spec_da.is_dac_spdif_same_buffer    = 1;

    spec_da.base_hdmi[0]                = gp_da_buf_;
    spec_da.base_hdmi[1]                = gp_da_buf_;
    spec_da.base_hdmi[2]                = gp_da_buf_;
    spec_da.base_hdmi[3]                = gp_da_buf_;
    spec_da.base_spdif                  = gp_da_buf_;

    spec_da.enable_Speaker              = 1;
    spec_da.enable_HeadPhone            = 1;
    spec_da.postpone_audio_output       = true;

    i2s_init_DAC(&spec_da);

    (void)printf("DAC_INIT first time\n");
}

void
flow_init_AD (
    void)
{
    if (NULL == gp_ad_buf_)
    {
        memset((void *)&spec_ad, 0, sizeof(STRC_I2S_SPEC));
        gp_ad_buf_ = (uint8_t *)malloc(I2S_ADC_BUF_LENGHT);
        if (NULL != gp_ad_buf_)
        {
            memset(gp_ad_buf_, 0, I2S_ADC_BUF_LENGHT);
        }
    }
    else
    {
        return;
    }
    /* ADC Spec */
    spec_ad.channels        = 1;
    spec_ad.sample_rate     = 8000;
    spec_ad.buffer_size     = I2S_ADC_BUF_LENGHT;

    spec_ad.is_big_endian   = false;
    spec_ad.base_i2s        = gp_ad_buf_;

    spec_ad.sample_size     = 16;
    spec_ad.record_mode     = 1;

    spec_ad.from_MIC_IN     = true;
#if CFG_I2S_HDMI_RX_CLK
    config_rx();
#endif
    i2s_init_ADC(&spec_ad);
    i2s_pause_ADC(true);

    (void)printf("ADC_INIT first time\n");
}

static void
DA_put_scilent (
    uint32_t size)
{
    uint8_t * buf = (uint8_t *)malloc(size);
    if (NULL != buf)
    {
        memset(buf, 0, size);
        i2s_da_data_put(buf, size);
        free(buf);
    }
}
// =============================================================================
//                          Write(DA) filter flow
// =============================================================================

static void
dawrite_init (
    IteFilter * f)
{
    SndData * d = (SndData *)ite_new(SndData, 1);
    if (d != NULL)
    {
        flow_init_DA();
        d->fn_cb             = NULL;
        d->autorelease       = true;
        // d->srcFlow=NULL;
        // pthread_mutex_init(&d->mutex,NULL);
        f->pthread_stacksize = 16 * 1024;
    }
    f->data = d;
}

static void
dawrite_uninit (
    IteFilter * f)
{
    SndData * d = (SndData *)f->data;
    if (d != NULL)
    {
        d->fn_cb = NULL;
        // d->srcFlow=NULL;
        // pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void
dawrite_process (
    IteFilter * f)
{
    SndData     * d         = (SndData *)f->data;
    IteFlower   * flower    = (IteFlower *)f->srcFlow;
    gbytes = 20 * (2 * spec_da.sample_rate * spec_da.channels) / 1000;             // 20ms data byte;
    EVEN(gbytes);
    // blk.AInfo.Eof =false;
    int count = 1;
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    DA_put_scilent(gbytes);

    // i2s_pause_DAC(false);
    while (f->run)
    {
        IteQueueblk blk = {0};
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;

            while (om && (GET_DA_RW_RGAP + om->size) >= GET_DA_BASE_LEN)
            {
                usleep(10000 * DAWAITR(gbytes)); // avoid put too many data into i2sbuf
                // (void)printf("%d\n",1000*GET_DA_RW_GAP/bytes);
            }

            if (om)
            {
                i2s_da_data_put(om->b_rptr, om->size);
                freemsg_ite(om);
            }

            if ((PlaySoundCase)blk.private1 == Eofsound)
            {
                if (d->fn_cb)
                {
                    d->fn_cb(Eofsound, NULL);
                }
            }

            if ((PlaySoundCase)blk.private2 == Eofmixsound)
            {
                if (d->fn_cb)
                {
                    d->fn_cb(Eofmixsound, NULL);
                }
            }

            if (((PlaySoundCase)blk.private1 == Eofclose) && ((PlaySoundCase)blk.private2 == Eofclose))
            {
                while (GET_DA_RW_RGAP > gbytes)
                {
                    usleep(1000 * DAWAITR(gbytes));
                    if (!f->run)
                    {
                        goto dawrite_end;
                    }
                }
                if (d->autorelease == true)
                {
                    flower->cb_func = d->fn_cb;
                    CreateDetachedThread(autostop_task, flower);
                    // break;
                }
            }
            if (count > 0)
            {
                count--;
                if (count == 0)
                {
                    i2s_pause_DAC(false);
                    //i2s_fadepause_DAC(false,0x01U,0xFFU);//fade in
                }
            }
        }
        else
        {
            if (GET_DA_RW_RGAP < 32)
            {
                DA_put_scilent(gbytes);
                // (void)printf("add scilent\n");
                //usleep(100000);
            }
        }
        usleep(1000 * DAWAITR(gbytes));
    }

dawrite_end:
    i2s_pause_DAC(true);
    //i2s_fadepause_DAC(true,0xFFU,0x04U);//fade out;
}

// =============================================================================
//                          Read(AD) filter flow
// =============================================================================

static void
adread_init (
    IteFilter * f)
{
    SndData * d = (SndData *)ite_new(SndData, 1);
    if (d != NULL)
    {
        flow_init_AD();
        flow_init_DA();
    }
    f->data = d;
}

static void
adread_uninit (
    IteFilter * f)
{
    SndData * d = (SndData *)f->data;
    if (d != NULL)
    {
        free(d);
    }
}

static void
adread_process (
    IteFilter * f)
{
    // SndData *d=(SndData*)f->data;
    IteQueueblk blk_output0 = {0};
    IteFlower   * flower    = (IteFlower *)f->srcFlow;
    int         bytes       = 20 * (2 * spec_ad.sample_rate * spec_ad.channels) / 1000; // 10ms data byte;
    EVEN(bytes);

    flower->dInfo.sr        = spec_ad.sample_rate;
    flower->dInfo.nch       = spec_ad.channels;
    flower->dInfo.bitsize   = 16;
    flower->dInfo.bytes20ms = bytes;
    flower->dInfo.init      = true;

    I2S_AD32_SET_RP(I2S_AD32_GET_WP());
    i2s_pause_ADC(false);

    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }

        if (GET_AD_RW_GAP < bytes)
        {   // wait data
            usleep(100000);
            continue;
        }
        else
        {   // get data
            mblk_ite * im = allocb_ite(bytes);

            i2s_ad_data_get(im->b_wptr, bytes);
            im->b_wptr          += bytes;
            blk_output0.datap   = im;
            ite_queue_put(f->output[0].Qhandle, &blk_output0);

            usleep(10000);
        }
    }
    i2s_pause_ADC(true);
}

// =============================================================================
//                               method
// =============================================================================
static void
dawrite_set_cb (
    IteFilter   * f,
    void        * arg)
{
    SndData * d = (SndData *)f->data;
    d->fn_cb = (cb_sound_t)arg;
}

static void
dawrite_set_autorelease (
    IteFilter   * f,
    void        * arg)
{
    SndData * d = (SndData *)f->data;
    d->autorelease = *((bool *)arg);
}

// clang-format off
static IteMethodDes snd_methods[] = {
    {ITE_FILTER_SET_CB,    dawrite_set_cb         },
    {ITE_FILTER_SET_PARAM, dawrite_set_autorelease},
    {0,                    NULL                   }
};

IteFilterDes        FilterDAWtire = {
    ITE_FILTER_DAWRITE_ID,
    dawrite_init,
    dawrite_uninit,
    dawrite_process,
    snd_methods
};

IteFilterDes        FilterADRead = {
    ITE_FILTER_ADREAD_ID,
    adread_init,
    adread_uninit,
    adread_process,
    snd_methods
};
// clang-format on

// =============================================================================
//                          global func AD DA deInit
// =============================================================================
void
dawrite_deinit_state (
    void)
{
    spec_da.sample_rate = 1000;
    spec_da.sample_size = 8;
    i2s_deinit_DAC();
}

void
adread_deinit_state (
    void)
{
    spec_ad.sample_rate = 1000;
    spec_ad.sample_size = 8;
    i2s_deinit_ADC();
}

void
Castor3snd_deinit_state (
    int rate,
    int bitsize,
    int channel)
{
    dawrite_deinit_state();
    adread_deinit_state();
}

// =============================================================================
//                          global func AD DA Init
// =============================================================================
int
dawrite_reinit_for_diff_rate (
    int rate,
    int bitsize,
    int channel)
{
    if ((spec_da.sample_rate == rate) && (spec_da.sample_size == bitsize) && (spec_da.channels == channel) &&
        i2s_get_DA_running())
    {
        return 0;
    }

    // I2S_DA32_WAIT_RP_EQUAL_WP(1);//wait play over;

    i2s_deinit_DAC();
    spec_da.sample_rate = rate;
    spec_da.sample_size = bitsize;
    spec_da.channels    = channel;

    i2s_init_DAC(&spec_da);

    gbytes = 20 * (2 * rate * channel) / 1000;
    EVEN(gbytes);
    return 1;
}

int
adread_reinit_for_diff_rate (
    int rate,
    int bitsize,
    int channel)
{
    if ((spec_ad.sample_rate == rate) && (spec_ad.sample_size == bitsize) && (spec_ad.channels == channel) &&
        i2s_get_AD_running())
    {
        return 0;
    }

    i2s_deinit_ADC();
    spec_ad.sample_rate = rate;
    spec_ad.sample_size = bitsize;
    spec_ad.channels    = channel;

    i2s_init_ADC(&spec_ad);

    gbytes = 20 * (2 * rate * channel) / 1000;
    EVEN(gbytes);
    return 1;
}

int
Castor3snd_reinit_for_diff_rate (
    int rate,
    int bitsize,
    int channel)
{
    dawrite_reinit_for_diff_rate(rate, bitsize, channel);
    adread_reinit_for_diff_rate(rate, bitsize, channel);
    return 1;
}

// =============================================================================
//                          global func AD DA level setting
// =============================================================================
// set DAC level
void
flow_set_dac_level (
    int level)
{
    i2s_set_direct_volperc(level);
}

// set ADC level
void
flow_set_adc_level (
    int level)
{
    i2s_ADC_set_rec_volperc(level);
}
