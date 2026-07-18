#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#ifdef CFG_BUILD_FLOWER
    #include "flower/flower.h"
IteFlower * itu_flower = NULL;
#endif

FILE* fp = NULL;

static const char audioName[] = "ITUAudio";

static void AudioOnStop(ITUAudio* audio)
{
    // DO NOTHING
}

bool ituAudioUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    ITUAudio* audio = (ITUAudio*) widget;
    assert(audio);

    if ((widget->flags & ITU_ENABLED) == 0)
    {
        return false;
    }

    if (ev == ITU_EVENT_TIMER)
    {
        if (audio->audioFlags & ITU_AUDIO_STOPPING)
        {
            if (audio->audioFlags & ITU_AUDIO_REPEAT)
            {
                ituAudioPlay(audio);
            }
            else
            {
                ITU_LOG_DBG("audio stopped\n" );
                ituAudioOnStop(audio);
                ituExecActions(widget, audio->actions, ITU_EVENT_STOPPED, 0);
                audio->audioFlags &= ~ITU_AUDIO_PLAYING;
                ituScene->playingAudio = NULL;
            }
            audio->audioFlags &= ~ITU_AUDIO_STOPPING;
        }
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        if ((audio->audioFlags & ITU_AUDIO_PLAYING)
        #ifdef CFG_BUILD_FLOWER
            && (itu_flower->audiocase!=AudioPlayFile)
        #endif
            )
        {
            ituAudioPlay(audio);
        }
    }
    else
    {
        /* do nothing */
    }

    return false;
}

void ituAudioOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUAudio* audio = (ITUAudio*) widget;
    assert(audio);

    switch (action)
    {
        case ITU_ACTION_PLAY:
            if (widget->flags & ITU_ENABLED)
            {
                ituAudioPlay(audio);
            }
            break;

        case ITU_ACTION_STOP:
            if (widget->flags & ITU_ENABLED)
            {
                ituAudioStop(audio);
                ituAudioOnStop(audio);
                ituExecActions(widget, audio->actions, ITU_EVENT_STOPPED, 0);
            }
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

void ituAudioInit(ITUAudio* audio)
{
    assert(audio);
    ITU_ASSERT_THREAD();

    (void)memset(audio, 0, sizeof (ITUAudio));

    ituWidgetInit(&audio->widget);

    ituWidgetSetType(audio, ITU_AUDIO);
    ituWidgetSetName(audio, audioName);
    ituWidgetSetUpdate(audio, ituAudioUpdate);
    ituWidgetSetOnAction(audio, ituAudioOnAction);
    ituAudioSetOnStop(audio, AudioOnStop);
}

void ituAudioLoad(ITUAudio* audio, uint32_t base)
{
    assert(audio);

    ituWidgetLoad(&audio->widget, base);
    ituWidgetSetUpdate(audio, ituAudioUpdate);
    ituWidgetSetOnAction(audio, ituAudioOnAction);
    ituAudioSetOnStop(audio, AudioOnStop);
}

#ifdef CFG_BUILD_FLOWER
static void  AudioPlayCallback(int state,void *arg)
{
    ITU_LOG_DBG("audio play state: %d\n", state );

    if (ituScene->playingAudio)
    {
        if (state == PLAY_EOF_FILE)
        {
            ituScene->playingAudio->audioFlags |= ITU_AUDIO_STOPPING;
        }
    }
}
#endif

void ituAudioPlay(ITUAudio* audio)
{
    assert(audio);
    ITU_ASSERT_THREAD();

#ifdef CFG_BUILD_FLOWER
    if (itu_flower == NULL)
    {
        itu_flower = IteStreamInit();
    }
#endif

    if ((audio->widget.flags & ITU_ENABLED) == 0
#ifdef CFG_BUILD_FLOWER
        || (itu_flower->audiocase) == AudioPlayFile
#endif
    )
    {
        return;
    }

    ITU_LOG_DBG("playing %s\n", audio->filePath );

    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

#ifdef CFG_BUILD_FLOWER

    if (audio->filePath[0] != '\0')
    {
        char* p = strrchr(audio->filePath, '.');
        if (NULL != p)
        {
            if (stricmp(p, ".mp3") == 0 || stricmp(p, ".wma") == 0 || stricmp(p, ".wav") == 0 ||
                stricmp(p, ".aac") == 0)
            {
    #ifdef __OPENRTOS__
                if (access(audio->filePath, 0) >= 0)
                {
                    flow_start_sound_play(itu_flower, audio->filePath, Normal, AudioPlayCallback);
                    ituScene->playingAudio = audio;
                    audio->audioFlags |= ITU_AUDIO_PLAYING;
                }
                if (audio->audioFlags & ITU_AUDIO_VOLUME)
                {
                    flow_set_dac_level(audio->volume);
                }
    #endif
            }
        }
    }
#endif // CFG_BUILD_FLOWER
}

void ituAudioStop(ITUAudio* audio)
{
    assert(audio);
    ITU_ASSERT_THREAD();

    if ((audio->widget.flags & ITU_ENABLED) == 0)
    {
        return;
    }

#if defined(CFG_BUILD_FLOWER) && defined(__OPENRTOS__)
    flow_stop_sound_play(itu_flower);
#endif

    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }
    audio->audioFlags &= ~ITU_AUDIO_PLAYING;
    ituScene->playingAudio = NULL;
}
