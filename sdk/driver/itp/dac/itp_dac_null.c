/* sy.chuang, 2013-0301, ITE Tech. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ite/ith.h"
#include "ite/itp.h"

#include "i2s/i2s_reg.h"
#include "i2s/i2s_digvol_tb.inc"

/* ************************************************************************** */
/* wrapper */
static inline uint16_t reg_read16 (uint16_t addr16)
{
    return ithReadRegH(addr16);
}
static inline void reg_write16 (uint16_t addr16, uint16_t data16)
{
    ithWriteRegH(addr16, data16);
}
static inline void i2s_delay_us (uint32_t us)
{
    ithDelay(us);
}

/* ************************************************************************** */
#define SERR()                                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)printf("ERROR# %s:%d, %s\n", __FILE__, __LINE__, __func__);                                              \
        while (true)                                                                                                   \
        {                                                                                                              \
        }                                                                                                              \
    } while (false)
#define S()                                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)printf("=> %s:%d, %s\n", __FILE__, __LINE__, __func__);                                                  \
    } while (false)

/* ************************************************************************** */

/* ************************************************************************** */
void itp_codec_depop (void)
{
}

/* === common DAC/ADC ops === */
void itp_codec_wake_up (void)
{
}

void itp_codec_standby (void)
{
}

/* DAC */
void itp_codec_playback_init (uint32_t output)
{
}

void itp_codec_playback_deinit (void)
{
}

void itp_codec_playback_amp_volume_down (void)
{
}

void itp_codec_playback_amp_volume_up (void)
{
}

void itp_codec_playback_set_direct_vol (uint32_t target_vol)
{
}

void itp_codec_playback_set_direct_volperc (uint32_t target_volperc)
{
}

void itp_codec_playback_get_currvol (uint32_t * currvol)
{
}

void itp_codec_playback_get_currvolperc (uint32_t * currvolperc)
{
}

void itp_codec_playback_get_vol_range (uint32_t * max, uint32_t * regular_0db, uint32_t * min)
{
}

void itp_codec_playback_set_direct_RLvol (uint32_t volstep, uint8_t RL)
{
}

void itp_codec_playback_set_direct_RLvolperc (uint32_t target_volperc, uint8_t RL)
{
}

void itp_codec_playback_get_RLcurrvol (uint32_t * currvolperc, uint8_t RL)
{
}

void itp_codec_playback_mute (void)
{
}

void itp_codec_playback_unmute (void)
{
}

void itp_codec_playback_linein_bypass (uint32_t bypass)
{
    /* line-in bypass to line-out directly */
    /* TODO */
    (void)printf("ERROR# This DAC does NOT support bypass function:(%02x)\n", bypass);
}

/* ADC */
void itp_codec_rec_init (uint32_t input_source)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
    (void)input_source;
}

void itp_codec_rec_deinit (void)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
}

void itp_codec_rec_set_direct_volperc (uint32_t target_recperc)
{
}

void itp_codec_rec_set_direct_vol (uint32_t target_vol)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
    (void)target_vol;
}

void itp_codec_rec_get_currvol (uint32_t * currvol)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
    *currvol = 0;
}

void itp_codec_rec_set_direct_RLvol (uint32_t micstep, uint8_t RL)
{
}

void itp_codec_rec_set_direct_RLvolperc (uint32_t target_micperc, uint8_t RL)
{
}

void itp_codec_rec_get_RLcurrvol (uint32_t * currvol, uint8_t RL)
{
}

void itp_codec_rec_get_vol_range (uint32_t * max, uint32_t * regular_0db, uint32_t * min)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
    *max         = 0U;
    *regular_0db = 0U;
    *min         = 0U;
}

void itp_codec_rec_mute (void)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
}

void itp_codec_rec_unmute (void)
{
    /* HARDWARE NOT SUPPORT RECORDING ! */
}

void itp_codec_power_on (void)
{
}

void itp_codec_power_off (void)
{
}

void itp_codec_get_i2s_sample_rate (uint32_t * sample_rate)
{
    (void)sample_rate;
}

void itp_codec_set_i2s_sample_rate (uint32_t sample_rate)
{
    (void)sample_rate;
}

bool itp_codec_get_DA_running (void)
{
    return false;
}

bool itp_codec_get_AD_running (void)
{
    return false;
}
