#include <sys/errno.h>
#include <string.h>
#include <lvgl.h>
#include "tslib.h"
#include "ite/ith.h"
#include "i2s/i2s.h"
#ifdef __OPENRTOS__
    #include "ite/audio.h"
    #include "lvgl_ite_itaudio.h"
#endif

lv_iteaudio_t play[1];
bool          init = false;

/*init audio play*/
int           lv_init_ite_itaudio (int id)
{
    lv_iteaudio_t * p = &play[id];
    if (p->flower != NULL)
    {
        return 0;
    }

    memset(p, 0, sizeof(lv_iteaudio_t));
    p->flower  = IteStreamInit();
    p->ID      = id;
    p->level   = 30;
    p->pstatus = FILTER_NONE;
    p->mode    = Normal;
    if (!init)
    {
        flow_init_DA();
        flow_set_dac_level(30);
    }
    return 0;
}

void lv_uninit_ite_itaudio (int id)
{
    lv_iteaudio_t * p = &play[id];
    if (p->flower)
    {
        IteStreamUninit(p->flower);
    }
    memset(p, 0, sizeof(lv_iteaudio_t));
}

/****************************************/
/*reset audio player*/
void lv_ite_audio_play_reset (int id)
{
    lv_iteaudio_t * p = &play[id];
    IteFlower *     f = p->flower;
    flow_stop_sound_play(f);
    p->start_cb  = NULL;
    p->stop_cb   = NULL;
    p->eof_cb    = NULL;
    p->pause_cb  = NULL;
    p->resume_cb = NULL;
    p->pstatus   = FILTER_NONE;
    memset(&p->playpath[0], 0, 256);
}

/*get play */
lv_iteaudio_t * lv_ite_audio_play_get (int id)
{
    lv_iteaudio_t * p = &play[id];
    return p;
}

/*set file path*/
void lv_ite_audio_set_playpath (int id, const char * text)
{
    lv_iteaudio_t * p = &play[id];
    strcpy(&p->playpath[0], text);
}

/******************************/
/*HW DAC level set*/
void lv_ite_audio_set_level (int id, int level)
{
    lv_iteaudio_t * p = &play[id];
    p->level          = level;
}

/*****************************/
/*start set callback func*/
void lv_ite_audio_set_startcb (int id, cb_sound_t cb)
{
    lv_iteaudio_t * p = &play[id];
    p->start_cb       = cb;
}

void lv_ite_audio_set_stopcb (int id, cb_sound_t cb)
{
    lv_iteaudio_t * p = &play[id];
    p->stop_cb        = cb;
}

void lv_ite_audio_set_eofcb (int id, cb_sound_t cb)
{
    lv_iteaudio_t * p = &play[id];
    p->eof_cb         = cb;
}

void lv_ite_audio_set_pausecb (int id, cb_sound_t cb)
{
    lv_iteaudio_t * p = &play[id];
    p->pause_cb       = cb;
}

void lv_ite_audio_set_resumecb (int ID, cb_sound_t cb)
{
    lv_iteaudio_t * p = &play[ID];
    p->resume_cb      = cb;
}

/******************************/
/*play start/stop*/
int lv_ite_audio_start (int id)
{
    lv_iteaudio_t * p = &play[id];
    IteFlower *     f = p->flower;
    flow_set_dac_level(p->level);
    flow_start_sound_play(f, p->playpath, Normal, p->eof_cb);
    if (play[id].flower->audiocase == AudioIdle)
    {
        return 0;
    }

    if (p->start_cb)
    {
        p->start_cb(0, (void *)p);
    }
    return 1;
}

void lv_ite_audio_stop (int id)
{
    lv_iteaudio_t * p = &play[id];
    IteFlower *     f = p->flower;
    flow_stop_sound_play(f);

    p->pstatus = FILTER_NONE;
    if (p->stop_cb)
    {
        p->stop_cb(0, (void *)p);
    }
}

/*player pause/resume*/
void lv_ite_audio_pause (int id)
{
    lv_iteaudio_t * p = &play[id];
    IteFlower *     f = p->flower;
    Flow_status_set(f, FILTER_PAUSE);
    i2s_pause_DAC(true);

    p->pstatus = FILTER_PAUSE;
    if (p->pause_cb)
    {
        p->pause_cb(FILTER_PAUSE, (void *)p);
    }
}

void lv_ite_audio_resume (int id)
{
    lv_iteaudio_t * p = &play[id];
    IteFlower *     f = p->flower;
    Flow_status_set(f, FILTER_RESUME);
    p->pstatus = FILTER_RESUME;
    i2s_pause_DAC(false);

    p->pstatus = FILTER_RESUME;
    if (p->resume_cb)
    {
        p->resume_cb(FILTER_RESUME, (void *)p);
    }
}

int lv_ite_audio_get_total_time (char * filename)
{

    DataInfo di;
    int      du = audioGetTotalTime(filename, &di); // sec

    return du;
}
/******************************/
