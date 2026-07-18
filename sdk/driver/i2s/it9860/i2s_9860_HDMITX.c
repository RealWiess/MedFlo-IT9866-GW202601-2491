#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "ite/ith.h"
#include "ite/itp_codec.h"

#include "i2s/i2s.h"
#include "i2s_reg_9860.h"

/* ************************************************************************** */
/* platform control */
#define I2S_SLAVE_MODE 0U
// #ifdef ENABLE_HDMI_GPIO_IN_SET
//     #define I2S_HDMI_RX_CLK     1
// #else
//     #define I2S_HDMI_RX_CLK     0
// #endif
// #define I2S_DEBUG_SET_CLOCK
// #define I2S_DEBUG_DEINIT_DAC_COST

/* ************************************************************************** */


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
static uint8_t         g_sample_width_    = 0;
static OutType         _hdmiselect = BOUTHOUT;

static pthread_mutex_t I2S_MUTEX   = PTHREAD_MUTEX_INITIALIZER;

#if 0
static void dump_i2s_reg (void)
{
    uint32_t i;
    for (i = 0x0U; i <= 0xC8U; i += 0x04U)
    {
        (void)printf("reg[0x%02x] = 0x%08X\n", i, ithReadRegA(I2S_REG_BASE | i));
    }
    (void)printf("REG_3C = 0x%08X\n", ithReadRegA(MMP_AUDIO_CLOCK_REG_3C));
    (void)printf("REG_40 = 0x%08X\n", ithReadRegA(MMP_AUDIO_CLOCK_REG_40));
}
#endif

static void outputHDMIslect (OutType select);

/* ************************************************************************** */
uint32_t    I2S_DA32_GET_RP (void)
{
    return ithReadRegA(I2S_REG_OUT_RDPTR);
}

uint32_t I2S_DA32_GET_WP (void)
{
    return ithReadRegA(I2S_REG_OUT_WRPTR);
}

void I2S_DA32_SET_WP (uint32_t data)
{
    ithWriteRegA(I2S_REG_OUT_WRPTR, data);
}

void I2S_DA32_WAIT_RP_EQUAL_WP (bool wait)
{
    static bool b_dacRPwait = false;
    bool        b_output; // check i2s enable
    b_dacRPwait = wait;
    b_output    = (0U != (ithReadRegA(I2S_REG_OUT_CTRL) & 0x1U));

    if (b_dacRPwait && b_output)
    {
        uint32_t nSampleRate = 0;
        uint32_t bytes       = 0;
        uint32_t nCh         = ((ithReadRegA(I2S_REG_OUT_CTRL) >> 4U) & 0x1U) + 1U;
        int32_t i           = 0;

        itp_codec_get_i2s_sample_rate(&nSampleRate);
        bytes = 10U * (2U * nSampleRate * nCh) / 1000U; // 10ms data byte;
        ithWriteRegMaskA(I2S_REG_OUT_CTRL, 1U << 2U, 1U << 2U);

        while (GET_DA_RW_GAP > 0U)
        {
            (void)usleep(2000U * DAWAIT((bytes)));
            i++;
            if (i > 100)
            {
                (void)printf("I2S# %d should not be happened\n", __LINE__);
                break;
            }
			if(0U == (ithReadRegA(I2S_REG_OUT_CTRL) & (1U << 2U)))
			{
				break;
			}
        }
    }
    b_dacRPwait = false;
    ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0U << 2U, 1U << 2U);
}

uint32_t I2S_AD32_GET_RP (void)
{
    return ithReadRegA(I2S_REG_IN1_RDPTR);
}

uint32_t I2S_AD32_GET_WP (void)
{
    return ithReadRegA(I2S_REG_IN1_WRPTR);
}

void I2S_AD32_SET_RP (uint32_t data)
{
    ithWriteRegA(I2S_REG_IN1_RDPTR, data);
}

uint32_t I2S_AD32_GET_HDMI_RP (void)
{
    // HDMI RX
    return ithReadRegA(I2S_REG_IN2_RDPTR);
}

uint32_t I2S_AD32_GET_HDMI_WP (void)
{
    // HDMI RX
    return ithReadRegA(I2S_REG_IN2_WRPTR);
}

void I2S_AD32_SET_HDMI_RP (uint32_t data)
{
    // HDMI RX
    ithWriteRegA(I2S_REG_IN2_RDPTR, data);
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

    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);  // Audio Clock Register 1
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
    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);   // Audio Clock Register 1
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
    data32 = ithReadRegA(0xD8000114U);
    data32 |= (((uint32_t)0x1U) << ((uint32_t)31U));
    ithWriteRegA(0xD8000114U, data32);

    ithDelay(100); /* FIXME: dummy loop */

    #if 0
    data32 = ithReadRegA(0xD800011CU);
    data32 |= (((uint32_t)0x1U) << ((uint32_t)31U));
    ithWriteRegA(0xD800011CU, data32);

    ithDelay(100); /* FIXME: dummy loop */;
    #endif
    #if 0
    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);
    data32 |= (((uint32_t)0xFU) << ((uint32_t)28U));//reset DA & AD engine
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);

    ithDelay(100); /* FIXME: dummy loop */;
    #endif

    data32 = ithReadRegA(MMP_AUDIO_CLOCK_REG_3C);
    data32 &= ~(((uint32_t)0xFU) << ((uint32_t)28U)); // normal option DA & AD engine
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C, data32);

    ithDelay(100); /* FIXME: dummy loop */
#endif

    (void)printf("I2S# %s\n", __func__);
}

static void i2s_aws_sync_ (void)
{
    ithWriteRegA(I2S_REG_CODEC_SET,
                 (((uint32_t)1U) << ((uint32_t)31U)) // ADCWS/DACWS syn
                                                     //|( I2S_HDMI_RX_CLK <<  4U) //CLK/WS/DATA1~4 from HDMI RX
                     | ((uint32_t)CFG_I2S_INTERNAL_CODEC << 2U) // select External/Internal DAC
                     | (I2S_SLAVE_MODE << 1U)         // IIS Output Slave/Master Mode
                     | (I2S_SLAVE_MODE << 0U)         // IIS Input Slave/Master Mode
    );
#if 0
    (void)usleep(10);
#endif

    ithWriteRegMaskA(I2S_REG_CODEC_SET, (uint32_t)0U, ((uint32_t)1U) << ((uint32_t)31U));
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
    ithWriteRegA(MMP_AUDIO_CLOCK_REG_40, 0x0002C801U); // AMCLK PLL2_OUT2 /2 ,[9:0]:amclk_ratio (1)

    if (demanded_sample_rate == 32000 || demanded_sample_rate == 48000)
    {
        // 12.288 MHz
        ithWriteRegA(0xD8000120U, 0xa0383701U);
        ithWriteRegA(0xD8000124U, 0x80035c00U);
        ithWriteRegA(0xD8000124U, 0xf0335c00U);
        ithDelay(220);
        ithWriteRegA(0xD8000124U, 0x80035c00U);
    }
    else if (demanded_sample_rate == 44100)
    {
        // 11.2896 MHz
        ithWriteRegA(0xD8000120U, 0xa0292C01U);
        ithWriteRegA(0xD8000124U, 0x8003F800U);
        ithWriteRegA(0xD8000124U, 0xf033F800U);
        ithDelay(220);
        ithWriteRegA(0xD8000124U, 0x8003F800U);
    }
    else
    {
        // 12 MHz
        ithWriteRegA(0xD8000120U, 0x20404001U);
        ithWriteRegA(0xD8000124U, 0x80000000U);
        ithWriteRegA(0xD8000124U, 0xf3000000U);
        ithDelay(220);
        ithWriteRegA(0xD8000124U, 0x80000000U);
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
    #if 0
    (void)printf("data8 = 0x%X demanded_sample_width =0x%X\n",data8,*demanded_sample_width);
    #endif

    // if(_cold_init) ithWriteRegA(MMP_AUDIO_CLOCK_REG_3C,(0xF2A88800|data8));//zclk_ratio
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
    data32 |= ((fading_step & 0xFU) << 8U);
    data32 |= ((fading_duration & 0xFFU) << 16U);
    ithWriteRegA(I2S_REG_OUT_FADE_SET, data32);
}

static void i2s_disable_fading_ (void)
{
    ithWriteRegMaskA(I2S_REG_OUT_FADE_SET, 0U, 1U << 0U); // Disable OUT_BASE1 fade
}

static void GPIO_init_ (int32_t * array, int32_t size, ITHGpioMode mode)
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
        int32_t GpioTable[] = {CFG_GPIO_I2S};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef ENABLE_HDMI_GPIO_OUT_SET
    {
        int32_t GpioTable[] = {CFG_GPIO_I2S_HDMITX};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef ENABLE_HDMI_GPIO_IN_SET
    {
        int32_t GpioTable[] = {CFG_GPIO_I2S_HDMIRX};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
#ifdef __OPENRTOS__
    {
        int32_t GpioTable[] = {CFG_GPIO_I2S_USER};
        GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4);
    }
#endif
    outputHDMIslect(_hdmiselect);
}

static void outputHDMIslect (OutType select)
{
#ifdef ENABLE_HDMI_GPIO_OUT_SET
    uint32_t GpioTable[] = {CFG_GPIO_I2S_HDMITX};
    int32_t          i;
    switch (select)
    {
        case HDMIOUT:
            GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4); // HDMI TX enable
            i2s_mute_volume(true);                                          // L/H out mute;
            break;
        case LHOUT:
            GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE0); // HDMI TX disable
            i2s_mute_volume(false);                                         // L/H out unmute;
            break;
        case BOUTHMUTE:
            GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE0); // HDMI TX disable
            i2s_mute_volume(true);                                          // L/H out unmute;
            break;
        case BOUTHOUT:
            GPIO_init_(GpioTable, ITH_COUNT_OF(GpioTable), ITH_GPIO_MODE4); // HDMI TX enable
            i2s_mute_volume(false);                                         // L/H out unmute;
            break;
        default:
            break;
    }
    _hdmiselect = select;
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

void i2s_pause_ADC (bool yesno)
{
    (void)printf("I2S# %s(%d)\n", __func__, yesno);

    if (yesno)
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
#if 0
        i2s_enable_fading_(0xFU, 0x01U); /* fast response */
        i2s_enable_fading_(0x7U, 0x7FU); /* moderato */
#endif
        i2s_enable_fading_(0x1U, 0xFFU); /* slow response */
        ithWriteRegA(I2S_REG_OUT_VOLUME, 0x007F0100U);
    }
    else
    {
        i2s_disable_fading_();
    }
}

void i2s_pause_DAC (bool yesno)
{
    (void)printf("I2S# %s(%d)\n", __func__, yesno);

    if (yesno)
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
    (void)pthread_mutex_lock(&I2S_MUTEX);
    if (false == gb_i2s_AD_running_)
    {
        (void)printf("I2S# ADC is 'NOT' running, skip deinit ADC !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }

    (void)printf("I2S# %s +\n", __func__);


    itp_codec_rec_deinit();

    /* disable hardware I2S */
    {
        data32 = ithReadRegA(I2S_REG_IN_CTRL);
        data32 &= ~(((uint32_t)3U) << ((uint32_t)0U));
        ithWriteRegA(I2S_REG_IN_CTRL, data32);
    }

    gb_i2s_AD_running_ = false; /* put before i2s_reset_() */
    i2s_reset_();

    i2s_power_off_();

    (void)pthread_mutex_unlock(&I2S_MUTEX);
    (void)printf("I2S# %s -\n", __func__);
}

void i2s_deinit_DAC (void)
{
    uint32_t data32;

   (void)pthread_mutex_lock(&I2S_MUTEX);

    if (!gb_i2s_DA_running_)
    {
        (void)printf("I2S# DAC is 'NOT' running, skip deinit DAC !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
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
    data32 &= ~((uint32_t)0x7U);
    ithWriteRegA(I2S_REG_OUT_CTRL, data32);

    gb_i2s_DA_running_ = false; /* put before i2s_reset_() */
    i2s_reset_();

    i2s_power_off_();

    (void)printf("I2S# %s -\n", __func__);
    (void)pthread_mutex_unlock(&I2S_MUTEX);
}

void i2s_init_DAC (STRC_I2S_SPEC * i2s_spec)
{
    uint32_t data32 = 0U;
    uint32_t resolution_type;
    uint32_t hdmi_num;

    (void)pthread_mutex_lock(&I2S_MUTEX);
    if (gb_i2s_DA_running_ && itp_codec_get_DA_running())
    {
        (void)printf("I2S# DAC is running, skip re-init !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }
    if ((((uint32_t)i2s_spec->base_i2s % 8U) > 0U) || ((i2s_spec->buffer_size % 8U) > 0U))
    {
        (void)printf("ERROR# I2S, bufbase/bufsize must be 8-Bytes alignment !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }
    if ((i2s_spec->channels != 1U) && (i2s_spec->channels != 2U))
    {
        (void)printf("ERROR# I2S, only support single or two channels !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }

    (void)printf("I2S# %s Start\n", __func__);
    

    i2s_power_on_();

    i2s_set_GPIO_();

    i2s_set_clock_(i2s_spec->sample_rate, &g_sample_width_);

#if 0
    i2s_reset_();
#endif

    /* mastor mode & AWS & ZWS sync */
    switch (i2s_spec->sample_size)
    {
        case 16:
            resolution_type = 0U;
            break;

        case 24:
            resolution_type = 1U;
            break;

        case 32:
            resolution_type = 2U;
            break;

        case 8:
            resolution_type = 3U;
            break;

        default:
            resolution_type = 2U;
            break;
    }

    data32 = (resolution_type << 28U)    /* ADC resolution bits, 00:16bits; 01:24bits; 10:32bits; 11:8bits */
             | (((uint32_t)1U) << 24U)   /* WS mute check, 0:Disable 1:Enable */
             | (((uint32_t)0xFU) << 16U) /* WS mute check times */
             | (g_sample_width_ << 0U);  /* sample width (in bits unit) */
    ithWriteRegA(I2S_REG_DAC_SRATE_SET, data32);
    ithWriteRegA(I2S_REG_ADC_SRATE_SET, data32);

    /* 970 remove PCM support*/
    i2s_aws_sync_();

    /*Faraday audio init*/
    if (i2s_spec->enable_Speaker)
    {
        itp_codec_playback_init(i2s_spec->channels); // 0:HPR only 1:HPL only 2:HPRL both
    }
    itp_codec_set_i2s_sample_rate(i2s_spec->sample_rate);

    /* buffer base */
#if 0
    for (i = 0; i < 4; i++)
    {
        if (i2s_spec->base_hdmi[i] == NULL)
        {
            i2s_spec->base_hdmi[i] = i2s_spec->base_i2s;
        }
    }
#endif
    ithWriteRegA(I2S_REG_OUT_BASE1, (uint32_t)i2s_spec->base_i2s); /*(IIS /SPDIF out) base 1*/
#if 0
    ithWriteRegA(I2S_REG_OUT_BASE2, (uint32_t)i2s_spec->base_hdmi[0]);/*(HDMI Data0 out) base 2*/
    ithWriteRegA(I2S_REG_OUT_BASE3, (uint32_t)i2s_spec->base_hdmi[1]);/*(HDMI Data1 out) base 3*/
    ithWriteRegA(I2S_REG_OUT_BASE4, (uint32_t)i2s_spec->base_hdmi[2]);/*(HDMI Data2 out) base 4*/
    ithWriteRegA(I2S_REG_OUT_BASE5, (uint32_t)i2s_spec->base_hdmi[3]);/*(HDMI Data3 out) base 5*/
#endif

    /* buffer length */
    ithWriteRegA(I2S_REG_OUT_LEN, i2s_spec->buffer_size - 1U);

    /* DA starvation interrupt threshold */
    ithWriteRegA(I2S_REG_OUT_RdWrGAP, 0x00000080U); // OutRdWrGap[31:0]

    /*DA output Fade in/out reset*/
    ithWriteRegA(I2S_REG_OUT_FADE_SET, 0x00010100U); // default:0x00010100
    /* output path */
    hdmi_num = 0U;
#if 0
    switch(i2s_spec->num_hdmi_audio_buffer){
        case 1: { hdmi_num |=  1; break; } /* buf 1 (HDMI Data0) */
        case 2: { hdmi_num |=  3; break; }
        case 3: { hdmi_num |=  7; break; }
        case 4: { hdmi_num |= 15; break; }
        case 0:  break;
    }
#endif

    /*should 970 set W0OutRqSize[3:0]=0b1000 ?*/
    ithWriteRegA(I2S_REG_OUT_CTRL,
                 (hdmi_num << 9U)             /* HDMI DATA Output*/
                     | (((uint32_t)1U) << 8U) /* Enable IIS 1/SPDIF Data Output */
                     | (0U << 6U)             /* Output Channel active level, 0:Low for Left; 1:High for Left */
                     | (0U << 5U)             /* Output Interface Format, 0:IIS Mode; 1:MSB Left-Justified Mode */
                     | ((i2s_spec->channels - 1U) << 4U) /* Output Channel Select, 0:Single Channel; 1:Two Channels */
                     | ((i2s_spec->is_big_endian ? 1U : 0U) << 3U) /* 0:Little Endian; 1:Big Endian */
                     | (0U << 2U)                                  /* 0:NOT_LAST_WPTR; 1:LAST_WPTR */
                     | (1U << 1U)                                  /* Fire the IIS for Audio Output */
    );

    I2S_DA32_SET_WP(I2S_DA32_GET_RP()); // reset rw point;

    if (!(i2s_spec->postpone_audio_output))
    {
        i2s_pause_DAC(false); /* Enable Audio Output */
    }
    else
    {
        i2s_pause_DAC(true);
    }

    gb_i2s_DA_running_ = true;

    (void)printf("I2S# %s End\n", __func__);
    (void)pthread_mutex_unlock(&I2S_MUTEX);
}

void i2s_init_ADC (STRC_I2S_SPEC * i2s_spec)
{
    int32_t  i;
    uint32_t data32 = 0U;
    uint32_t resolution_type;
    uint32_t hdmi_num;

    (void)pthread_mutex_lock(&I2S_MUTEX);
    if (gb_i2s_AD_running_ && itp_codec_get_AD_running())
    {
        (void)printf("I2S# ADC is running, skip re-init ADC !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }
    if ((((uint32_t)i2s_spec->base_i2s % 8U) > 0U) || ((i2s_spec->buffer_size % 8U) > 0U))
    {
        (void)printf("ERROR# I2S, bufbase/bufsize must be 8-Bytes alignment !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }
    if ((i2s_spec->channels != 1U) && (i2s_spec->channels != 2U))
    {
        (void)printf("ERROR# I2S, only support single or two channels !\n");
        (void)pthread_mutex_unlock(&I2S_MUTEX);
        return;
    }

    (void)printf("I2S# %s Start\n", __func__);

    i2s_power_on_();

    i2s_set_GPIO_();

    i2s_set_clock_(i2s_spec->sample_rate, &g_sample_width_);

#if 0
    i2s_reset_();
#endif

    itp_codec_set_i2s_sample_rate(i2s_spec->sample_rate);

    /* mastor mode & AWS & ZWS sync */
    switch (i2s_spec->sample_size)
    {
        case 16:
            resolution_type = 0U;
            break;

        case 24:
            resolution_type = 1U;
            break;

        case 32:
            resolution_type = 2U;
            break;

        case 8:
            resolution_type = 3U;
            break;

        default:
            resolution_type = 2U;
            break;
    }

    /* config AD */
    data32 = (resolution_type << 28U)    /* ADC resolution bits, 00:16bits; 01:24bits; 10:32bits; 11:8bits */
             | (((uint32_t)1U) << 24U)   /* WS mute check, 0:Disable 1:Enable */
             | (((uint32_t)0xFU) << 16U) /* WS mute check times */
             | (g_sample_width_ << 0U);  /* sample width (in bits unit) */
    ithWriteRegA(I2S_REG_ADC_SRATE_SET, data32);
    ithWriteRegA(I2S_REG_DAC_SRATE_SET, data32);

    i2s_aws_sync_();

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
    ithWriteRegA(I2S_REG_IN_LEN, i2s_spec->buffer_size - 1U);

    /* DA starvation interrupt threshold */
    ithWriteRegA(I2S_REG_IN_RdWrGAP, 0x00000040U); // OutRdWrGap[31:0]

    hdmi_num = 0U;
    switch (i2s_spec->num_hdmi_audio_buffer)
    {
        case 1: /* buf 1 (HDMI Data0) */
            hdmi_num |= 1U;
            break;

        case 2:
            hdmi_num |= 3U;
            break;

        case 3:
            hdmi_num |= 7U;
            break;

        case 4:
            hdmi_num |= 15U;
            break;

        default:
            hdmi_num |= 0U;
            break;
    }

    ithWriteRegA(
        I2S_REG_IN_CTRL,
        (hdmi_num << 12U)                      /* Eanble HDMI Data Input */
            | (((uint32_t)1U) << 8U)           /* Eanble Input1 IIS Data Input */
            | (0U << 6U)                       /* Input Channel active level, 0:Low for Left; 1:High for Left */
            | (0U << 5U)                       /* Input Interface Format, 0:IIS Mode; 1:MSB Left-Justified Mode */
            | ((i2s_spec->channels - 1U) << 4U) /* Input Channel, 0:Single Channel; 1:Two Channels */
            | ((i2s_spec->is_big_endian ? 1U : 0U) << 3U) /* 0:Little Endian; 1:Big Endian */
            | ((i2s_spec->record_mode ? 1U : 0U) << 2U) /* 0:AV Sync Mode (wait capture to start); 1:Only Record Mode */
            | (1U << 1U)                                /* Fire the IIS for Audio Input */
            | (1U << 0U)                                /* Enable Audio Input */
    );

    gb_i2s_AD_running_ = true;

    (void)pthread_mutex_unlock(&I2S_MUTEX);
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
    }
    else /* resume */
    {
        itp_codec_playback_unmute();
    }
    gb_i2s_DA_mute_ = mute;
}

void i2s_set_direct_volperc (uint32_t volperc)
{
    /*volperc: 0~100*/
    itp_codec_playback_set_direct_volperc(volperc);
    if (0U == volperc)
    {
        i2s_mute_volume(true); // mute
    }
    else if ((!gb_i2s_DA_mute_) && gb_bar_mute_flag_)
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
    uint32_t currvolperc = 0U;
    itp_codec_playback_get_currvol(&currvolperc);
    return currvolperc;
}

#if 0
void i2s_set_direct_RLvolperc (uint32_t volperc, uint8_t RL)
{
    /*volperc: 0~100*/
    itp_codec_playback_set_direct_RLvolperc(volperc, RL);
}

uint32_t i2s_get_current_RLvolperc (uint8_t RL)
{
    uint32_t currvolperc = 0U;
    itp_codec_playback_get_RLcurrvol(&currvolperc, RL);
    return currvolperc;
}
#endif

void i2s_ADC_set_rec_volperc (uint32_t recperc)
{
    /*recperc:0~100*/
    itp_codec_rec_set_direct_volperc(recperc);
    if (0U == recperc)
    {
        i2s_mute_ADC(true); // mute
    }
    else if (gb_i2s_AD_mute_)
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
    uint32_t currmicperc = 0U;
    itp_codec_rec_get_currvol(&currmicperc);
    return currmicperc;
}
#if 0
void i2s_ADC_set_rec_RLvolperc (uint32_t recperc, uint8_t RL)
{
    /*recperc:0~100*/
    itp_codec_rec_set_direct_RLvolperc(recperc, RL);
}

uint32_t i2s_ADC_get_current_RLvolstep (uint8_t RL)
{
    uint32_t currmicperc = 0U;
    itp_codec_rec_get_RLcurrvol(&currmicperc, RL);
    return currmicperc;
}
#endif

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
    uint8_t * base_i2s    = (uint8_t *)GET_DA_BASE;
    uint32_t  buffer_size = GET_DA_BASE_LEN;

    if ((DA_w + size) > buffer_size)
    {
        uint32_t szbuf = buffer_size - DA_w;
        if (szbuf > 0U)
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
    uint8_t * base_i2s    = (uint8_t *)GET_AD_BASE;
    uint32_t  buffer_size = GET_AD_BASE_LEN;

    if ((AD_r + size) <= buffer_size)
    {

        ithInvalidateDCacheRange(base_i2s + AD_r, size);
        (void)memcpy(ptr, base_i2s + AD_r, size);
        AD_r += size;
    }
    else
    {
        // AD_r > AD_w
        uint32_t szsec0 = buffer_size - AD_r;
        uint32_t szsec1 = size - szsec0;
        if (szsec0 > 0U)
        {
            ithInvalidateDCacheRange(base_i2s + AD_r, szsec0);
            (void)memcpy(ptr, base_i2s + AD_r, szsec0);
        }
        ithInvalidateDCacheRange(base_i2s, szsec1);
        (void)memcpy(ptr + szsec0, base_i2s, szsec1);
        AD_r = szsec1;
    }

    I2S_AD32_SET_RP(AD_r);

    return 1;
}
