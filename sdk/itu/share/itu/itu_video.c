#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#ifdef CFG_BUILD_AUDIO_MGR
    #include "audio_mgr.h"
#endif

#ifdef CFG_VIDEO_ENABLE
    #include "castor3player.h"
    #include "ite/itv.h"
    #include "ith/ith_video.h"
#endif // CFG_VIDEO_ENABLE

bool              isOtherVideoPlaying = false;

static pthread_t    tid;
static const char   videoName[]               = "ITUVideo";
static bool         videoPlayerIsFileEOF      = false;
static bool         videoloopIsPlaying        = false;
static bool         isVideoWidgetStartPlaying = false;
#ifdef CFG_VIDEO_ENABLE
static bool      videoPlayerSetWindow = false;
static MTAL_SPEC mtal_spec            = {0};
// extern bool audioKeySoundPaused;
#endif

#ifdef CFG_VIDEO_ENABLE
static void ItuEnterVideoState (ITUWidget* widget)
{
    ITUVideo* video = (ITUVideo*)widget;
    #ifndef DISABLE_SWITCH_VIDEO_STATE
    ituFrameFuncInit();
    if (video->surfTmp == NULL)
    {
        video->surfTmp = ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, 0, 0);
    }
    #endif
}

static void ItuLeaveVideoState (ITUWidget* widget)
{
    ITUVideo* video = (ITUVideo*)widget;
    #ifndef DISABLE_SWITCH_VIDEO_STATE
    ituFrameFuncExit();
    if (video->surfTmp != NULL)
    {
        ituDestroySurface(video->surfTmp);
        video->surfTmp = NULL;
    }
        #ifdef CFG_LCD_ENABLE
    ituLcdInit();
        #endif
        #ifdef CFG_M2D_ENABLE
    ituM2dInit();
        #else
    ituSWInit();
        #endif
    #endif
}

static void EventHandler (PLAYER_EVENT nEventID, void * arg)
{
    switch (nEventID)
    {
        case PLAYER_EVENT_EOF:
            (void)printf("File EOF\n");
            videoPlayerIsFileEOF = true;
            break;
        case PLAYER_EVENT_OPEN_FILE_FAIL:
            (void)printf("Open file fail\n");
            videoPlayerIsFileEOF = true;
            break;
        case PLAYER_EVENT_UNSUPPORT_FILE:
            (void)printf("File not support\n");
            videoPlayerIsFileEOF = true;
            break;
        default:
            break;
    }
}

static void * polling_video_widget_status (void * arg)
{
    ITUVideo * video = (ITUVideo *)arg;
    while (videoloopIsPlaying)
    {
        if (videoPlayerIsFileEOF)
        {
            videoPlayerIsFileEOF = false;
            mtal_pb_seekto(0);
        }
        (void)usleep(2000);
    }
    pthread_exit(NULL);
}
#endif
static void VideoOnStop (ITUVideo * video)
{
    // DO NOTHING
}

void ituVideoExit (ITUWidget * widget)
{
    ITUVideo * video = (ITUVideo *)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

#ifdef CFG_VIDEO_ENABLE
    mtal_pb_stop();
    mtal_pb_exit();
    videoloopIsPlaying = false;
    ItuLeaveVideoState(widget);
#endif // CFG_VIDEO_ENABLE

    ituVideoOnStop(video);

    ituWidgetExitImpl(widget);
}

bool ituVideoUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    ITUVideo * video  = (ITUVideo *)widget;
    bool       result = false;
    assert(video);

    if (ev == ITU_EVENT_TIMER)
    {
        if (!isOtherVideoPlaying && !isVideoWidgetStartPlaying)
        {
#ifdef CFG_VIDEO_ENABLE
            if (video->playing && video->filePath[0] != '\0')
            {
                ItuEnterVideoState(widget);
                videoPlayerSetWindow = true;
    #ifdef CFG_VIDEO_ENABLE
                itv_set_video_window(0, 0, ithLcdGetWidth(), ithLcdGetHeight());
    #endif
                mtal_pb_init(EventHandler);
                strlcpy(mtal_spec.srcname, video->filePath, sizeof(mtal_spec.srcname));
                if (video->volume)
                {
                    mtal_spec.vol_level = video->volume;
                }
                mtal_pb_select_file(&mtal_spec);
                if (video->repeat)
                {
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                    mtal_pb_play_videoloop();
                    videoloopIsPlaying = true;
                    pthread_create(&tid, &attr, polling_video_widget_status, (void *)video);
                }
                else
                {
                    mtal_pb_play();
                }
                isVideoWidgetStartPlaying = true;
            }
#endif
        }

        if (video->playing)
        {
            if (videoPlayerIsFileEOF)
            {
                if (!video->repeat)
                {
                    videoPlayerIsFileEOF = false;
#ifdef CFG_VIDEO_ENABLE
                    mtal_pb_stop();
#endif
                    // video->playing = false;
                    ituVideoOnStop(video);
                }
            }
            result = widget->dirty = true;
        }
    }
    else if (ev == ITU_EVENT_LOAD)
    {
        isVideoWidgetStartPlaying = false;
        if (!isOtherVideoPlaying)
        {
#ifdef CFG_VIDEO_ENABLE
            if (video->playing && video->filePath[0] != '\0')
            {
                ItuEnterVideoState(widget);
                videoPlayerSetWindow = true;
    #ifdef CFG_VIDEO_ENABLE
                itv_set_video_window(0, 0, ithLcdGetWidth(), ithLcdGetHeight());
    #endif
                mtal_pb_init(EventHandler);
                strlcpy(mtal_spec.srcname, video->filePath, sizeof(mtal_spec.srcname));
                if (video->volume)
                {
                    mtal_spec.vol_level = video->volume;
                }
                mtal_pb_select_file(&mtal_spec);
                if (video->repeat)
                {
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                    mtal_pb_play_videoloop();
                    videoloopIsPlaying = true;
                    pthread_create(&tid, &attr, polling_video_widget_status, (void *)video);
                }
                else
                {
                    mtal_pb_play();
                }
                isVideoWidgetStartPlaying = true;
            }
#endif
        }
    }
    else if (ev == ITU_EVENT_RELEASE)
    {
#ifdef CFG_VIDEO_ENABLE
        mtal_pb_stop();
        mtal_pb_exit();
        videoloopIsPlaying = false;
        // audioKeySoundPaused = false;
        ItuLeaveVideoState(widget);
#endif
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
#ifdef CFG_VIDEO_ENABLE
        videoPlayerSetWindow = true;
#endif
        result = widget->dirty = true;
    }
    else if (ev == ITU_EVENT_MOUSEMOVE)
    {
#ifdef CFG_VIDEO_ENABLE
        videoPlayerSetWindow = true;
        result = widget->dirty = true;
#endif
    }
    else
    {
        /* do nothing */
    }
    result |= ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);
    return widget->visible ? result : false;
}

void ituVideoDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int            destx, desty;
    ITURectangle   prevClip;
    ITUVideo *     video = (ITUVideo *)widget;
    ITURectangle * rect  = (ITURectangle *)&widget->rect;
    assert(video);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
#ifdef CFG_VIDEO_ENABLE
    if (videoPlayerSetWindow)
    {
        itv_set_video_window(destx, desty, rect->width, rect->height);
        videoPlayerSetWindow = false;
    }
#endif
    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860 || CFG_CHIP_FAMILY == 9830 || CFG_CHIP_FAMILY == 9870 || CFG_CHIP_FAMILY == 9880) && (CFG_VIDEO_ENABLE)
    if (video->surfTmp != NULL)
    {
        ituDrawVideoSurface_ex(dest, destx, desty, rect->width, rect->height, video->angle, video->surfTmp);
    }
    else
    {
        ituDrawVideoSurface_ex(dest, destx, desty, rect->width, rect->height, 0, NULL);
    }
#else
    ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
#endif

    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}

void ituVideoOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    assert(widget);

    switch (action)
    {
        case ITU_ACTION_PLAY:
            if (param[0] != '\0')
            {
                ituVideoPlay((ITUVideo *)widget, atoi(param));
            }
            break;

        case ITU_ACTION_STOP:
            ituVideoStop((ITUVideo *)widget);
            break;

        case ITU_ACTION_GOTO:
            if (param[0] != '\0')
            {
                ituVideoGoto((ITUVideo *)widget, atoi(param));
            }
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

void ituVideoInit (ITUVideo * video)
{
    assert(video);
    ITU_ASSERT_THREAD();

    (void)memset(video, 0, sizeof(ITUVideo));

    ituWidgetInit(&video->widget);

    ituWidgetSetType(video, ITU_VIDEO);
    ituWidgetSetName(video, videoName);
    ituWidgetSetExit(video, ituVideoExit);
    ituWidgetSetUpdate(video, ituVideoUpdate);
    ituWidgetSetDraw(video, ituVideoDraw);
    ituWidgetSetOnAction(video, ituVideoOnAction);
    ituVideoSetOnStop(video, VideoOnStop);
}

void ituVideoLoad (ITUVideo * video, uint32_t base)
{
    assert(video);

    ituWidgetLoad((ITUWidget *)video, base);

    ituWidgetSetExit(video, ituVideoExit);
    ituWidgetSetUpdate(video, ituVideoUpdate);
    ituWidgetSetDraw(video, ituVideoDraw);
    ituWidgetSetOnAction(video, ituVideoOnAction);
    ituVideoSetOnStop(video, VideoOnStop);
}

void ituVideoPlay (ITUVideo * video, int sec)
{
    assert(video);
    ITU_ASSERT_THREAD();

#ifdef CFG_VIDEO_ENABLE
    if (video->filePath[0] != '\0')
    {
        if (!video->playing)
        {
            // audioKeySoundPaused = true;
    #ifdef CFG_BUILD_AUDIO_MGR
            smtkAudioMgrQuickStop();
    #endif
            ItuEnterVideoState((ITUWidget *)video);
            videoPlayerSetWindow = true;

            itv_set_video_window(0, 0, ithLcdGetWidth(), ithLcdGetHeight());

            mtal_pb_init(EventHandler);
            strlcpy(mtal_spec.srcname, video->filePath, sizeof(mtal_spec.srcname));
            if (video->volume)
            {
                mtal_spec.vol_level = video->volume;
            }
            mtal_pb_select_file(&mtal_spec);
            if (video->repeat)
            {
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                mtal_pb_play_videoloop();
                videoloopIsPlaying = true;
                pthread_create(&tid, &attr, polling_video_widget_status, (void *)video);
            }
            else
            {
                mtal_pb_play();
            }
            isVideoWidgetStartPlaying = true;
        }
        else if (video->paused)
        {
            mtal_pb_pause();
            video->paused = false;
        }
        else
        {
            /* do nothing */
        }
    }

    if (sec > 0)
    {
        mtal_pb_seekto(sec);
    }
#endif // CFG_VIDEO_ENABLE

    video->playing      = true;
    video->widget.dirty = true;
}

void ituVideoStop (ITUVideo * video)
{
    assert(video);
    ITU_ASSERT_THREAD();

    if (video->playing)
    {
#ifdef CFG_VIDEO_ENABLE
        ITUWidget* widget = (ITUWidget*)video;
        mtal_pb_stop();
        mtal_pb_exit();
        videoloopIsPlaying = false;
        ItuLeaveVideoState(widget);

        // audioKeySoundPaused = false;
#endif // CFG_VIDEO_ENABLE
        video->playing      = false;
        video->paused       = false;
        video->widget.dirty = true;
    }
}

void ituVideoPause (ITUVideo * video)
{
    assert(video);
    ITU_ASSERT_THREAD();

    if (video->playing && !video->paused)
    {
#ifdef CFG_VIDEO_ENABLE
        mtal_pb_pause();
#endif // CFG_VIDEO_ENABLE
        video->paused = true;
    }
}

void ituVideoGoto (ITUVideo * video, int percentage)
{
    assert(video);
    ITU_ASSERT_THREAD();

    if (video->playing && percentage >= 0)
    {
#ifdef CFG_VIDEO_ENABLE
        int pos;
        int totaltime = 0;

        mtal_pb_get_total_duration(&totaltime);
        if (totaltime > 0)
        {
            pos = percentage * totaltime / 100;
        }
        else
        {
            pos = 0;
        }
        mtal_pb_seekto(pos);
#endif // CFG_VIDEO_ENABLE

        video->widget.dirty = true;
    }
}

int ituVideoGetPlayingPercentage (ITUVideo * video)
{
    assert(video);
    ITU_ASSERT_THREAD();

    if (video->playing)
    {
        int percentage_value;
        int totaltime   = 0;
        int currenttime = 0;

#ifdef CFG_VIDEO_ENABLE
        int ret = mtal_pb_get_total_duration(&totaltime);
        if (ret < 0)
        {
            return -1;
        }
        ret = mtal_pb_get_current_time(&currenttime);
        if (ret < 0)
        {
            return -1;
        }
#endif
        if (totaltime > 0)
        {
            percentage_value = currenttime * 100 / totaltime;
            if (percentage_value > 100)
            {
                percentage_value = 100;
            }
        }
        else
        {
            percentage_value = 0;
        }

        return percentage_value;
    }
    return -1;
}

void ituVideoSpeedUpDown (ITUVideo * video, float speed)
{
    assert(video);
    ITU_ASSERT_THREAD();

    if (video->playing)
    {
#ifdef CFG_VIDEO_ENABLE
        mtal_pb_slow_fast_play(speed);
#endif // CFG_VIDEO_ENABLE
    }
}

void ituVideoSetRotateAngle(ITUVideo* video, int angle)
{
    assert(video);
    ITU_ASSERT_THREAD();
    
    video->angle = angle;
}

