#include <stdio.h>
#include "flower/flower.h"

typedef struct _VoidData
{
    void * tmp;
} VoidData;

static void void_init (IteFilter * f)
{
    VoidData * s = (VoidData *)ite_new(VoidData, 1);
    f->data      = s;
}

static void void_uninit (IteFilter * f)
{
    VoidData * s = (VoidData *)f->data;
    if (s != NULL)
    {
        free(s);
    }
}

static void void_process (IteFilter * f)
{
    // VoidData *s=(VoidData*)f->data;
    IteQueueblk blk = {0};

    while (f->run)
    {

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;
            // ite_queue_put(f->output[0].Qhandle, &blk);
            if (om)
            {
                freemsg_ite(om);
                om        = NULL;
                blk.datap = NULL;
            }
        }
        usleep(20000);
    }
}

static void void_set_method (IteFilter * f, void * arg)
{
}

static IteMethodDes void_methods[] = {
    {ITE_FILTER_A_Method, void_set_method},
    {                  0,            NULL}
};

// clang-format off
IteFilterDes FilterVoid = {
    ITE_FILTER_VOID_ID,
    void_init,
    void_uninit,
    void_process,
    void_methods
};
// clang-format on
