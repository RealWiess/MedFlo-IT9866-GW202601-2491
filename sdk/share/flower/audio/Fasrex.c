#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

int  ASR_Init (void);
void ASR_Reset (void);
int  ASR_Recog (short * buf, int buf_len, const char ** text, float * score);
void ASR_Release ();
//=============================================================================
//                              Constant Definition
//=============================================================================
#define MEASUREMENTS
#ifdef MEASUREMENTS
uint32_t itpGetTickCount (void);
uint32_t itpGetTickDuration (uint32_t tick);
#endif

typedef struct ASRstate
{
    IteFilter * msF;
    rbuf_ite *  Buf;
    int         framesize;
    cb_sound_t  fn_cb;
    bool        isbypass;

} ASRstate;

static void asr_process (IteFilter * f)
{
    ASRstate *  s          = (ASRstate *)f->data;

    IteQueueblk blk        = {0};
    int         flushcount = 25;
#ifdef MEASUREMENTS
    uint32_t start_cnt = 0;
    uint32_t count     = 1;
    uint64_t diff      = 0;
#endif
    int          nbytes = s->framesize;
    float        score;
    int          rs;
    const char * text;
    uint16_t     PcmBuf1[480] = {0};
    int          err1         = 0;

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om  = blk.datap;
            int        ret = 0;

            ret            = ite_rbuf_put(s->Buf, om->b_rptr, om->size);
            if (om)
            {
                freemsg_ite(om);
            }

            if (flushcount++ < 25)
            {
                ite_rbuf_flush(s->Buf); // flush 25*20ms data to avoid tipsound
                                        // match ASR again
            }

            while (ite_rbuf_get((char *)PcmBuf1, s->Buf, nbytes * 2))
            {
#ifdef MEASUREMENTS
                start_cnt = itpGetTickCount();
#endif
                rs = ASR_Recog((short *)PcmBuf1, nbytes, &text, &score);
#ifdef MEASUREMENTS
                count++;
                diff += itpGetTickDuration(start_cnt);
                if (count % 200 == 0)
                {
                    printf("ASR 200 time %d bufsize=%d\n", (int)diff,
                           ite_rbuf_get_avail_size(s->Buf));
                    diff = 0;
                }
#endif
                if (rs == 1)
                {
                    asrStruct data;
                    data.rs    = rs;
                    data.text  = (char *)text;
                    data.index = -1;
                    data.score = score;
                    if (s->fn_cb)
                    {
                        s->fn_cb(Asrevent, (void *)&data);
                    }
                    printf("ASR Result: %s\n", text);
                    flushcount = 0;
                }
            }
        }
        usleep(5000);
    }
}

static void asr_init (IteFilter * f)
{
    ASRstate * s = (ASRstate *)ite_new(ASRstate, 1);
    if (s != NULL)
    {
        f->data      = s;
        s->framesize = 480;
        s->Buf       = ite_rbuf_init(s->framesize * 12);
        s->fn_cb     = NULL;
        s->isbypass  = false;
        s->msF       = f;
        ASR_Init();
        printf("asr_init\n");
    }
    else
    {
        f->data = NULL;
    }
}

static void asr_uninit (IteFilter * f)
{
    ASRstate * s = (ASRstate *)f->data;
    if (s != NULL)
    {
        ASR_Release();
        ite_rbuf_free(s->Buf);
        free(s);
    }
}

static void asr_set_cb (IteFilter * f, void * arg)
{
    ASRstate * s = (ASRstate *)f->data;
    s->fn_cb     = (cb_sound_t)arg;

    if (!s->isbypass)
    {
        Castor3snd_reinit_for_diff_rate(16000, 16, 1);
    }
}

static void asr_set_bypass (IteFilter * f, void * arg)
{
    ASRstate * s = (ASRstate *)f->data;
    s->isbypass  = true;
}

static IteMethodDes asr_methods[] = {
    {    ITE_FILTER_SET_CB,     asr_set_cb},
    {ITE_FILTER_SET_BYPASS, asr_set_bypass},
    {                    0,           NULL}
};

// clang-format off
IteFilterDes FilterAsr = {
    ITE_FILTER_ASR_ID,
    asr_init,
    asr_uninit,
    asr_process,
    asr_methods
};
// clang-format on
