#include <stdio.h>
#include "audio/include/fileheader.h"
#include "flower/flower.h"
#include "ite/audio.h"
#include "i2s/i2s.h"
#include "castor3player.h"
#include "ite/itv.h"

bool              is_file_eof = false;
extern gQueueData gQ;

typedef struct _FfmpegData
{
    PlayerState state;
    MTAL_SPEC   mtal_spec;
    bool        eof;
    void *      tmp;
    int         framesize;
} FfmpegData;

//=============================================================//

static void EventHandler (PLAYER_EVENT nEventID, void * arg)
{
    switch (nEventID)
    {
        case PLAYER_EVENT_EOF:
            printf("PLAYER_EVENT_EOF ...\n");
            is_file_eof = true;
            break;

        default:
            is_file_eof = true;
            break;
    }
}

static void ffmpeggrab_init (IteFilter * f)
{
    FfmpegData * d = (FfmpegData *)ite_new(FfmpegData, 1);
    if (d != NULL)
    {
        memset(&d->mtal_spec, 0, sizeof(MTAL_SPEC));
#ifdef CFG_BUILD_ITV
        itv_init();
#endif
        d->framesize = 320;
        d->eof       = false;
        is_file_eof  = false;
        mtal_pb_init(EventHandler);
        if (!gQ.startcount)
        {
            streamQInit(ITE_PCM_CODEC);
        }
    }
    f->data = d;
}

static void ffmpeggrab_uninit (IteFilter * f)
{
    FfmpegData * d = (FfmpegData *)f->data;
    if (d != NULL)
    {
        mtal_pb_stop();
        mtal_pb_exit();
        is_file_eof = false;
        free(d);
        // gQ.startcount=0;
        streamQUninit();
#ifdef CFG_BUILD_ITV
        itv_deinit();
#endif
    }
}

static void ffmpeggrab_process (IteFilter * f)
{
    FfmpegData * d      = (FfmpegData *)f->data;
    IteQueueblk  blk    = {0};
    IteFlower *  flower = (IteFlower *)f->srcFlow;

    // while(streamQCount()<10 && !flower->dInfo.init && f->run){
    //     sleep(1);
    // }
    // d->framesize=flower->dInfo.bytes20ms;
    d->state            = Playing;
    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            sleep(1);
            streamQflush();
            continue;
        }
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }
        // iteAudioUpdateMessageQ();

        if (d->state == Playing)
        {
            mblk_ite *  im  = streamQGet(); // get data from Q
            IteQueueblk blk = {0};
            if (im)
            {
                if (im->size == 0)
                {
                    if (im)
                    {
                        freemsg_ite(im);
                    }
                    return;
                }
                blk.datap = im;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
            if (is_file_eof && IteAudioQueueIsEmpty(f, 0))
            {
                printf("play eof\n");
                d->state = Eof;
            }
        }

        if (d->state == Eof)
        {
            blk.datap    = allocb0_ite(flower->dInfo.bytes20ms);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
    streamQflush();
}

static void ffmpeggrab_open (IteFilter * f, void * arg)
{
    FfmpegData * d    = (FfmpegData *)f->data;
    const char * file = (const char *)arg;

#if CFG_BUILD_FFMPEG
    gfSetInitLinkToFFmpeg((IteFlower **)&f->srcFlow);
#endif
    strlcpy(d->mtal_spec.srcname, file, sizeof(d->mtal_spec.srcname));
    d->mtal_spec.vol_level = i2s_get_current_volperc();
    mtal_pb_stop();
    mtal_pb_select_file(&d->mtal_spec);
    mtal_pb_play();
}

static IteMethodDes ffmpeggrab_methods[] = {
    {ITE_FILTER_FILEOPEN, ffmpeggrab_open},
    {                  0,            NULL}
};

// clang-format off
IteFilterDes FilterFfmpegGrab = {
    ITE_FILTER_FFMPEGGRAB_ID,
    ffmpeggrab_init,
    ffmpeggrab_uninit,
    ffmpeggrab_process,
    ffmpeggrab_methods
};
// clang-format on
