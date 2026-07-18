#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "ite/ith.h"
#include "ite/itp.h"

#include "i2s/i2s.h"
#include "i2s_reg_9860.h"

/* ************************************************************************** */
#ifndef CFG_GPIO_I2S_USER
    #define CFG_GPIO_I2S_USER -1
#endif
#define I2S_SLAVE_MODE 0U

/******************************/
// ENABLE_HDMI_GPIO_IN_SET
// #define I2S_SLAVE_MODE      1U
// #define I2S_HDMI_RX_CLK     1
/* ************************************************************************** */
#define TV_CAL_DIFF_MS(TIME2, TIME1)                                                                                   \
    ((((TIME2.tv_sec * 1000000UL) + TIME2.tv_usec) - ((TIME1.tv_sec * 1000000UL) + TIME1.tv_usec)) / 1000)

// #define DEBUG_PRINT (void)printf
#define DEBUG_PRINT(...)

#ifdef _WIN32
    #define asm()
#endif

/* ************************************************************************** */

static bool            gb_i2s_DA_running_ = false;
static bool            gb_i2s_AD_running_ = false;
static bool            gb_i2s_DA_mute_    = false;
static bool            gb_i2s_AD_mute_    = false;
static bool            gb_bar_mute_flag_  = false;
static uint8_t              sample_width;

static pthread_mutex_t I2S_MUTEX = PTHREAD_MUTEX_INITIALIZER;
#if 0
static void dump_i2s_reg (void)
{
    int32_t i;
    for (i = 0x0; i <= 0xC8; i += 0x04)
    {
        (void)printf("reg[0x%02x] = 0x%08X\n", i, ithReadRegA(I2S_REG_BASE | i));
    }
    (void)printf("REG_3C = 0x%08X\n", ithReadRegA(MMP_AUDIO_CLOCK_REG_3C));
    (void)printf("REG_40 = 0x%08X\n", ithReadRegA(MMP_AUDIO_CLOCK_REG_40));
}
#endif

/* ************************************************************************** */
uint32_t                    I2S_DA32_GET_RP (void)
{
    return ithReadRegA(I2S_REG_OUT_RDPTR);
}

uint32_t I2S_DA32_GET_WP (void)
{
    return ithReadRegA(I2S_REG_OUT_WRPTR);
}

void I2S_DA32_SET_WP (uint32_t data32)
{
    ithWriteRegA(I2S_REG_OUT_WRPTR, data32);
}

void I2S_DA32_WAIT_RP_EQUAL_WP (int32_t wait)
{
static bool            b_dacRPwait      = false;
    b_dacRPwait = wait;

    if (b_dacRPwait)
    {
        int32_t nCh, nSampleRate, bytes, i = 0;

        itp_codec_get_i2s_sample_rate(&nSampleRate);
        nCh   = ((ithReadRegA(I2S_REG_OUT_CTRL) >> 4) & 0x1) + 1;
        bytes = 10 * (2 * nSampleRate * nCh) / 1000; // 10ms data byte;
        ithWriteRegMaskA(I2S_REG_OUT_CTRL, 1 << 2, 1 << 2);

        while (GET_DA_RW_GAP)
        {
            (void)usleep(2000 * DAWAIT(bytes));
            i++;
            if (i > 100)
            {
                break;
                (void)printf("I2S# %d should not be happened\n", __LINE__);
            }
            if (!b_dacRPwait)
            {
                break;
            }
        }
    }
    b_dacRPwait = false;
    ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0 << 2, 1 << 2);
}

uint32_t I2S_AD32_GET_RP (void)
{
    return ithReadRegA(I2S_REG_IN1_RDPTR);
}

uint32_t I2S_AD32_GET_WP (void)
{
    return ithReadRegA(I2S_REG_IN1_WRPTR);
}

void I2S_AD32_SET_RP (uint32_t data32)
{
    ithWriteRegA(I2S_REG_IN1_RDPTR, data32);
}

uint32_t I2S_AD32_GET_HDMI_RP (void)
{ // HDMI RX
    return ithReadRegA(I2S_REG_IN2_RDPTR);
    // return 0;
}

uint32_t I2S_AD32_GET_HDMI_WP (void)
{ // HDMI RX
    return ithReadRegA(I2S_REG_IN2_WRPTR);
    // return 0;
}

void I2S_AD32_SET_HDMI_RP (uint32_t data32)
{ // HDMI RX
    ithWriteRegA(I2S_REG_IN2_RDPTR, data32);
}

bool i2s_is_DAstarvation (void)
{
    return (0U != (ithReadRegA(I2S_REG_OUT_STATUS1) & 0x1U));
}

static void i2s_power_on_ (void)
{
    uint32_t data32;
    /* enable audio clock for I2S modules */
    /* NOTE: we SHOULD enable audio clock before toggling 0x003A/3C/3E */
    /* enable PLL2 for audio */
    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_40);
    data32 |= (((uint32_t)1U) << ((uint32_t)17U)); /* enable AMCLK (system clock for wolfson chip) */
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_40, data32);

    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C); // Audio Clock Register 1
    data32 |= (((uint32_t)1U) << ((uint32_t)19U)); /* enable ZCLK  (IIS DAC clock)                 */
    data32 |= (((uint32_t)1U) << ((uint32_t)21U)); /* enable M7CLK (memory clock for DAC)          */
    data32 |= (((uint32_t)1U) << ((uint32_t)23U)); /* enable M8CLK (memory clock for ADC)          */
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);
}

static void i2s_power_off_ (void)
{
    uint32_t data32;
    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_40);
    data32 &= ~(((uint32_t)1U) << ((uint32_t)17U)); /* disable AMCLK (system clock for wolfson chip) */
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_40, data32);

    /* disable audio clock for I2S modules */
    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C); // Audio Clock Register 1
    data32 &= ~(((uint32_t)1U) << ((uint32_t)19U)); /* disable ZCLK  (IIS DAC clock)                 */
    data32 &= ~(((uint32_t)1U) << ((uint32_t)21U)); /* disable M7CLK (memory clock for DAC)          */
    data32 &= ~(((uint32_t)1U) << ((uint32_t)23U)); /* disable M8CLK (memory clock for ADC)          */
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);
}

static void i2s_reset_ (void)
{
    uint32_t data32;

#if 0
    if (gb_i2s_DA_running_ && itp_codec_get_DA_running())
    {
        (void)printf("I2S# DAC is running, skip reset I2S !\n");
        return;
    }
    if (gb_i2s_AD_running_ && itp_codec_get_AD_running())
    {
        (void)printf("I2S# ADC is running, skip reset I2S !\n");
        return;
    }
#endif

#if 1 // FixMe: should 970 use???
    /*Disable PLLL2*/
    data32 = ithReadRegA(0xD8000114);
    data32 |= ((uint32_t)0x1 << 31);
    ithWriteRegA(0xD8000114, data32);

    ithDelay(100); /* FIXME: dummy loop */
    ;

    // data32 = ithReadRegA(0xD800011C);
    // data32 |= (0x1 << 31);
    // ithWriteRegA(0xD800011C,data32);

    // ithDelay(100); /* FIXME: dummy loop */;

    /*  data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);
        data32 |= (0xF << 28);//reset DA & AD engine
        ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);

        ithDelay(100); /* FIXME: dummy loop */
    ;

    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);
    data32 &= ~((uint32_t)0xF << 28); // normal option DA & AD engine
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);

    ithDelay(100); /* FIXME: dummy loop */
    ;
#endif

    (void)printf("I2S# %s\n", __func__);
}

static void i2s_aws_sync_ (STRC_I2S_SPEC * i2s_spec)
{
    uint32_t data32;
    data32 = (((uint32_t)1U) << ((uint32_t)31U)) // ADCWS/DACWS syn
             | (CFG_I2S_HDMI_RX_CLK << 4U)       // CLK/WS/DATA1~4 from HDMI RX
             | (CFG_I2S_INTERNAL_CODEC << 2U)    // select External/Internal DAC
             | (I2S_SLAVE_MODE << 1U)            // IIS Output Slave/Master Mode
             | (I2S_SLAVE_MODE << 0U)            // IIS Input Slave/Master Mode
        ;
    if (i2s_spec->from_LineIN)
    {
        data32 &= (~1 << 2);
    }
    ithWriteRegA(I2S_REG_CODEC_SET, data32);
    (void)usleep(10);

    ithWriteRegMaskA(I2S_REG_CODEC_SET, 0U, ((uint32_t)1U) << ((uint32_t)31U));
}

#if CFG_I2S_INTERNAL_CODEC
static void i2s_set_clock_ (uint32_t demanded_sample_rate, uint8_t * demanded_sample_width)
{
    uint8_t data8 = 0x0;

    #if 0
    if (gb_i2s_DA_running_ && itp_codec_get_DA_running())
    {
        (void)printf("I2S# DAC is running, skip set clock !\n");
        return;
    }
    if (gb_i2s_AD_running_ && itp_codec_get_AD_running())
    {
        (void)printf("I2S# ADC is running, skip set clock !\n");
        return;
    }
    #endif

    (void)printf("I2S# %s, demanded_sample_rate: %u\n", __func__, demanded_sample_rate);
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_40, 0x0002B801); // AMCLK PLL2_OUT2 /2 ,[9:0]:amclk_ratio (1)

    if (demanded_sample_rate == 32000 || demanded_sample_rate == 48000)
    {
        // 12.288 MHz
        ithWriteRegA(0xD8000118U, 0xa0383701U);
        ithWriteRegA(0xD800011CU, 0x80035c00U);
        ithWriteRegA(0xD800011CU, 0xf0335c00U);
        ithDelay(220);
        ithWriteRegA(0xD800011cU, 0x80035c00U);
    }
    else if (demanded_sample_rate == 44100)
    {
        // 11.2896 MHz
        ithWriteRegA(0xD8000118U, 0xa0292C01U);
        ithWriteRegA(0xD800011CU, 0x8003F800U);
        ithWriteRegA(0xD800011CU, 0xf033F800U);
        ithDelay(220);
        ithWriteRegA(0xD800011CU, 0x8003F800U);
    }
    else
    {
        // 12 MHz
        ithWriteRegA(0xD8000118U, 0x20404001U);
        ithWriteRegA(0xD800011CU, 0x80000000U);
        ithWriteRegA(0xD800011CU, 0xf3000000U);
        ithDelay(220);
        ithWriteRegA(0xD800011CU, 0x80000000U);
    }

    switch (demanded_sample_rate)
    {
        case 48000:
            data8                  = 0x4;
            *demanded_sample_width = 0x1F;
            break;
        case 44100:
            data8                  = 0x4;
            *demanded_sample_width = 0x1F;
            break;
        case 32000:
            data8                  = 0x6;
            *demanded_sample_width = 0x1F;
            break;
        case 24000:
            data8                  = 0xA;
            *demanded_sample_width = 0x18;
            break;
        case 22050:
            data8                  = 0x8;
            *demanded_sample_width = 0x21;
            break;
        case 16000:
            data8                  = 0xF;
            *demanded_sample_width = 0x18;
            break;
        case 12000:
            data8                  = 0xA;
            *demanded_sample_width = 0x31;
            break;
        case 11025:
            data8                  = 0x10;
            *demanded_sample_width = 0x21;
            break;
        case 8000:
            data8                  = 0x1E;
            *demanded_sample_width = 0x18;
            break;
        default:
            (void)printf("not support sampling rate : %d\n", (int32_t)demanded_sample_rate);
            break;
    }
    //(void)printf("data8 = 0x%X demanded_sample_width =0x%X\n",data8,*demanded_sample_width);

    // if(_cold_init) ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C,(0xF2A88800|data8));//zclk_ratio
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, (0xB2A88800U | data8)); // zclk_ratio

    ithDelay(10);
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, (0x02A88800U | data8));
}

static void _i2s_set_BTclock (uint32_t demanded_sample_rate, uint8_t * demanded_sample_width)
{
    uint8_t data8 = 0x0;

    (void)printf("I2S# %s, demanded_sample_rate: %u\n", __func__, demanded_sample_rate);
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_40, 0x0002B801U); // AMCLK PLL2_OUT2 /2 ,[9:0]:amclk_ratio (1)

    if (demanded_sample_rate == 48000 || demanded_sample_rate == 8000 || demanded_sample_rate == 44100)
    {
        // 12.288 MHz
        ithWriteRegA(0xD8000118U, 0xa0383701U);
        ithWriteRegA(0xD800011CU, 0x80035c00U);
        ithWriteRegA(0xD800011CU, 0xf0335c00U);
        ithDelay(220);
        ithWriteRegA(0xD800011cU, 0x80035c00U);
    }
    else
    {
        (void)printf("not support %d\n", (int32_t)demanded_sample_rate);
    }

    switch (demanded_sample_rate)
    {
        case 48000:
            data8                  = 0x8;
            *demanded_sample_width = 0x0F;
            break;
        case 44100:
            data8                  = 0x8;
            *demanded_sample_width = 0x0F;
            break;
        case 8000:
            data8                  = 0x30;
            *demanded_sample_width = 0x0F;
            break;
        default:
            (void)printf("not support sampling rate : %d\n", (int32_t)demanded_sample_rate);
            break;
    }

    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, (0xB2A88800U | data8)); // zclk_ratio

    ithDelay(10);
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, (0x02A88800U | data8));
}

#else
static void i2s_set_clock_ (uint32_t demanded_sample_rate, uint8_t * demanded_sample_width)
{
    (void)printf("not support external DAC\n");
}
#endif

static void i2s_enable_fading_ (uint32_t fading_step, uint32_t fading_duration)
{
    uint32_t data32;

    ithWriteRegMaskA(I2S_REG_OUT_FADE_SET, 1U << 0U, 1U << 0U); // Enable OUT_BASE1 fade
    data32 = ithReadRegA(I2S_REG_OUT_FADE_SET);
    data32 |= ((fading_step & 0xF) << 8);
    data32 |= ((fading_duration & 0xFF) << 16);
    ithWriteRegA(I2S_REG_OUT_FADE_SET, data32);
}

static void i2s_disable_fading_ (void)
{
    ithWriteRegMaskA(I2S_REG_OUT_FADE_SET, 0U, 1U << 0U); // Disable OUT_BASE1 fade
}

static void GPIO_init_ (int32_t array[], int32_t size, ITHGpioMode mode)
{
    int32_t i;
    for (i = 0; i < size; i++)
    {
        ithGpioSetMode(array[i], mode);
        (void)printf("CFG_GPIO_I2S : %d mode %d\n", array[i], mode);
    }
}

static void i2s_set_GPIO_ (void) /* GPIO settings for CFG2 */
{
#if (CFG_I2S_INTERNAL_CODEC == 0)
    {
        uint32_t GpioTable[] = {CFG_GPIO_I2S};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef ENABLE_HDMI_GPIO_OUT_SET
    {
        uint32_t GpioTable[] = {CFG_GPIO_I2S_HDMITX};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef ENABLE_HDMI_GPIO_IN_SET
    {
        uint32_t GpioTable[] = {CFG_GPIO_I2S_HDMIRX};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef __OPENRTOS__
    {
        uint32_t GpioTable[] = {CFG_GPIO_I2S_USER};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
}

/* ************************************************************************** */

void i2s_volume_up (void)
{
    // amp use. useless function
}

void i2s_volume_down (void)
{
    // amp use. useless function
}

void i2s_pause_ADC (bool pause)
{
    (void)printf("I2S# %s(%d)\n", __func__, pause);

    if (pause)
    {
        ithWriteRegMaskA(I2S_REG_IN_CTRL, 0U, 1U << 0U);
        itp_codec_rec_mute();
    }
    else /* resume */
    {
        ithWriteRegMaskA(I2S_REG_IN_CTRL, 1U << 0U, 1U << 0U);
        itp_codec_rec_unmute();
    }
}

void i2s_enable_fading (bool yesno)
{
    if (yesno)
    {
        //      i2s_enable_fading_(0xF, 0x01); /* fast response */
        //      i2s_enable_fading_(0x7, 0x7F); /* moderato */
        i2s_enable_fading_(0x1, 0xFF); /* slow response */
        ithWriteRegA(I2S_REG_OUT_VOLUME, 0x007F0100);
    }
    else
    {
        i2s_disable_fading_();
    }
}

void i2s_pause_DAC (bool pause)
{
    (void)printf("I2S# %s(%d)\n", __func__, pause);

    if (pause)
    {
        ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0U, 1U << 0U);
        itp_codec_playback_mute();
    }
    else /* resume */
    {
        if (!gb_i2s_DA_mute_)
        {
            itp_codec_playback_unmute();
        }
        else
        {
            itp_codec_playback_mute();
        }
        ithWriteRegMaskA(I2S_REG_OUT_CTRL, 1U << 0U, 1U << 0U);
    }
}

void i2s_deinit_ADC (void)
{
    uint32_t data32;

    if (0 == gb_i2s_AD_running_)
    {
        (void)printf("I2S# ADC is 'NOT' running, skip deinit ADC !\n");
        return;
    }

    (void)printf("I2S# %s +\n", __func__);

    pthread_mutex_lock(&I2S_MUTEX);

    itp_codec_rec_deinit();

    /* disable hardware I2S */
    {
        data32 = ithReadRegA(I2S_REG_IN_CTRL);
        data32 &= ~(3 << 0);
        ithWriteRegA(I2S_REG_IN_CTRL, data32);
    }

    gb_i2s_AD_running_ = false; /* put before i2s_reset_() */
    i2s_reset_();

    i2s_power_off_();

    pthread_mutex_unlock(&I2S_MUTEX);
    (void)printf("I2S# %s -\n", __func__);
}

void i2s_deinit_DAC (void)
{
    uint32_t data32;

    pthread_mutex_lock(&I2S_MUTEX);

    if (!gb_i2s_DA_running_)
    {
        (void)printf("I2S# DAC is 'NOT' running, skip deinit DAC !\n");
        pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }

    (void)printf("I2S# %s +\n", __func__);

    /*cost 2ms prevent pop sound */
    itp_codec_playback_mute();
    ithWriteRegMaskA(I2S_REG_OUT_CTRL, 1U << 0U, 1U << 0U);
    ithDelay(2000);

    /*deinit dac*/
    itp_codec_playback_deinit();

    /* disable I2S_OUT_FIRE & I2S_OUTPUT_EN */
    data32 = ithReadRegA(I2S_REG_OUT_CTRL);
    data32 &= ~(0x7);
    ithWriteRegA(I2S_REG_OUT_CTRL, data32);

    gb_i2s_DA_running_ = 0; /* put before i2s_reset_() */
    i2s_reset_();

    i2s_power_off_();

    (void)printf("I2S# %s -\n", __func__);
    pthread_mutex_unlock(&I2S_MUTEX);
}

void i2s_init_DAC (STRC_I2S_SPEC * i2s_spec)
{
    int32_t param_fail = 0;
    int32_t i;
    uint32_t data32 = 0;
    uint8_t  resolution_type;
    uint8_t  hdmi_num;

    if (gb_i2s_DA_running_ && itp_codec_get_DA_running())
    {
        (void)printf("I2S# DAC is running, skip re-init !\n");
        return;
    }
    if (((uint32_t)i2s_spec->base_i2s % 8) || (i2s_spec->buffer_size % 8))
    {
        (void)printf("ERROR# I2S, bufbase/bufsize must be 8-Bytes alignment !\n");
        return;
    }
    if ((i2s_spec->channels != 1) && (i2s_spec->channels != 2))
    {
        (void)printf("ERROR# I2S, only support single or two channels !\n");
        return;
    }

    (void)printf("I2S# %s Start\n", __func__);
    pthread_mutex_lock(&I2S_MUTEX);

    i2s_power_on_();

    i2s_set_GPIO_();

    if (i2s_spec->from_LineIN)
    {
        _i2s_set_BTclock(i2s_spec->sample_rate, &sample_width);
    }
    else
    {
        i2s_set_clock_(i2s_spec->sample_rate, &sample_width);
    }

    //i2s_reset_();

    /* mastor mode & AWS & ZWS sync */
    switch (i2s_spec->sample_size)
    {
        case 16:
            {
                resolution_type = 0;
                break;
            }
        case 24:
            {
                resolution_type = 1;
                break;
            }
        case 32:
            {
                resolution_type = 2;
                break;
            }
        case 8:
            {
                resolution_type = 3;
                break;
            }
        default:
            {
                resolution_type = 2;
                break;
            }
    }

    data32 = (resolution_type << 28) /* ADC resolution bits, 00:16bits; 01:24bits; 10:32bits; 11:8bits */
             | (1 << 24)             /* WS mute check, 0:Disable 1:Enable */
             | (0xF << 16)           /* WS mute check times */
             | (sample_width << 0);  /* sample width (in bits unit) */
    ithWriteRegA(I2S_REG_DAC_SRATE_SET, data32);
    ithWriteRegA(I2S_REG_ADC_SRATE_SET, data32);

    /* 970 remove PCM support*/
    i2s_aws_sync_(i2s_spec);

    /*Faraday audio init*/
    if (i2s_spec->enable_Speaker)
    {
        itp_codec_playback_init(i2s_spec->channels); // 0:HPR only 1:HPL only 2:HPRL both
    }
    itp_codec_set_i2s_sample_rate(i2s_spec->sample_rate);

    /* buffer base */
    // for (i = 0; i<4; i++) {
    //     if(i2s_spec->base_hdmi[i] == NULL) {i2s_spec->base_hdmi[i] = i2s_spec->base_i2s;}
    // }
    ithWriteRegA(I2S_REG_OUT_BASE1, (uint32_t)i2s_spec->base_i2s); /*(IIS /SPDIF out) base 1*/
    // ithWriteRegA(I2S_REG_OUT_BASE2, (uint32_t)i2s_spec->base_hdmi[0]);/*(HDMI Data0 out) base 2*/
    // ithWriteRegA(I2S_REG_OUT_BASE3, (uint32_t)i2s_spec->base_hdmi[1]);/*(HDMI Data1 out) base 3*/
    // ithWriteRegA(I2S_REG_OUT_BASE4, (uint32_t)i2s_spec->base_hdmi[2]);/*(HDMI Data2 out) base 4*/
    // ithWriteRegA(I2S_REG_OUT_BASE5, (uint32_t)i2s_spec->base_hdmi[3]);/*(HDMI Data3 out) base 5*/

    /* buffer length */
    ithWriteRegA(I2S_REG_OUT_LEN, i2s_spec->buffer_size - 1);

    /* DA starvation interrupt threshold */
    ithWriteRegA(I2S_REG_OUT_RdWrGAP, 0x00000080); // OutRdWrGap[31:0]

    /*DA output Fade in/out reset*/
    ithWriteRegA(I2S_REG_OUT_FADE_SET, 0x00010100); // default:0x00010100
    /* output path */
    hdmi_num = 0;
    // switch(i2s_spec->num_hdmi_audio_buffer){
    //     case 1: { hdmi_num |=  1; break; } /* buf 1 (HDMI Data0) */
    //     case 2: { hdmi_num |=  3; break; }
    //     case 3: { hdmi_num |=  7; break; }
    //     case 4: { hdmi_num |= 15; break; }
    //     case 0:
    //     default: break;
    // }

    /*should 970 set W0OutRqSize[3:0]=0b1000 ?*/
    ithWriteRegA(I2S_REG_OUT_CTRL,
                 (hdmi_num << 9) /* HDMI DATA Output*/
                                       | (1 << 8)                                 /* Enable IIS 1/SPDIF Data Output */
                                       | (0 << 6)                                 /* Output Channel active level, 0:Low for Left; 1:High for Left */
                                       | (0 << 5)                                 /* Output Interface Format, 0:IIS Mode; 1:MSB Left-Justified Mode */
                                       | ((i2s_spec->channels - 1) << 4)          /* Output Channel Select, 0:Single Channel; 1:Two Channels */
                                       | ((i2s_spec->is_big_endian ? 1 : 0) << 3) /* 0:Little Endian; 1:Big Endian */
                                       | (0 << 2)                                 /* 0:NOT_LAST_WPTR; 1:LAST_WPTR */
                                       | (1 << 1)                                 /* Fire the IIS for Audio Output */
    );

    if (!i2s_spec->postpone_audio_output)
    {
        i2s_pause_DAC(false); /* Enable Audio Output */
    }
    else
    {
        i2s_pause_DAC(true);
    }

    gb_i2s_DA_running_ = 1;

    (void)printf("I2S# %s End\n", __func__);
    pthread_mutex_unlock(&I2S_MUTEX);
}

void i2s_init_ADC (STRC_I2S_SPEC * i2s_spec)
{
    int32_t i;
    uint32_t data32 = 0;
    uint8_t  resolution_type;
    uint8_t  hdmi_num;

    if (gb_i2s_AD_running_ && itp_codec_get_AD_running())
    {
        (void)printf("I2S# ADC is running, skip re-init ADC !\n");
        return;
    }
    if (((uint32_t)i2s_spec->base_i2s % 8) || (i2s_spec->buffer_size % 8))
    {
        (void)printf("ERROR# I2S, bufbase/bufsize must be 8-Bytes alignment !\n");
        return;
    }
    if ((i2s_spec->channels != 1) && (i2s_spec->channels != 2))
    {
        (void)printf("ERROR# I2S, only support single or two channels !\n");
        return;
    }

    (void)printf("I2S# %s Start\n", __func__);
    pthread_mutex_lock(&I2S_MUTEX);

    i2s_power_on_();

    i2s_set_GPIO_();

    if (i2s_spec->from_LineIN)
    {
        _i2s_set_BTclock(i2s_spec->sample_rate, &sample_width);
    }
    else
    {
        i2s_set_clock_(i2s_spec->sample_rate, &sample_width);
    }

    //i2s_reset_();

    itp_codec_set_i2s_sample_rate(i2s_spec->sample_rate);

    /* mastor mode & AWS & ZWS sync */
    switch (i2s_spec->sample_size)
    {
        case 16:
            {
                resolution_type = 0;
                break;
            }
        case 24:
            {
                resolution_type = 1;
                break;
            }
        case 32:
            {
                resolution_type = 2;
                break;
            }
        case 8:
            {
                resolution_type = 3;
                break;
            }
        default:
            {
                resolution_type = 2;
                break;
            }
    }

    /* config AD */
    data32 = (resolution_type << 28) /* ADC resolution bits, 00:16bits; 01:24bits; 10:32bits; 11:8bits */
             | (1 << 24)             /* WS mute check, 0:Disable 1:Enable */
             | (0xF << 16)           /* WS mute check times */
             | (sample_width << 0);  /* sample width (in bits unit) */
    ithWriteRegA(I2S_REG_ADC_SRATE_SET, data32);
    ithWriteRegA(I2S_REG_DAC_SRATE_SET, data32);

    i2s_aws_sync_(i2s_spec);

    if (i2s_spec->from_MIC_IN)
    {
        itp_codec_rec_init(i2s_spec->channels); // 0:micR only 1:micL only 2:micRLboth
    }

    /* buffer base */
    for (i = 0; i < 4; i++)
    {
        if (i2s_spec->base_hdmi[i] == NULL)
        {
            i2s_spec->base_hdmi[i] = i2s_spec->base_i2s;
        }
    }

    ithWriteRegA(I2S_REG_IN1_BASE1, (uint32_t)i2s_spec->base_i2s); /*(I2S  Data  in ) IN1 base1*/
    ithWriteRegA(I2S_REG_IN2_BASE4, (uint32_t)i2s_spec->base_hdmi[0]);
    ithWriteRegA(I2S_REG_IN2_BASE3, (uint32_t)i2s_spec->base_hdmi[1]);
    ithWriteRegA(I2S_REG_IN2_BASE2, (uint32_t)i2s_spec->base_hdmi[2]);
    ithWriteRegA(I2S_REG_IN2_BASE1, (uint32_t)i2s_spec->base_hdmi[3]);

    /* buffer length */
    ithWriteRegA(I2S_REG_IN_LEN, i2s_spec->buffer_size - 1);

    /* DA starvation interrupt threshold */
    ithWriteRegA(I2S_REG_IN_RdWrGAP, 0x00000040); // OutRdWrGap[31:0]

    hdmi_num = 0;
    switch (i2s_spec->num_hdmi_audio_buffer)
    {
        case 1:
            {
                hdmi_num |= 1;
                break;
            } /* buf 1 (HDMI Data0) */
        case 2:
            {
                hdmi_num |= 3;
                break;
            }
        case 3:
            {
                hdmi_num |= 7;
                break;
            }
        case 4:
            {
                hdmi_num |= 15;
                break;
            }
        default:
            {
                hdmi_num |= 0;
                break;
            }
    }

    ithWriteRegA(
        I2S_REG_IN_CTRL,
        (hdmi_num << 12)                      /* Eanble HDMI Data Input */
                                      | (1 << 8)                                 /* Eanble Input1 IIS Data Input */
                                      | (0 << 6)                                 /* Input Channel active level, 0:Low for Left; 1:High for Left */
                                      | (0 << 5)                                 /* Input Interface Format, 0:IIS Mode; 1:MSB Left-Justified Mode */
                                      | ((i2s_spec->channels - 1) << 4)          /* Input Channel, 0:Single Channel; 1:Two Channels */
                                      | ((i2s_spec->is_big_endian ? 1 : 0) << 3) /* 0:Little Endian; 1:Big Endian */
                                      | ((i2s_spec->record_mode ? 1 : 0) << 2)   /* 0:AV Sync Mode (wait capture to start); 1:Only Record Mode */
                                      | (1 << 1)                                 /* Fire the IIS for Audio Input */
                                      | (1 << 0)                                 /* Enable Audio Input */
    );

    gb_i2s_AD_running_ = true;

    pthread_mutex_unlock(&I2S_MUTEX);
    (void)printf("I2S# %s End\n", __func__);
}

bool i2s_get_DA_running (void)
{
    return itp_codec_get_DA_running();
}

bool i2s_get_AD_running (void)
{
    return itp_codec_get_AD_running();
}

void i2s_mute_DAC (bool mute)
{

    DEBUG_PRINT("I2S# %s(%d)\n", __func__, mute);
    if (mute)
    {
        itp_codec_playback_mute();
        gb_i2s_DA_mute_ = 1;
    }
    else /* resume */
    {
        itp_codec_playback_unmute();
        gb_i2s_DA_mute_ = 0;
    }
}

void i2s_set_direct_volperc (uint32_t volperc)
{ /*volperc: 0~100*/
    itp_codec_playback_set_direct_volperc(volperc);
    if (0U == volperc)
    {
        i2s_mute_volume(true); // mute
    }
    else if (!gb_i2s_DA_mute_ && gb_bar_mute_flag_)
    {
        i2s_mute_volume(false); // unmute
    }
    else
    {
        // do nothing
    }
}

uint32_t i2s_get_current_volperc (void)
{
    uint32_t currvolperc = 0;
    itp_codec_playback_get_currvol(&currvolperc);
    return currvolperc;
}

void i2s_set_direct_RLvolperc (uint32_t volperc, uint8_t RL)
{ /*volperc: 0~100*/
    itp_codec_playback_set_direct_RLvolperc(volperc, RL);
}

uint32_t i2s_get_current_RLvolperc (uint8_t RL)
{
    uint32_t currvolperc = 0;
    itp_codec_playback_get_RLcurrvol(&currvolperc, RL);
    return currvolperc;
}

void i2s_ADC_set_rec_volperc (uint32_t recperc)
{
    /*recperc:0~100*/
    itp_codec_rec_set_direct_volperc(recperc);
    if (!recperc)
    {
        i2s_mute_ADC(true); // mute
    }
    else if (gb_i2s_AD_mute_ && recperc)
    {
        i2s_mute_ADC(false); // unmute
    }
    else
    {
        // do nothing
    }
}

uint32_t i2s_ADC_get_current_volstep (void)
{
    uint32_t currmicperc = 0;
    itp_codec_rec_get_currvol(&currmicperc);
    return currmicperc;
}

void i2s_ADC_set_rec_RLvolperc (uint32_t recperc, uint8_t RL)
{
    /*recperc:0~100*/
    itp_codec_rec_set_direct_RLvolperc(recperc, RL);
}

uint32_t i2s_ADC_get_current_RLvolstep (uint8_t RL)
{
    uint32_t currmicperc = 0;
    itp_codec_rec_get_RLcurrvol(&currmicperc, RL);
    return currmicperc;
}

void i2s_ADC_get_volstep_range (uint32_t * max, uint32_t * normal, uint32_t * min)
{
    itp_codec_rec_get_vol_range(max, normal, min);
}

void i2s_mute_ADC (bool mute)
{
    DEBUG_PRINT("I2S# %s(%d)\n", __func__, mute);

    if (mute)
    {
        itp_codec_rec_mute();
    }
    else
    {
        itp_codec_rec_unmute();
    }

    gb_i2s_AD_mute_ = mute;
}

static void _show_i2s_spec (STRC_I2S_SPEC * i2s_spec)
{
    (void)printf("    @channels   %x\n", i2s_spec->channels);
    (void)printf("    @sample_rate    %x\n", i2s_spec->sample_rate);
    (void)printf("    @buffer_size    %x\n", i2s_spec->buffer_size);
    (void)printf("    @is_big_endian  %x\n", i2s_spec->is_big_endian);
    (void)printf("    @sample_size    %x\n", i2s_spec->sample_size);
    (void)printf("    @from_LineIN    %x\n", i2s_spec->from_LineIN);
    (void)printf("    @from_MIC_IN    %x\n", i2s_spec->from_MIC_IN);
    (void)printf("    @num_hdmi_buf   %x\n", i2s_spec->num_hdmi_audio_buffer);
    (void)printf("    @base_i2s   %x\n", i2s_spec->base_i2s);
    (void)printf("    @spdif_same_buf %x\n", i2s_spec->is_dac_spdif_same_buffer);
    (void)printf("    @base_spdif %x\n", i2s_spec->base_spdif);
    (void)printf("    @aud_out    %x\n", i2s_spec->postpone_audio_output);
    (void)printf("    @record_mode    %x\n", i2s_spec->record_mode);
    (void)printf("    @Speaker    %x\n", i2s_spec->enable_Speaker);
    (void)printf("    @HeadPhone  %x\n", i2s_spec->enable_HeadPhone);
}

void i2s_mute_volume (bool mute)
{
    gb_bar_mute_flag_ = mute;
    if (mute)
    {
        itp_codec_playback_mute();
    }
    else
    {
        itp_codec_playback_unmute();
    }
}
/*put data to i2s base (SPK)*/
uint32_t i2s_da_data_put (uint8_t * ptr, uint32_t size)
{
    uint32_t  DA_w        = I2S_DA32_GET_WP();
    uint8_t * base_i2s    = GET_DA_BASE;
    uint32_t  buffer_size = GET_DA_BASE_LEN;

    if ((DA_w + size) > buffer_size)
    {
        int32_t szbuf = buffer_size - DA_w;
        if (szbuf > 0)
        {
            (void)memcpy(base_i2s + DA_w, ptr, szbuf);
            ithFlushDCacheRange(base_i2s + DA_w, szbuf);
        }
        DA_w = size - szbuf;
        (void)memcpy(base_i2s, ptr + szbuf, DA_w);
        ithFlushDCacheRange(base_i2s, DA_w);
    }
    else
    {
        (void)memcpy(base_i2s + DA_w, ptr, size);
        ithFlushDCacheRange(base_i2s + DA_w, size);
        DA_w += size;
    }

    if (DA_w == buffer_size)
    {
        DA_w = 0;
    }

    I2S_DA32_SET_WP(DA_w);

    return 1;
}
/*get data from i2s base (MIC)*/
uint32_t i2s_ad_data_get (uint8_t * ptr, uint32_t size)
{

    uint32_t  AD_r        = I2S_AD32_GET_RP();
    uint8_t * base_i2s    = GET_AD_BASE;
    uint32_t  buffer_size = GET_AD_BASE_LEN;

    if (AD_r + size <= buffer_size)
    {
        // bsize = AD_w - AD_r;
        ithInvalidateDCacheRange(base_i2s + AD_r, size);
        (void)memcpy(ptr, base_i2s + AD_r, size);
        AD_r += size;
    }
    else
    { // AD_r > AD_w
        uint32_t szsec0 = buffer_size - AD_r;
        uint32_t szsec1 = size - szsec0;
        if (szsec0)
        {
            ithInvalidateDCacheRange(base_i2s + AD_r, szsec0);
            (void)memcpy(ptr, base_i2s + AD_r, szsec0);
        }
        ithInvalidateDCacheRange(base_i2s, szsec1);
        (void)memcpy(ptr + szsec0, base_i2s, szsec1);
        AD_r = szsec1;
    }

    I2S_AD32_SET_RP(AD_r);

    if (AD_r == buffer_size)
    {
        AD_r = 0;
    }

    return 1;
}
