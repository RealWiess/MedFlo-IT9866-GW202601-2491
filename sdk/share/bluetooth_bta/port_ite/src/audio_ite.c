#include <stdbool.h>
#include "bta_platform.h"
#include "bte_glue.h"
#include "i2s/i2s.h"
#include "ite/itp.h"

#include "btm_api.h"
#include "audio_ite.h"
#include "audio_ctrl.h"

#define DAC_BUFFER_SIZE (48 * 1024) // if sampleRate:44KHz*2ch*16bit=172KB/S, can hold stream ~0.28s
#define ADC_BUFFER_SIZE (4 * 1024)  // nbs(8k,16b):0.4s, wbs(16k,16b):0.2s

static uint8_t *     dac_buf        = NULL;
static uint8_t *     adc_buf        = NULL;
static STRC_I2S_SPEC playback_param = {0}; // da speak
static STRC_I2S_SPEC capture_param  = {0}; // ad microphone
static STRC_I2S_SPEC ite_audio_param;
static bool          audio_play = false, bt_link = false;

void                 audio_init_DA (STRC_I2S_SPEC * i2s_spec);
void                 audio_pause_DA (bool pause);
void                 audio_pause_AD (bool pause);
void                 audio_deinit_AD ();
void                 audio_deinit_DA ();
void                 audio_DA_volume_set (uint32_t level);
void                 audio_AD_volume_set (uint32_t level);

#define MIC_EN 1

#if MIC_EN
void AUDIO_Capture_SCO_2_ite_Params (uint16_t codec_type, STRC_I2S_SPEC * param)
{
    APPL_TRACE_API1("AUDIO_Capture_SCO_2_ite_Params codec:%d", codec_type);

    if (adc_buf == NULL)
    {
        adc_buf = GKI_os_malloc(ADC_BUFFER_SIZE);
        memset((uint8_t *)dac_buf, 0, DAC_BUFFER_SIZE);
    }

    /* init ADC */
    param->channels      = 1; // 2
    param->sample_rate   = (codec_type == BTM_SCO_CODEC_MSBC ? 16000 : 8000);
    param->buffer_size   = ADC_BUFFER_SIZE;
    param->is_big_endian = false;
    param->base_i2s      = (uint8_t *)adc_buf;
    param->sample_size   = 16;
    param->record_mode   = 1;
    param->from_LineIN   = false;
    param->from_MIC_IN   = true;
}
#endif // ENDIF MIC_EN

void AUDIO_Playback_SCO_2_ite_Params (uint16_t codec_type, STRC_I2S_SPEC * param)
{

    APPL_TRACE_API1("AUDIO_Playback_SCO_2_ite_Params codec:%d", codec_type);

    if (dac_buf == NULL)
    {
        dac_buf = (uint8_t *)GKI_os_malloc(DAC_BUFFER_SIZE);
        memset((uint8_t *)dac_buf, 0, DAC_BUFFER_SIZE);
    }

    /* init DAC */
    param->channels                 = 2;
    param->sample_rate              = (codec_type == BTM_SCO_CODEC_MSBC ? 16000 : 8000);
    param->buffer_size              = ADC_BUFFER_SIZE; // ADC_BUFFER_SIZE; //;
    param->is_big_endian            = false;
    param->base_i2s                 = (uint8_t *)dac_buf;
    param->sample_size              = 16;
    param->num_hdmi_audio_buffer    = 0;
    param->is_dac_spdif_same_buffer = 1;
    param->enable_Speaker           = 1;
    param->enable_HeadPhone         = 1;
    param->postpone_audio_output    = true;
    param->base_spdif               = (uint8_t *)dac_buf;
}

void AUDIO_Playback_init_ite_Params (uint8_t ch, uint32_t sample_rate, STRC_I2S_SPEC * param)
{
    APPL_TRACE_API2("AUDIO_Playback_init_ite_Params: ch:%d, sample_rate:%d", ch, sample_rate);

    if (dac_buf == NULL)
    {
        dac_buf = GKI_os_malloc(DAC_BUFFER_SIZE);
    }

    memset(dac_buf, 0, sizeof(DAC_BUFFER_SIZE));

    param->channels                 = ch,
    param->sample_rate              = sample_rate; // sample_rate;
    param->buffer_size              = DAC_BUFFER_SIZE;
    param->is_big_endian            = false;
    param->base_i2s                 = (uint8_t *)dac_buf;
    param->sample_size              = 16;
    param->num_hdmi_audio_buffer    = 0;
    param->is_dac_spdif_same_buffer = 1;
    param->enable_Speaker           = 1;
    param->enable_HeadPhone         = 1;
    param->postpone_audio_output    = true;
    param->base_spdif               = (uint8_t *)dac_buf;
}

void audio_mirror_init (void)
{
    if (bt_link)
    {
        audio_deinit_DA();
        audio_init_DA(&playback_param);
        audio_pause_DA(false);
    }

    audio_play = true;
}

void audio_mirror_stop (void)
{
    audio_play = false;
    audio_deinit_DA();
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
}

void AUDIO_Ucodec_2_ite_Params (tUCODEC_CNF_SBC * sbc, STRC_I2S_SPEC * param)
{
    APPL_TRACE_API0("AUDIO_Ucodec_2_ite_Params");
    switch (sbc->ChannelMode)
    {
        /* If the current SBC is Mono */
        case UCODEC_CHN_MONO:
            param->channels = 1;
            break;
        case UCODEC_CHN_DUAL:
        case UCODEC_CHN_STEREO:
        case UCODEC_CHN_JOINT_STEREO:
            param->channels = 2;
            break;
        default:
            param->channels = 0;
#if 0
            (void)printf("AUDIO_Ucodec_2_ite_Params error bad sbc_channel_mode:%d", sbc->ChannelMode);
#endif
            break;
    }

    if (dac_buf == NULL)
    {
        dac_buf = GKI_os_malloc(DAC_BUFFER_SIZE);
    }

    param->sample_rate              = ucodec_get_samplerate_value(sbc->SampleFreq);
    param->buffer_size              = DAC_BUFFER_SIZE;
    param->is_big_endian            = false;
    param->base_i2s                 = (uint8_t *)dac_buf;
    param->sample_size              = 16;
    param->num_hdmi_audio_buffer    = 0;
    param->is_dac_spdif_same_buffer = 1;
    param->enable_Speaker           = 1;
    param->enable_HeadPhone         = 1;
    param->postpone_audio_output    = true;
    param->base_spdif               = (uint8_t *)dac_buf;
    audio_init_DA(param);
    audio_pause_DA(true);
    bt_link = true;
}

int AUDIO_Playback_Open (void)
{
    APPL_TRACE_API0("AUDIO_Playback_Open");
    audio_pause_DA(false);
    bt_link = true;
    return 0;
}

int AUDIO_Playback_Close (void)
{
    APPL_TRACE_API0("AUDIO_Playback_Close");
    audio_pause_DA(true);
    bt_link = false;
    return 0;
}

int AUDIO_Playback_Configure (STRC_I2S_SPEC * param)
{
    APPL_TRACE_API0("AUDIO_Playback_Configure");
    if (memcmp(&playback_param, param, sizeof(playback_param)))
    {
        APPL_TRACE_API0("reinit DA");
        memcpy(&playback_param, param, sizeof(playback_param));
        audio_pause_DA(true);
        audio_deinit_DA();
        audio_deinit_AD();
        audio_init_DA(&playback_param);
        I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    }

    return 0;
}

void AUDIO_Playback_Play (uint8_t * data, int len)
{

#if (defined BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE)
    extern int btapp_codec_wavein_cback(uint8_t *, int);
    // if return >0, av-avk is connected
    if (btapp_codec_wavein_cback(data, len))
    {
        return;
    }
#endif

    if (GET_DA_RW_GAP + len >= GET_DA_BASE_LEN)
    {
        APPL_TRACE_API0("AUDIO_Playback_Play pending data too much");
        return;
    }
    i2s_da_data_put(data, len);
}

int AUDIO_Capture_Open (void)
{
    APPL_TRACE_API0("AUDIO_Capture_Open");
#if MIC_EN
    audio_pause_AD(false);
#endif
    return 0;
}

int AUDIO_Capture_Close (void)
{
    APPL_TRACE_API0("AUDIO_Capture_Close");
#if MIC_EN
    audio_pause_AD(true);
#endif
    return 0;
}

int AUDIO_Capture_Configure (void * param)
{
    APPL_TRACE_API0("AUDIO_Capture_Configure");
    STRC_I2S_SPEC * tmp = (STRC_I2S_SPEC *)param;
    if (memcmp(&capture_param, tmp, sizeof(playback_param)))
    {
        APPL_TRACE_API0("reinit AD");
        memcpy(&capture_param, tmp, sizeof(playback_param));
#if 0
        audio_pause_AD(true);
        audio_deinit_AD();
        audio_init_AD(&capture_param);
#endif
    }
    return 0;
}

int AUDIO_Capture_Record (uint8_t * ptr, int len)
{
    int rlen = 0;
#if MIC_EN
    if (GET_AD_RW_GAP >= len)
    {
        i2s_ad_data_get(ptr, len); // get data from i2s
    }
#else
    memset(ptr, 0, len);
#endif
    return len;
}

void AUDIO_SCO_Reinit_Diff_Param (int rate, int bitsize, int channel)
{
    i2s_deinit_ADC();
    i2s_deinit_DAC();
    // channel bit etc. can be changed here;
    i2s_init_DAC(&playback_param);
    i2s_init_ADC(&capture_param);
    i2s_pause_ADC(true);
}

void AUDIO_SCO_Open (uint16_t codec_type)
{
    APPL_TRACE_API1("AUDIO_SCO_Open:%d", codec_type);
    uint32_t sample_rate = (codec_type == BTM_SCO_CODEC_MSBC ? 16000 : 8000);
#if 0
    /* Speak */
    AUDIO_Playback_SCO_2_ite_Params(codec_type, &temp);
    AUDIO_Playback_Configure(&temp);
    AUDIO_Playback_Open();
#else
    AUDIO_Playback_init_ite_Params(1, sample_rate, &ite_audio_param);
    AUDIO_Playback_Configure(&ite_audio_param);
    #if 0
    AUDIO_Playback_Open();
    audio_DA_volume_set(80);
    #endif
#endif

#if MIC_EN
    /* Microphone */
    AUDIO_Capture_SCO_2_ite_Params(codec_type, &ite_audio_param);
    AUDIO_Capture_Configure(&ite_audio_param);
    #if 0
    AUDIO_Capture_Open();
    audio_AD_volume_set(100);
    #endif
#endif
    // DA & AD must init same time?�DDA & AD sample rate must be the same value??
    AUDIO_SCO_Reinit_Diff_Param(sample_rate, 16, 1);

    // set AD & DA vol level
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    I2S_AD32_SET_RP(I2S_AD32_GET_WP());
    audio_DA_volume_set(80);
    audio_AD_volume_set(100);

    // AD & DA start
    AUDIO_Playback_Open();
    AUDIO_Capture_Open();
}

void AUDIO_SCO_Close (void)
{
    APPL_TRACE_API0("AUDIO_SCO_Close");

    AUDIO_Playback_Close();
    AUDIO_Capture_Close();
}

void AUDIO_SCO_In_Data (uint8_t * data, int len)
{
    AUDIO_Playback_Play(data, len);
}

int AUDIO_SCO_Out_Data (uint8_t * data, int len)
{
    return AUDIO_Capture_Record(data, len);
}
