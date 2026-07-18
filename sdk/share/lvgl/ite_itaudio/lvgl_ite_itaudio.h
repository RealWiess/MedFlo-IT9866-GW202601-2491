#ifndef LVGL_ITE_ITAUDIO_H_
#define LVGL_ITE_ITAUDIO_H_

#include <lvgl.h>
#include "ite/audio.h"
#include "flower/flower.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _lv_iteaudio_t {
    IteFlower *flower;
    int ID;
    int level;
    PlaySoundCase mode;
    IteFilterStatus pstatus;
    char playpath[256];
    cb_sound_t start_cb;
    cb_sound_t stop_cb;
    cb_sound_t eof_cb;
    cb_sound_t pause_cb;
    cb_sound_t resume_cb;
}lv_iteaudio_t;


int lv_init_ite_itaudio(int id);
void lv_uninit_ite_itaudio(int id);
void lv_ite_audio_play_reset(int id);
lv_iteaudio_t *lv_ite_audio_play_get(int id);
void lv_ite_audio_set_playpath(int id,const char * text);
void lv_ite_audio_set_level(int id,int level);
void lv_ite_audio_set_startcb(int id,cb_sound_t cb);
void lv_ite_audio_set_stopcb(int id,cb_sound_t cb);
void lv_ite_audio_set_eofcb(int id,cb_sound_t cb);
void lv_ite_audio_set_pausecb(int id,cb_sound_t cb);
void lv_ite_audio_set_resumecb(int id,cb_sound_t cb);
int lv_ite_audio_start(int id);
void lv_ite_audio_stop(int id);
void lv_ite_audio_pause(int id);
void lv_ite_audio_resume(int id);
int lv_ite_audio_get_total_time(char* filename);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_ITE_ITAUDIO_H_ */