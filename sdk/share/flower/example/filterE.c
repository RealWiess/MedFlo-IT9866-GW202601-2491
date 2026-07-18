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
    printf("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
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
        }

        //*(blk.datap->b_datap) = 0;
    }
#endif

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
}

static void test_method (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes FilterE_methods[] = {
    {ITE_FILTER_E_Method, test_method},
    {                  0,        NULL}
};

// clang-format off
IteFilterDes FilterE = {
    ITE_FILTER_E_ID,
    test_init,
    test_uninit,
    test_process,
    FilterE_methods
};
// clang-format on
