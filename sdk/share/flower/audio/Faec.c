#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "ite/audio.h"
#ifdef AEC_RUN_IN_ARM
    #include "type_def.h"
    #include "aecm_core.h"
    #include "basic_op.h"
    #include "hd_aec.h"
    #include "rfft_256.h"
#endif

#ifdef ENABLE_DUMP_AEC_DATA
    #define DUMP_AEC_DATA
#endif

#ifdef DUMP_AEC_DATA
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

static int noiseLev = 0;
//=============================================================================
//                              Private Function Declaration
//=============================================================================
typedef struct SbcAECState
{
    IteFilter *     msF;
    mblkq           refQ;
    mblkq           ecoQ;

    int             framesize;
    int             samplerate;
    int             delay_ms;
    int             AECframesize;

    bool            echostarted;
    bool            bypass_mode;
    bool            hw_start;

    pthread_t       hw_thread;
    pthread_mutex_t mutex;

#ifdef DUMP_AEC_DATA
    mblkq eco_copy_q;
    mblkq ref_copy_q;
    mblkq clean_copy_q;
#endif

} SbcAECState;

#ifdef AEC_RUN_IN_ARM
void AEC_IN_ARM_INIT (int * AECframesize)
{
    AEC_Init(&aec_config[0]);
    NR_Create(&anr_config[0], 1);
    AEC_Create(&aec_config[0]);

    #if (CFG_AUDIO_SAMPLING_RATE == 16000)
    *AECframesize = 256;
    #else
    *AECframesize = 128;
    #endif
}

void AEC_IN_ARM_UNINIT ()
{
    AEC_Destroy(&aec_config[0]);
    // TimeStretch_Destroy(&sndstrh_config[0]);
}
#endif

static void * hw_engine_thread (void * arg)
{
    SbcAECState * s         = (SbcAECState *)arg;
    int           hw_nbytes = s->AECframesize * 2;
#ifdef DUMP_AEC_DATA
    mblkqinit(&s->eco_copy_q);
    mblkqinit(&s->ref_copy_q);
    mblkqinit(&s->clean_copy_q);
#endif

    IteQueueblk blk = {0};

    while (s->hw_start)
    {
        int err1, err2;

        pthread_mutex_lock(&s->mutex);
        err1 = getmblkqavail(&s->refQ);
        err2 = getmblkqavail(&s->ecoQ);
        pthread_mutex_unlock(&s->mutex);
        // printf("err1 = %d err2 =%d\n",err1,err2);
        if (err1 && err2)
        {
            mblk_ite *ref, *eco, *oecho;
            oecho = allocb_ite(hw_nbytes);

            pthread_mutex_lock(&s->mutex);
            ref = getmblkq(&s->refQ);
            eco = getmblkq(&s->ecoQ);
            pthread_mutex_unlock(&s->mutex);

#ifdef AEC_RUN_IN_ARM
    #if (CFG_AUDIO_SAMPLING_RATE == 16000)
            PAES_Process_Block(
                &aecm, eco->b_rptr, ref->b_rptr,
                oecho->b_wptr); // 16K
                                // memcpy(oecho->b_wptr,eco->b_rptr,hw_nbytes);
                                // FreqWrapping(eco->b_rptr, ref->b_rptr,
                                // oecho->b_wptr, &aec_config[0]);
    #else
            // aecm_core(eco->b_rptr, ref->b_rptr, oecho->b_wptr,
            // &aec_config[0]);
            if (ref)
            {
                EchoPlayBack(&aec_config[0], (Word16 const *)(ref->b_rptr));
            }
            if (eco)
            {
                EchoCapture(&aec_config[0], (Word16 const *)(eco->b_rptr),
                            (Word16 *)(oecho->b_wptr), FRAME_LEN);
            }
                // TimeStretch(eco->b_rptr, oecho->b_wptr, &sndstrh_config[0]);
    #endif
#else
            if (eco)
            {
                iteAecCommand(AEC_CMD_PROCESS, (uint32_t)eco->b_rptr,
                              (uint32_t)ref->b_rptr, (uint32_t)oecho->b_wptr,
                              hw_nbytes, 0);
            }
#endif
            oecho->b_wptr += hw_nbytes;

#ifdef DUMP_AEC_DATA
            putmblkq(&(s->eco_copy_q), dupmblk(eco));
            putmblkq(&(s->ref_copy_q), dupmblk(ref));
            putmblkq(&(s->clean_copy_q), dupmblk(oecho));
#endif

            blk.datap = oecho;
            ite_queue_put(s->msF->output[0].Qhandle, &blk);

            if (eco)
            {
                freemsg_ite(eco);
            }
            if (ref)
            {
                freemsg_ite(ref);
            }
        }
        usleep(1000);

        if (s->hw_start == false)
        {
            break;
        }
    }

#ifdef DUMP_AEC_DATA
    {
        FILE *     echofile;
        FILE *     reffile;
        FILE *     cleanfile;
        static int index = 0;
        mblk_ite * eco, *ref, *clean;
        char       fname[128];
        char       USBPATH = Get_Storage_path();
        if (USBPATH == NULL)
        {
            printf("USB not insert \n");
        }
        else
        {
            printf("save audio data in USB %c:/ \n", USBPATH);
            sprintf(fname, "%c:/eco%03d.raw", USBPATH, index);
            echofile = fopen(fname, "w");
            sprintf(fname, "%c:/ref%03d.raw", USBPATH, index);
            reffile = fopen(fname, "w");
            sprintf(fname, "%c:/clean%03d.raw", USBPATH, index);
            cleanfile = fopen(fname, "w");
        }

        while (1)
        {
            eco = ref = NULL;
            eco       = getmblkq(&s->eco_copy_q);
            ref       = getmblkq(&s->ref_copy_q);
            clean     = getmblkq(&s->clean_copy_q);
            if (eco && ref && clean)
            {
                fwrite(eco->b_rptr, hw_nbytes, 1, echofile);
                freemsg_ite(eco);
                fwrite(ref->b_rptr, hw_nbytes, 1, reffile);
                freemsg_ite(ref);
                fwrite(clean->b_rptr, hw_nbytes, 1, cleanfile);
                freemsg_ite(clean);
            }
            else
            {
                flushmblkq(&s->eco_copy_q);
                flushmblkq(&s->ref_copy_q);
                flushmblkq(&s->clean_copy_q);
                fclose(echofile);
                fclose(reffile);
                fclose(cleanfile);
                break;
            }
        }
        index++;
    }
#endif

    return (void *)0;
}

static void aec_start_hwthread (SbcAECState * e)
{

    if (e->hw_start == false)
    {
        pthread_attr_t     attr;
        struct sched_param param;
        e->hw_start = true;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 25 * 1024);
        param.sched_priority = sched_get_priority_min(0) + 1;
        pthread_attr_setschedparam(&attr, &param);
        pthread_create(&e->hw_thread, &attr, hw_engine_thread, e);
    }
}

static void aec_stop_hwthread (SbcAECState * e)
{
    e->hw_start = false;
    pthread_join(e->hw_thread, NULL);
}

static void aec_init (IteFilter * f)
{
    SbcAECState * s = (SbcAECState *)ite_new(SbcAECState, 1);
    if (s != NULL)
    {
        s->samplerate = CFG_AUDIO_SAMPLING_RATE;

        mblkQShapeInit(&s->refQ, 256 * 16);
        mblkQShapeInit(&s->ecoQ, 256 * 16);

        s->echostarted = false;
        s->bypass_mode = false;
        s->hw_start    = false;

        s->delay_ms    = 0;

        pthread_mutex_init(&s->mutex, NULL);

#ifdef AEC_RUN_IN_ARM
        AEC_IN_ARM_INIT(&s->AECframesize);
        printf("ARM version\n");
#else
        iteAudioOpenEngine(ITE_SBC_CODEC);
        iteAecCommand(AEC_CMD_INIT, noiseLev, 0, 0, 0,
                      (uint32_t *)&s->AECframesize);
        printf("armLite version\n");
#endif
        printf("s->AECframesize=%d delay_ms=%d\n", s->AECframesize,
               s->delay_ms);
        s->framesize = s->AECframesize; // s->framesize can be any size, equal
                                        // to AECframesize more efficient
#if 0
        if (s->delay_ms > 0)
        {
            mblk_ite * m;
            int        delay_samples = s->delay_ms * s->samplerate / 1000;
            m                        = allocb_ite(delay_samples * 2);
            memset(m->b_wptr, 0, delay_samples * 2);
            m->b_wptr += delay_samples * 2;
            ite_rbuf_put(s->delayed_ref, m->b_rptr, m->size);
            if (m)
            {
                freemsg_ite(m);
            }
        }
#endif
        aec_start_hwthread(s);
        f->pthread_stacksize = 16 * 1024;
        s->msF               = f;
    }
    f->data = s;
}

static void aec_uninit (IteFilter * f)
{
    SbcAECState * s = (SbcAECState *)f->data;
    if (s != NULL)
    {
        mblkQShapeUninit(&s->refQ);
        mblkQShapeUninit(&s->ecoQ);
#ifdef AEC_RUN_IN_ARM
        AEC_IN_ARM_UNINIT();
#endif
        pthread_mutex_destroy(&s->mutex);
        free(s);
    }
}

/*  inputs[0]= reference signal from far end (sent to soundcard)
 *  inputs[1]= near speech & eco signal    (read from soundcard)
 *  outputs[0]=  is a copy of inputs[0] to be sent to soundcard
 */
static void aec_process (IteFilter * f)
{
    SbcAECState * s      = (SbcAECState *)f->data;
    int           nbytes = s->AECframesize * 2;

    IteQueueblk   blkref = {0};
    IteQueueblk   blkeco = {0};
    bool          start  = false;
    while (f->run)
    {

        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }

        if (s->bypass_mode)
        {

            if (ite_queue_get(f->input[1].Qhandle, &blkeco) == 0)
            {
                ite_queue_put(f->output[0].Qhandle, &blkeco);
            }

            if (ite_queue_get(f->input[0].Qhandle, &blkref) == 0)
            {
                mblk_ite * rm = blkref.datap;
                freemsg_ite(rm);
            }

            usleep(10000);
            continue;
        }

        if (start == false)
        {
            if (ite_queue_get_size(f->input[0].Qhandle) > 0)
            {
                start = true; // ref get ,start to AEC process
            }
            else
            {
                ite_mblk_queue_flush(
                    f->input[1].Qhandle); // ref not get , flush echo
            }
        }

        pthread_mutex_lock(&s->mutex);
        // ite_queue_put_to_mblkQShape(&blkref,f,0,&s->refQ,s->AECframesize*2);
        // ite_queue_put_to_mblkQShape(&blkeco,f,1,&s->ecoQ ,s->AECframesize*2);

        while (ite_queue_get(f->input[0].Qhandle, &blkref) == 0)
        {
            mblkQShapePut(&s->refQ, blkref.datap, s->AECframesize * 2);
        }
        while (ite_queue_get(f->input[1].Qhandle, &blkeco) == 0)
        {
            mblkQShapePut(&s->ecoQ, blkeco.datap, s->AECframesize * 2);
        }
        pthread_mutex_unlock(&s->mutex);

        usleep(10000);
    }

    aec_stop_hwthread(s);
}

static void aec_set_bypass (IteFilter * f, void * arg)
{
    SbcAECState * s = (SbcAECState *)f->data;
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    s->bypass_mode = 1;
}

// clang-format off
static IteMethodDes aec_methods[] = {
    {ITE_FILTER_SET_BYPASS , aec_set_bypass},
    {0, NULL}
};

IteFilterDes FilterAec = {
    ITE_FILTER_AEC_ID,
    aec_init,
    aec_uninit,
    aec_process,
    aec_methods
};
// clang-format on
