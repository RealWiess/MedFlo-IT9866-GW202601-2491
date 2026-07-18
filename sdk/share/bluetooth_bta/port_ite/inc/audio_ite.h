
#ifndef _ADUDIO_ITE_H_
#define _ADUDIO_ITE_H_

#include "i2s/i2s.h" 

#define AUDIO_DA_VOLUME_MAX 100

UINT32 ucodec_get_samplerate_value(tUCODEC_SBC_SMP_FREQ sample_rate);
void AUDIO_Ucodec_2_ite_Params(tUCODEC_CNF_SBC *sbc, STRC_I2S_SPEC *ite_param);

int AUDIO_Playback_Open(void);
int AUDIO_Playback_Close(void);
//int AUDIO_Playback_Configure(struct AUDIO_PARAMS *param);
int AUDIO_Playback_Configure(STRC_I2S_SPEC *param);
void AUDIO_Playback_Play(uint8_t *data, int len);

int AUDIO_Capture_Open(void );
int AUDIO_Capture_Close(void);
//int AUDIO_Capture_Configure(struct AUDIO_PARAMS *param);
int AUDIO_Capture_Configure(void *param);
int AUDIO_Capture_Record(uint8_t *ptr, int len);

void AUDIO_SCO_Open(uint16_t codec_type);
void AUDIO_SCO_Close(void);
void AUDIO_SCO_In_Data(uint8_t *data, int len);
int AUDIO_SCO_Out_Data(uint8_t *data, int len);
void AUDIO_Playback_init_ite_Params(uint8_t ch , uint32_t sample_rate , STRC_I2S_SPEC * param);


#endif


