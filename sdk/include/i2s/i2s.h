/* sy.chuang, 2012-0423, ITE Tech. */

#ifndef I2S_H
#define I2S_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IISBASE         0xD0100000U

#define GET_DA_BASE     ithReadRegA(IISBASE | 0x70U)
#define GET_DA_BASE_LEN ((uint32_t)ithReadRegA(IISBASE | 0x90U) + 1U)
#define GET_DA_RW_RGAP                                                                                                 \
    ((ithReadRegA(IISBASE | 0xC0U) >= ithReadRegA(IISBASE | 0xC4U))                                                    \
         ? (ithReadRegA(IISBASE | 0xC0U) - ithReadRegA(IISBASE | 0xC4U))                                               \
         : ((GET_DA_BASE_LEN - ithReadRegA(IISBASE | 0xC4U)) + ithReadRegA(IISBASE | 0xC0U))) // i2s rw ptr real gap
#define GET_DA_RW_GAP   ithReadRegA(IISBASE | 0xC8U) // i2s rw ptr transition gap
#define GET_AD_BASE     ithReadRegA(IISBASE | 0x10U)
#define GET_AD_BASE_LEN ((uint32_t)ithReadRegA(IISBASE | 0x30U) + 1U)
#define GET_AD_RW_GAP                                                                                                  \
    ((ithReadRegA(IISBASE | 0x54U) >= ithReadRegA(IISBASE | 0x50U))                                                    \
         ? (ithReadRegA(IISBASE | 0x54U) - ithReadRegA(IISBASE | 0x50U))                                               \
         : ((GET_AD_BASE_LEN - ithReadRegA(IISBASE | 0x50U)) + ithReadRegA(IISBASE | 0x54U)))
#define GET_AD2_RW_GAP                                                                                                 \
    ((ithReadRegA(IISBASE | 0x5CU) >= ithReadRegA(IISBASE | 0x58U))                                                    \
         ? (ithReadRegA(IISBASE | 0x5CU) - ithReadRegA(IISBASE | 0x58U))                                               \
         : ((GET_AD_BASE_LEN - ithReadRegA(IISBASE | 0x58U)) + ithReadRegA(IISBASE | 0x5CU)))
#define DAWAITR(x) (1U + GET_DA_RW_RGAP / x)
#define DAWAIT(x)  (1U +( GET_DA_RW_GAP / x ))
#define EVEN(x)    (x &= ~1)
// GET_DA_RW_RGAP usually equal GET_DA_RW_GAP while i2s working,
// GET_DA_RW_GAP may not correct while i2s start
/* ************************************************************************** */
typedef struct
{
    /* input/ouput common */
    uint32_t  channels;    /* ex: 1(mono) or 2(stereo) */
    uint32_t  sample_rate; /* ex: 44100/48000 Hz */
    uint32_t  buffer_size;
    bool      is_big_endian;
    uint8_t * base_i2s;
    uint32_t  sample_size; /* ex: 16/24/32 bits */

    /* for input use */
    uint32_t  from_LineIN;
    bool      from_MIC_IN;
    uint32_t  from_PDM_IN;

    /* for output use */
    uint32_t  num_hdmi_audio_buffer; /* 0: no hdmi audio output (could save bandwidth) */
    uint32_t  is_dac_spdif_same_buffer;
    uint8_t * base_hdmi[4];          /*hdmi buffer base (TX RX are same)*/
    uint8_t * base_spdif;            /*spdif   buffer base*/
    bool  postpone_audio_output; /* manually enable audio output */

    /* for input use */
    bool  record_mode; /* 0: hardware start via capture hardware, 1: hardware start via software set */

    /* for output use */
    bool  enable_Speaker;   // speaker-out
    uint32_t  enable_HeadPhone; // line-out. If "enable_Speaker"=0, "enable_HeadPhone" will be set as true in i2s driver
                                // defaut setting.
} STRC_I2S_SPEC;

typedef enum OutType_t
{
    HDMIOUT = 0,
    LHOUT,
    BOUTHOUT,
    BOUTHMUTE,
} OutType;

#if CFG_I2S_FOR_BT
typedef enum _Resetmode
{
    ADreset = 0,
    DAreset,
    Reset,
} Resetmode;
#endif
/* ************************************************************************** */
/* DA */
uint32_t I2S_DA32_GET_RP (void);
uint32_t I2S_DA32_GET_WP (void);
void     I2S_DA32_SET_WP (uint32_t data);
void     I2S_DA32_WAIT_RP_EQUAL_WP(bool wait);
bool     i2s_is_DAstarvation (void);

bool     i2s_get_DA_running (void);
bool     i2s_get_AD_running (void);
uint32_t I2S_AD32_GET_RP (void);
uint32_t I2S_AD32_GET_WP (void);
void     I2S_AD32_SET_RP (uint32_t data);

/*HDMI*/
uint32_t I2S_AD32_GET_HDMI_RP (void);
uint32_t I2S_AD32_GET_HDMI_WP (void);
void     I2S_AD32_SET_HDMI_RP (uint32_t data);

/********************** export APIs **********************/
void     i2s_CODEC_wake_up (void);
void     i2s_CODEC_standby (void);

/* DA */
void     i2s_volume_up (void);
void     i2s_volume_down (void);
void     i2s_pause_DAC (bool yesno);
void     i2s_deinit_DAC (void);
void     i2s_init_DAC (STRC_I2S_SPEC * i2s_spec);
void     i2s_mute_DAC (bool mute);
void     i2s_set_direct_volperc (uint32_t volperc);
uint32_t i2s_get_current_volperc (void);
void     i2s_mute_volume (bool mute);
void     i2s_enable_fading (bool yesno);
void     i2s_fadepause_DAC(bool yesno,uint32_t fading_step, uint32_t fading_duration);
uint32_t i2s_da_data_put (uint8_t * ptr, uint32_t size);
void     i2s_DAC_channel_switch (int32_t channels, int32_t RL);
uint32_t GET_PLAY_RW_GAP (void);
/* AD */
void     i2s_pause_ADC (bool yesno);
void     i2s_deinit_ADC (void);
void     i2s_init_ADC (STRC_I2S_SPEC * i2s_spec);
void     i2s_ADC_set_direct_volstep (uint32_t recstep);
void     i2s_ADC_set_rec_volperc (uint32_t recperc);
uint32_t i2s_ADC_get_current_volstep (void);
void     i2s_ADC_get_volstep_range (uint32_t * max, uint32_t * normal, uint32_t * min);
void     i2s_mute_ADC (bool mute);
uint32_t i2s_ad_data_get (uint8_t * ptr, uint32_t size);
void     i2s_ADC_channel_switch (int32_t channels, int32_t RL);
void     i2s_loopback_set (uint8_t * ptr, int32_t number);
uint32_t i2s_ad2_data_get (uint8_t * ptr, uint32_t size, int32_t number);
#if CFG_I2S_FOR_BT
void i2s_set_externalCODEC (bool is_external);
bool i2s_is_externalCODEC ();

#endif
/*DSP*/
int16_t clamp16 (int32_t sample);

/* ************************************************************************** */
#ifdef __cplusplus
}
#endif

#endif // I2S_H
