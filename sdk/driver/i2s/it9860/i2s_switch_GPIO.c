#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ite/ith.h"
#include "ite/itp_codec.h"
#include "i2s/i2s.h"
#include "i2s_reg_9860.h"

void i2s_DAC_channel_switch (int32_t channels, int32_t RL)
{
    int32_t Creg = channels - 1;
    bool reg  = !RL;
    if (channels != 1 && channels != 2)
    {
        (void)printf("channels(%d) must be 1 or 2 :set channel as 2\n", channels);
        reg = 2;
    }
    if (channels == 2)
    {
        itp_codec_playback_init(channels);
    }
    else
    {
        itp_codec_playback_init(RL);
    }
    ithWriteRegMaskA(I2S_REG_OUT_CTRL, reg << 6 | Creg << 4, 1 << 6 | 1 << 4);
#if 0
    (void)printf("channels %d RL %d Creg=%d reg=%d\n",channels,RL,Creg,reg);
#endif
}

void i2s_ADC_channel_switch (int32_t channels, int32_t RL)
{
    int32_t Creg = channels - 1;
    bool reg  = !RL;
    if (channels != 1 && channels != 2)
    {
        (void)printf("channels(%d) must be 1 or 2 :set channel as 2\n", channels);
        Creg = 2;
    }

    if (channels == 2)
    {
        itp_codec_rec_init(channels);
    }
    else
    {
        itp_codec_rec_init(RL);
    }
    ithWriteRegMaskA(I2S_REG_IN_CTRL, reg << 6 | Creg << 4, 1 << 6 | 1 << 4);
    //(void)printf("channels %d RL %d Creg=%d reg=%d\n",channels,RL,Creg,reg);
}

void i2s_loopback_set (uint8_t * ptr, int32_t number)
{
    /*loopbac DA to HDMI IN 0*/
    if (ptr != NULL)
    {

        switch (number)
        {
            case 0:
                ithWriteRegA(I2S_REG_IN2_BASE1, (uint32_t)ptr);
                break;
            case 1:
                ithWriteRegA(I2S_REG_IN2_BASE2, (uint32_t)ptr);
                break;
            case 2:
                ithWriteRegA(I2S_REG_IN2_BASE3, (uint32_t)ptr);
                break;
            case 3:
                ithWriteRegA(I2S_REG_IN2_BASE4, (uint32_t)ptr);
                break;
            default:
                ithWriteRegA(I2S_REG_IN2_BASE1, (uint32_t)ptr);
                break;
        }
        ithWriteRegMaskA(I2S_REG_IN_CTRL, 1 << (12 + number), 1 << (12 + number)); // enable base[number]
        ithWriteRegMaskA(I2S_REG_DATA_LOOPBACK, 1 << (5 + number), 1 << (5 + number));
    }
    else
    {
        ithWriteRegMaskA(I2S_REG_IN_CTRL, 0 << (12 + number), 0 << (12 + number)); // enable base[number]
        ithWriteRegMaskA(I2S_REG_DATA_LOOPBACK, 0 << (5 + number), 0 << (5 + number));
    }
}

uint32_t i2s_ad2_data_get (uint8_t * ptr, uint32_t size, int32_t number)
{ // get data from AD2 base[number]
    uint32_t  AD_r = I2S_AD32_GET_HDMI_RP();
    uint8_t * base_i2s;
    uint32_t  buffer_size = GET_AD_BASE_LEN;
    switch (number)
    {
        case 0:
            base_i2s = (uint8_t *)ithReadRegA(I2S_REG_IN2_BASE1);
            break;
        case 1:
            base_i2s = (uint8_t *)ithReadRegA(I2S_REG_IN2_BASE2);
            break;
        case 2:
            base_i2s = (uint8_t *)ithReadRegA(I2S_REG_IN2_BASE3);
            break;
        case 3:
            base_i2s = (uint8_t *)ithReadRegA(I2S_REG_IN2_BASE4);
            break;
        default:
            base_i2s = (uint8_t *)ithReadRegA(I2S_REG_IN2_BASE1);
            break;
    }

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

    I2S_AD32_SET_HDMI_RP(AD_r);

    return 1;
}

// haved play DA data , for AEC Align
static uint32_t PLAYRP = 0;

uint32_t        I2S_PLAY32_GET_RP (void)
{
    return PLAYRP;
}

uint32_t I2S_PLAY32_GET_PLAY_WP (void)
{
    return I2S_DA32_GET_RP();
}

void I2S_PLAY32_SET_PLAY_RP (uint32_t data32)
{
    PLAYRP = data32;
}

uint32_t GET_PLAY_RW_GAP (void)
{
    uint32_t wp = I2S_PLAY32_GET_PLAY_WP();
    uint32_t rp = I2S_PLAY32_GET_RP();
    return ((wp >= rp) ? (wp - rp) : (GET_DA_BASE_LEN - rp + wp));
}

uint32_t i2s_played_data_get (uint8_t * ptr, uint32_t size)
{
    uint32_t  PL_r        = I2S_PLAY32_GET_RP();
    uint8_t * base_i2s    = (uint8_t *)GET_DA_BASE;
    uint32_t  buffer_size = GET_DA_BASE_LEN;

    if (PL_r + size <= buffer_size)
    {
        (void)memcpy(ptr, base_i2s + PL_r, size);
        PL_r += size;
    }
    else
    { // PL_r > AD_w
        uint32_t szsec0 = buffer_size - PL_r;
        uint32_t szsec1 = size - szsec0;
        if (szsec0)
        {
            (void)memcpy(ptr, base_i2s + PL_r, szsec0);
        }
        (void)memcpy(ptr + szsec0, base_i2s, szsec1);
        PL_r = szsec1;
    }

    I2S_PLAY32_SET_PLAY_RP(PL_r);

    return 1;
}

int16_t clamp16 (int32_t sample)
{
    if ((sample >> 15) ^ ((int64_t)sample >> 31))
    {
        sample = 0x7FFE ^ ((int64_t)sample >> 31);
    }
    return sample;
}
