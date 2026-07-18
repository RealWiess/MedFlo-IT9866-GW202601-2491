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
        sleep(3);
    }
#endif

#if 0
    IteQueueblk   blk;
    unsigned char data;

    data               = 0x1;
    blk.datap          = (mblk_ite *)malloc(sizeof(mblk_ite));
    blk.datap->b_datap = &data;

    while (f->run)
    {
        if (ite_queue_put(f->output[0].Qhandle, &blk) == 0)
        {
            printf("[%s] FilterD put data\n", __FUNCTION__);

            sem_post(&f->output[0].semHandle);
        }
        sleep(3);
    }
#endif

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
}

static void test_method (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes FilterD_methods[] = {
    {ITE_FILTER_D_Method, test_method},
    {                  0,        NULL}
};

// clang-format off
IteFilterDes FilterD = {
    ITE_FILTER_D_ID,
    test_init,
    test_uninit,
    test_process,
    FilterD_methods
};
// clang-format on
