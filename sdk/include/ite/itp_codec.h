/* sy.chuang, 2012-0702, ITE Tech. */

#ifndef ITP_CODEC_H
#define ITP_CODEC_H

#ifdef __cplusplus
extern "C"
{
#endif

/* === common DAC/ADC ops === */
void    itp_codec_wake_up (void);
void    itp_codec_standby (void);

/* DAC */
void    itp_codec_playback_init (uint32_t output);
void    itp_codec_playback_deinit (void);

void    itp_codec_playback_amp_volume_down (void);
void    itp_codec_playback_amp_volume_up (void);

void    itp_codec_playback_set_direct_vol (uint32_t target_vol);
void    itp_codec_playback_set_direct_volperc (uint32_t target_volperc);
void    itp_codec_playback_set_direct_RLvolperc (uint32_t target_volperc, uint8_t RL);
void    itp_codec_playback_get_RLcurrvol (uint32_t * currvolperc, uint8_t RL);
void    itp_codec_playback_get_currvol (uint32_t * currvol);
void    itp_codec_playback_get_currvolperc (uint32_t * currvolperc);
void    itp_codec_playback_get_vol_range (uint32_t * max, uint32_t * regular_0db, uint32_t * min);

void    itp_codec_playback_mute (void);
void    itp_codec_playback_unmute (void);

void    itp_codec_playback_linein_bypass (uint32_t bypass);

/* ADC */
void    itp_codec_rec_init (uint32_t input_source);
void    itp_codec_rec_deinit (void);

void    itp_codec_rec_set_direct_vol (uint32_t target_vol);
void    itp_codec_rec_set_direct_volperc (uint32_t target_micperc);
void    itp_codec_rec_get_currvol (uint32_t * currvol);
void    itp_codec_rec_set_direct_RLvolperc (uint32_t target_micperc, uint8_t RL);
void    itp_codec_rec_get_RLcurrvol (uint32_t * currvol, uint8_t RL);
void    itp_codec_rec_get_vol_range (uint32_t * max, uint32_t * regular_0db, uint32_t * min);

void    itp_codec_rec_mute (void);
void    itp_codec_rec_unmute (void);

/* Sample Rate */
void    itp_codec_get_i2s_sample_rate (uint32_t * samplerate);
void    itp_codec_set_i2s_sample_rate (uint32_t samplerate);

bool itp_codec_get_DA_running (void);
bool itp_codec_get_AD_running (void);

#ifdef __cplusplus
}
#endif

#endif // ITP_CODEC_H
