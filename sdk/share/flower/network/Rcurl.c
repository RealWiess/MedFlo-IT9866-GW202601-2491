#include <stdio.h>
#include "curl/curl.h"
#include "flower/flower.h"
#include "audio/include/fileheader.h"
#include "curl/curl.h"
#include "ite/audio.h"

#ifdef CFG_NET_WIFI
extern int WifiMgr_Get_Sta_Connect_Complete (void);
#endif

typedef struct _CurlData
{
    PlayerState state;
    CURL *      curl;
    CURLM *     curlm;
    mblkq       dataQ;
    uint32_t    size_len;
    int         count;
    int         recvEof;
    bool        curlPause;
    int         chopsize;
} CurlData;

size_t get_stream (void * buffer, size_t size, size_t nmemb, void * userp)
{
    IteFilter * f      = (IteFilter *)userp;
    CurlData *  s      = (CurlData *)f->data;
    IteQueueblk blk    = {0};
    int         rbytes = size * nmemb;
    srcQShapePut(&s->dataQ, buffer, rbytes, s->chopsize);
    s->size_len += size * nmemb;
    if (s->count++ % 100 == 0)
    {
        printf("recv : %ud(kb)\n", s->size_len / 1024);
    }
    // printf("recv %d\n",rbytes);
    return size * nmemb;
}

static void Curlconnect_Start (IteFilter * f)
{
    CurlData * s = (CurlData *)f->data;
    if (s->curlm != NULL)
    {
        curl_multi_cleanup(s->curlm);
    }
    s->curlm = curl_multi_init();
    curl_easy_setopt(s->curl, CURLOPT_RESUME_FROM, s->size_len);
    curl_multi_add_handle(s->curlm, s->curl);
}

static int CheckCurlState (IteFilter * f)
{
    CurlData * s   = (CurlData *)f->data;
    int        ret = 0;

    if (s->curlPause == false)
    {
        if (f->status == FILTER_PAUSE
#ifdef CFG_NET_WIFI
            || WifiMgr_Get_Sta_Connect_Complete() != 1
#endif
        )
        {
            curl_multi_remove_handle(s->curlm, s->curl);
            // curl_multi_cleanup(s->curlm);
            s->curlPause = true;
            ret          = 1;
            printf("curl pause\n");
        }
    }

    if (s->curlPause == true)
    {
        if (f->status == FILTER_RESUME
#ifdef CFG_NET_WIFI
            && WifiMgr_Get_Sta_Connect_Complete() == 1
#endif
        )
        {
            // s->curlm = curl_multi_init();
            // curl_easy_setopt(s->curl, CURLOPT_RESUME_FROM , s->size_len);
            curl_multi_add_handle(s->curlm, s->curl);
            s->curlPause = false;
            printf("curl resume\n");
        }
        else
        {
            usleep(500000);
        }
        ret = 1;
    }

    return ret;
}

static void * curl_task (IteFilter * f)
{
    CurlData * s       = (CurlData *)f->data;
    CURLcode   res     = -1;
    int        runflag = 1;
    Curlconnect_Start(f);

    while (s->recvEof && f->run)
    {

        if (CheckCurlState(f))
        {
            continue;
        }
        res = curl_multi_perform(s->curlm, &runflag);
        if (CURLE_OK != res)
        {
            printf("curl_multi_perform() fail: %d\n", res);
        }
        usleep(1000 * (getmblkqavail(&s->dataQ) + 1));

        if (!runflag)
        {
            struct CURLMsg * m;
            int              msgq = 0;
            m                     = curl_multi_info_read(s->curlm, &msgq);
            if (m->data.result == CURLE_OK)
            {
                s->recvEof = 0;
                printf("recvEof msg =%d\n", m->data.result);
            }
            else
            {
                Curlconnect_Start(f);
                usleep(100000);
                printf("error msg =%d reconnect again\n", m->data.result);
            }
        }
    }

    printf("eof curl_task :(%d) Qdata:%d\n", s->recvEof,
           getmblkqavail(&s->dataQ));

    return (void *)0;
}

static void curl_init (IteFilter * f)
{
    CurlData * s = (CurlData *)ite_new(CurlData, 1);
    if (s != NULL)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        s->curl      = curl_easy_init();
        s->curlm     = NULL;
        s->state     = Closed;
        // mblkQShapeInit(&s->dataQ,4096);
        s->chopsize  = 512;
        s->size_len  = 0;
        s->count     = 0;
        s->recvEof   = 1;
        s->curlPause = false;
    }
    f->data = s;
}

static void curl_uninit (IteFilter * f)
{
    CurlData * s = (CurlData *)f->data;
    if (s != NULL)
    {
        if (s->curl)
        {
            curl_easy_cleanup(s->curl);
        }
        if (s->curlm)
        {
            curl_multi_cleanup(s->curlm);
        }
        mblkQShapeUninit(&s->dataQ);
        curl_global_cleanup();
        free(s);
    }
}

static void curl_process (IteFilter * f)
{
    CurlData *  s      = (CurlData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    IteQueueblk blk    = {0};

    CreateDetachedThread(curl_task, f);
    s->state = Playing;

    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }
        if (s->state == Playing)
        {
            if (getmblkqavail(&s->dataQ) > 0)
            {
                mblk_ite * om = getmblkq(&s->dataQ);
                blk.datap     = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
                // printf("%d\n",getmblkqavail(&s->dataQ));
            }
            if (s->recvEof == 0 && getmblkqavail(&s->dataQ) == 0 &&
                flower->dInfo.init)
            {
                int avail = ite_rbuf_get_avail_size(s->dataQ.rb);
                if (avail)
                { // get final data from ringbuf
                    mblk_ite * om = allocb_ite(avail);
                    ite_rbuf_get(om->b_wptr, s->dataQ.rb, avail);
                    om->b_wptr += avail;
                    blk.datap = om;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                }
                s->state = Eof;
            }
        }

        if (s->state == Eof)
        { // add null time(data) prevent abnormal sound
            blk.datap = allocb0_ite(0);
            // blk.private1=Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            s->state = Closed;
            printf("EOF\n");
        }
        usleep(5000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void curl_chopsize_setting (IteFilter * f, const char * url)
{
    CurlData * s         = (CurlData *)f->data;
    int        codecType = audiomgrCodecType((char *)url);
    switch (codecType)
    {
        case ITE_MP3_DECODE:
            s->chopsize = 2 * 1024;
            break;
        case ITE_FLAC_DECODE:
            s->chopsize = 8 * 1024;
            break;
        case ITE_AAC_DECODE:
            s->chopsize = 2 * 1024;
            break;
    }
    mblkQShapeInit(&s->dataQ, (s->chopsize + 64));
}

static void curl_open (IteFilter * f, void * arg)
{
    CurlData *   s   = (CurlData *)f->data;
    const char * url = (const char *)arg;
    curl_chopsize_setting(f, url);
    printf("+curl_play_stream(%s)\n", url);
    curl_easy_setopt(s->curl, CURLOPT_URL, url);
    curl_easy_setopt(s->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(s->curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(s->curl, CURLOPT_WRITEFUNCTION, get_stream);
    curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(s->curl, CURLOPT_WRITEDATA, (void *)f);
}

static void curl_set_method (IteFilter * f, void * arg)
{
}

static IteMethodDes curl_methods[] = {
    {ITE_FILTER_FILEOPEN,       curl_open},
    {ITE_FILTER_A_Method, curl_set_method},
    {                  0,            NULL}
};

// clang-format off
IteFilterDes FilterCurl = {
    ITE_FILTER_CURL_ID,
    curl_init,
    curl_uninit,
    curl_process,
    curl_methods
};
// clang-format on
