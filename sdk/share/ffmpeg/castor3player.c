#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "castor3player.h"
#include "ite/itv.h"

#define SERR()
extern bool audioKeySoundPaused;

typedef enum VIDEO_PLAYER_STATE_TAG
{
    VIDEO_PLAYER_STATE_NOT_INITED,
    VIDEO_PLAYER_STATE_STOPPED,
    VIDEO_PLAYER_STATE_PLAYING
} VIDEO_PLAYER_STATE;

//mtal player mutex
pthread_mutex_t    mtal_mutex = PTHREAD_MUTEX_INITIALIZER;
VIDEO_PLAYER_STATE mtal_state = VIDEO_PLAYER_STATE_NOT_INITED;

static cb_handler_t gcallback;
ithMediaPlayer *media_player = &fileplayer;

/* MTAL */
// MTAL is a video player
int mtal_pb_init(cb_handler_t callback) /* open for later playback */
{
    (void)printf("MTAL# %s +\n", __func__);

    (void)pthread_mutex_lock(&mtal_mutex);
    gcallback = callback;

    switch (mtal_state)
    {
    case VIDEO_PLAYER_STATE_NOT_INITED:
        {
            /* TODO: add playback routine here */
            if ( (media_player != NULL) && (media_player->init != NULL) )
            {
                // ...
                int retv = media_player->init(callback);

                if (retv < 0)
                {
                    SERR();
                }
            }
            mtal_state = VIDEO_PLAYER_STATE_STOPPED;
        }
        break;

    default:
		; /*Do nothing*/
        break;
    }

    (void)pthread_mutex_unlock(&mtal_mutex);

    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_exit(void) /* close for termination of playback */
{
    (void)printf("MTAL# %s +\n", __func__);
    (void)pthread_mutex_lock(&mtal_mutex);

    switch (mtal_state)
    {
    case VIDEO_PLAYER_STATE_NOT_INITED:
        break;

    default:
        {
            if ( (media_player != NULL) && (media_player->deinit != NULL) )
            {
                // ...
                int retv = media_player->deinit();

                if (retv < 0)
                {
                    SERR();
                }
            }

            mtal_state = VIDEO_PLAYER_STATE_NOT_INITED;
        }
        break;
    }

    (void)pthread_mutex_unlock(&mtal_mutex);

    printf("MTAL# %s -\n", __func__);
    audioKeySoundPaused = false;

    return 0;
}

int mtal_pb_select_file(MTAL_SPEC *spec)
{
    ithMediaPlayer *new_player = NULL;
    (void)printf("MTAL# %s +\n", __func__);
    (void)pthread_mutex_lock(&mtal_mutex);

    if (spec->camera_in > NOT_CAMERA)
    {
        if(spec->camera_in == CAPTURE_IN)
        {
        	#ifdef CFG_CAPTURE_ENABLE
            new_player = &captureplayer;
			#endif
        }
    }
    else
    {
        new_player = &fileplayer;
    }


    if (new_player != media_player)
    {
    	(void)pthread_mutex_unlock(&mtal_mutex);
        mtal_pb_exit();
		(void)pthread_mutex_lock(&mtal_mutex);
        media_player = new_player;
		if ( (media_player != NULL) && (media_player->init != NULL) )
		{
            media_player->init(gcallback);
		}
		mtal_state = VIDEO_PLAYER_STATE_STOPPED;
    }

    if ( (media_player != NULL) && (media_player->select != NULL) )
    {
        media_player->select(spec->srcname, spec->vol_level);
    }

#if defined(CFG_VIDEO_ENABLE)
    itv_set_pb_mode(1);
#endif

    (void)pthread_mutex_unlock(&mtal_mutex);
    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_play(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    audioKeySoundPaused = true;
    (void)pthread_mutex_lock(&mtal_mutex);
    /* mtal routine */
#ifdef CFG_BUILD_ITV
    itv_flush_dbuf();
#endif

    /* TODO: add playback routine here */
    if ( (media_player != NULL) && (media_player->play != NULL) )
    {
        int retv;
        retv = media_player->play();
        if (retv < 0)
        {
            SERR();
        }
    }
    (void)pthread_mutex_unlock(&mtal_mutex);
    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_pause(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->pause != NULL) )
    {
        int retv;
        retv = media_player->pause();
        if (retv < 0)
        {
        	(void)pthread_mutex_unlock(&mtal_mutex);
            return -1;
    }
    }

    (void)pthread_mutex_unlock(&mtal_mutex);
    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_stop(void)
{
    int retv;
    (void)pthread_mutex_lock(&mtal_mutex);
    (void)printf("MTAL# %s +\n", __func__);
    switch (mtal_state)
    {
    case VIDEO_PLAYER_STATE_NOT_INITED:
        break;

    default:
        {
            /* TODO: add playback routine here */
            if ( (media_player != NULL) && (media_player->stop != NULL) )
            {
#if (CFG_BUILD_ITV) && ( (CFG_CHIP_FAMILY == 970) || (CFG_CHIP_FAMILY == 9860) )
                itv_stop_vidSurf_anchor();
#endif
                retv = media_player->stop();

                if (retv < 0)
                {
                    SERR();
                }
            }
            /* mtal routine */
#ifdef CFG_BUILD_ITV
            itv_flush_dbuf();
#if defined(CFG_VIDEO_ENABLE)
            itv_set_pb_mode(0);
#endif
#endif
            // -
            mtal_state = VIDEO_PLAYER_STATE_STOPPED;
        }
        break;
    }
    (void)pthread_mutex_unlock(&mtal_mutex);
    (void)printf("MTAL# %s -\n", __func__);

    audioKeySoundPaused = false;
    return 0;
}

int mtal_pb_play_videoloop(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    audioKeySoundPaused = true;

    (void)pthread_mutex_lock(&mtal_mutex);
    /* mtal routine */
#ifdef CFG_BUILD_ITV
    itv_flush_dbuf();
#endif

    /* TODO: add playback routine here */
    if ( (media_player != NULL) && (media_player->play_videoloop != NULL) )
    {
        int retv;
        retv = media_player->play_videoloop();
        if (retv < 0)
        {
            SERR();
        }
    }
    (void)pthread_mutex_unlock(&mtal_mutex);
    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_get_video_resolution_ext(int *width, int *height, char *filepath)
{
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->getVideoResolution_ext != NULL) )
    {
        int retv = media_player->getVideoResolution_ext(width, height, filepath);
        if (retv < 0)
        {
            SERR();
    }
    }
    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_get_total_duration(int *totaltime)
{
    int64_t micro_secs = 0;
#if 0
    int     hours, mins, secs;
#else
    int secs;
#endif

    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->gettotaltime != NULL) )
    {
        int retv = media_player->gettotaltime(&micro_secs);
        if (retv < 0)
        {
            (void)pthread_mutex_unlock(&mtal_mutex);
            return -1;
        }
    }
    secs       = (int)(micro_secs / 1000000);
    *totaltime = secs;
#if 0
    mins       = secs / 60;
    secs      %= 60;
    hours      = mins / 60;
    mins      %= 60;

    sprintf(totaltime, "%02d:%02d:%02d", hours, mins, secs);
#endif
    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_get_total_duration_ext(int *totaltime, char *filepath)
{
    int64_t micro_secs = 0;
    int secs;
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->gettotaltime_ext != NULL) )
    {
        int retv = media_player->gettotaltime_ext(&micro_secs, filepath);
        if (retv < 0)
        {
            SERR();
        }
    }
    secs = (int)(micro_secs / 1000000);
    *totaltime = secs;
    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_get_current_time(int *currenttime)
{
    int64_t ctimes = 0;
    int     retv;
#if 0
    int     hours, mins, secs;
#else
    int secs;
#endif

    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->getcurrenttime != NULL) )
    {
        retv = media_player->getcurrenttime(&ctimes);
        if (retv < 0)
        {
            (void)pthread_mutex_unlock(&mtal_mutex);
            return -1;
        }
    }
    secs         = (int)(ctimes / 1000000);
    *currenttime = secs;
#if 0
    mins         = secs / 60;
    secs        %= 60;
    hours        = mins / 60;
    mins        %= 60;

    sprintf(currenttime, "%02d:%02d:%02d", hours, mins, secs);
#endif
    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_seekto(int file_pos)
{
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->seekto != NULL) )
    {
        int retv = media_player->seekto(file_pos);
        if (retv < 0)
        {
            SERR();
        }
    }

    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_slow_fast_play(float speed)
{
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->slow_fast_play != NULL) )
    {
        int retv = media_player->slow_fast_play(speed);
        if (retv < 0)
        {
            SERR();
        }
    }

    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}


int mtal_pb_get_file_pos(int *file_pos)
{
    double pos = 0.0f;
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->getfilepos != NULL) )
    {
        int retv = media_player->getfilepos(&pos);
        if (retv < 0)
        {
            SERR();
        }
    }

    *file_pos = (int)pos;
    (void)pthread_mutex_unlock(&mtal_mutex);
    return 0;
}

int mtal_pb_get_audio_codec_id(char *filepath)
{
    int codec_id = 0;
    (void)pthread_mutex_lock(&mtal_mutex);
    if ( (media_player != NULL) && (media_player->gettotaltime_ext != NULL) )
    {
        media_player->getaudioCodecId(&codec_id, filepath);
    }
    (void)pthread_mutex_unlock(&mtal_mutex);
    return codec_id;
}

bool mtal_pb_check_fileplayer_playing()
{
    if ( (media_player != NULL) && (media_player->check_fileplayer_playing != NULL) )
    {
        return media_player->check_fileplayer_playing();
    }
    return false;
}

bool mtal_pb_check_fileplayer_with_audio()
{
    if ( (media_player != NULL) && (media_player->check_fileplayer_playing != NULL) && (media_player->check_fileplayer_with_audio != NULL) )
    {
        return media_player->check_fileplayer_with_audio();
    }
    return false;
}

int mtal_pb_InitAVDecodeEnv(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    if ( (media_player != NULL) && (media_player->InitAVDecodeEnv != NULL) )
    {
        media_player->InitAVDecodeEnv();
    }

    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_InitH264DecodeEnv(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    if ( (media_player != NULL) && (media_player->InitH264DecodeEnv != NULL) )
    {
        media_player->InitH264DecodeEnv();
    }

    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_InitAudioDecodeEnv(int samplerate, int num_channels, RTSPCLIENT_AUDIO_CODEC codec_id)
{
    (void)printf("MTAL# %s +\n", __func__);
    if ( (media_player != NULL) && (media_player->InitAudioDecodeEnv != NULL) )
    {
        media_player->InitAudioDecodeEnv(samplerate, num_channels, codec_id);
    }

    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_h264_decode_from_rtsp(unsigned char *buf, int size, double timestamp)
{
    if ( (media_player != NULL) && (media_player->h264_decode_from_rtsp != NULL) )
    {
        int retv = media_player->h264_decode_from_rtsp(buf, size, timestamp);
        if (retv < 0)
        {
            SERR();
        }
    }
    return 0;
}

int mtal_pb_audio_decode_from_rtsp(unsigned char *buf, int size, double timestamp)
{
    if ( (media_player != NULL) && (media_player->audio_decode_from_rtsp != NULL) )
    {
        int retv = media_player->audio_decode_from_rtsp(buf, size, timestamp);
        if (retv < 0)
        {
            SERR();
        }
    }
    return 0;
}

int mtal_pb_DeinitAVDecodeEnv(void)
{
    (void)printf("MTAL# %s +\n", __func__);
    if ( (media_player != NULL) && (media_player->DeinitAVDecodeEnv != NULL) )
    {
        media_player->DeinitAVDecodeEnv();
    }

    (void)printf("MTAL# %s -\n", __func__);
    return 0;
}

int mtal_pb_setVideoDisplayResolution(int width, int height)
{
	if ( (media_player != NULL) && (media_player->setVideoDisplayResolution != NULL) )
    {
        media_player->setVideoDisplayResolution(width, height);
    }
	return 0;
}

/* for live streaming */
int mtal_drop_all_input_streams(void)
{
    return media_player->drop_all_input_streams();
}

