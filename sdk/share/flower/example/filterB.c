#include <stdio.h>
#include "flower/flower.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void test_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void test_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void test_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

#if 1
    while (f->run)
    {
        printf("[%s] Filter(%d). thread run\n", __FUNCTION__, f->filterDes.id);
        //        while(f->status == FILTER_PAUSE) {
        //            sleep(1);  // non-blocking wait
        //        }
        sleep(3);
    }
#endif

#if 0
    IteQueueblk   blk;
    IteBufferWrap BW0;
    uint8_t       data0[10] = {0};
    int           data_len  = 0;

    ite_bufferWrap_init(&BW0, 32);

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            int size = strlen(blk.streamBuffer);

            printf("[FilterB] GET_Q buffer=%s size=%d\n", blk.streamBuffer,
                   size);

            ite_buffer_put(&BW0, blk.streamBuffer, size);
        }

        if ((data_len = ite_buffer_get(&BW0, data0, 3)) > 0)
        {
            printf("[FilterB] GET_BUFFER data_len=%d data=%s\n", data_len,
                   data0);
            data_len = 0;
            memset(data0, 0, sizeof(char) * 10);
        }

        sleep(10);
    }

    ite_bufferWrap_deinit(&BW0);
#endif

#if 0
    IteQueueblk blk;
    while (f->run)
    {
        sem_wait(&f->input[0].semHandle);

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            *(blk.datap->b_datap) = *(blk.datap->b_datap) | 0x2;

            if (ite_queue_put(f->output[0].Qhandle, &blk) == 0)
            {
                sem_post(&f->output[0].semHandle);
            }

            // printf("[%s] FilterB blk0=%x\n", __FUNCTION__,
            // *(blk.datap->b_datap));
        }

        //*(blk.datap->b_datap) = 0;
    }
#endif

#if 0
    IteQueueblk blk0, blk1;
    while (f->run)
    {
        sem_wait(&f->input[0].semHandle);

        if (ite_queue_get(f->input[0].Qhandle, &blk0) == 0)
        {
            *(blk0.datap->b_datap) = *(blk0.datap->b_datap) | 0x2;

            if (ite_queue_put(f->output[0].Qhandle, &blk0) == 0)
            {
                sem_post(&f->output[0].semHandle);
            }

            printf("[%s] FilterB blk0=%x\n", __FUNCTION__,
                   *(blk0.datap->b_datap));
        }

        sem_wait(&f->input[1].semHandle);

        if (ite_queue_get(f->input[1].Qhandle, &blk1) == 0)
        {
            *(blk1.datap->b_datap) =
                *(blk0.datap->b_datap) | *(blk1.datap->b_datap);

            if (ite_queue_put(f->output[1].Qhandle, &blk1) == 0)
            {
                sem_post(&f->output[1].semHandle);
            }

            printf("[%s] FilterB blk1=%x\n", __FUNCTION__, blk1.data);
        }

        // reset
        *(blk0.datap->b_datap) = 0;
        *(blk1.datap->b_datap) = 0;
    }
#endif

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
}

static void test_method (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes FilterB_methods[] = {
    {ITE_FILTER_B_Method, test_method},
    {                  0,        NULL}
};

// clang-format off
IteFilterDes FilterB = {
    ITE_FILTER_B_ID,
    test_init,
    test_uninit,
    test_process,
    FilterB_methods
};
// clang-format on
