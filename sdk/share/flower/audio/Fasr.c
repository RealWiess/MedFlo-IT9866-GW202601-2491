#include "chains/models/words.h"
#include "flower/flower.h"
#include "include/audioqueue.h"
#include <stdio.h>

extern const char * words_str[];
//=============================================================================
//                              Constant Definition
//=============================================================================
#define MEASUREMENTS
#ifdef MEASUREMENTS
uint32_t itpGetTickCount (void);
uint32_t itpGetTickDuration (uint32_t tick);
#endif
// #define DUMP_DATA
//=============================================================================
//                               Private Function Declaration
//=============================================================================
typedef struct Fvad            Fvad;
typedef struct C_Compound_Type C_Compound_Type;
extern C_Compound_Type *       ITE_WFST_Init (Fvad ** vs);
extern int  ITE_WFST_Decoder (Fvad ** vs, short * in, C_Compound_Type * asr);
extern void ITE_WFST_Destroy (Fvad ** vs, C_Compound_Type * asr);
extern C_Compound_Type * nnet_init ();
extern void              InitAsrModels ();
void                     nnet (C_Compound_Type * net);
void                     AsrReset ();
int                      AsrRead (short * const data);
Fvad *                   vs;

#ifdef DUMP_DATA
    #include "ite/itp.h"
static char * Get_Storage_path (void)
{
    ITPDriveStatus * driveStatusTable;
    ITPDriveStatus * driveStatus = NULL;
    int              i;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        driveStatus = &driveStatusTable[i];
        if (driveStatus->disk >= ITP_DISK_MSC00 &&
            driveStatus->disk <= ITP_DISK_MSC17)
        {
            if (driveStatus->avail)
            {
                printf("USB #%d inserted: %s\n",
                       driveStatus->disk - ITP_DISK_MSC00,
                       driveStatus->name[0]);
                return driveStatus->name[0];
            }
        }
    }
    return NULL;
}
#endif

typedef struct ASRstate
{
    IteFilter *       msF;
    int               framesize;
    cb_sound_t        fn_cb;
    bool              isbypass;
    C_Compound_Type * asr;
    rbuf_ite *        Buf;
#ifdef DUMP_DATA
    mblkq dump_q;
#endif
} ASRstate;

static void asr_process (IteFilter * f)
{
    ASRstate *  s            = (ASRstate *)f->data;
    IteQueueblk blk          = {0};
    char        PcmBuf1[320] = {0};              // 16000*10/1000*2 (10ms)
    int         nbytes       = s->framesize * 2; // 160*2
    int         rs;
    int         flushcount = 10;
#ifdef MEASUREMENTS
    uint32_t start_cnt = 0;
    uint32_t count     = 1;
    uint64_t diff      = 0;
#endif

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om  = blk.datap;
            int        ret = 0;

            ret            = ite_rbuf_put(s->Buf, om->b_rptr, om->size);
#ifdef DUMP_DATA
            putmblkq(&s->dump_q, dupmblk(om));
#else
            if (flushcount++ < 65)
            {
                ite_rbuf_flush(s->Buf); // flush 25*20ms data to avoid tipsound
                                        // match ASR again
            }
#endif
            if (om)
            {
                freemsg_ite(om);
            }
            while (ite_rbuf_get(PcmBuf1, s->Buf, nbytes))
            {
#ifdef MEASUREMENTS
                start_cnt = itpGetTickCount();
#endif
                // ITE_WFST_Decoder(&vs, m->b_rptr, s->asr);
                rs = AsrRead((short * const)PcmBuf1);
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
                if (rs > 0)
                {
                    asrStruct data;
                    data.rs    = rs;
                    data.text  = (char *)words_str[rs - 1];
                    data.index = -1;
                    // data.score=score;
                    if (s->fn_cb)
                    {
                        s->fn_cb(Asrevent, (void *)&data);
                    }
                    printf("ASR Result: %s\n", data.text);
#ifdef DUMP_DATA
                    {
                        mblk_ite * om = allocb0_ite(16);
                        memset(om->b_rptr, 30000, 16);
                        putmblkq(&s->dump_q, dupmblk(om));
                    }
#else
                    flushcount = 0;
#endif
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
        s->framesize = 160;
        s->fn_cb     = NULL;
        s->isbypass  = false;
        s->msF       = f;
        s->Buf       = ite_rbuf_init(s->framesize * 12);
        InitAsrModels();
        AsrReset();
        printf("asr_init\n");
#ifdef DUMP_DATA
        mblkqinit(&s->dump_q);
#endif
        f->pthread_stacksize = 32 * 1024;
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
#ifdef DUMP_DATA
        {
            FILE *     dumpfile;
            static int index = 0;
            mblk_ite * dump;
            char       fname[128];
            char       USBPATH = Get_Storage_path();
            if (USBPATH == NULL)
            {
                printf("USB not insert \n");
            }
            else
            {
                printf("save audio data in USB %c:/ \n", USBPATH);
                sprintf(fname, "%c:/asr%03d.raw", USBPATH, index);
                dumpfile = fopen(fname, "w");
            }
            while (1)
            {
                dump = NULL;
                dump = getmblkq(&s->dump_q);
                if (dump)
                {
                    fwrite(dump->b_rptr, dump->size, 1, dumpfile);
                    freemsg_ite(dump);
                }
                else
                {
                    flushmblkq(&s->dump_q);
                    fclose(dumpfile);
                    break;
                }
            }
            index++;
        }
        printf("dump data end\n");
#endif
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