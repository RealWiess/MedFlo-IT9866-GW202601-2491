#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

IteFlower * gflower = NULL;

/*Init Flow*/
IteFlower * IteStreamInit (void)
{
    flow_init_AD();
    flow_init_DA();

#if CFG_RESERVE_FILTER
    IteFlower * f = ItoFlowInit(1);
#else
    IteFlower * f = (IteFlower *)ite_new0(IteFlower, 1);
#endif
    if (f != NULL)
    {
        ite_flower_init();
        f->audiostream = NULL;
        f->asrstream   = NULL;
        f->videostream = NULL;
        f->audiocase   = AudioIdle;

        dinfoSetDefault(&f->dInfo);

#if CFG_BUILD_FFMPEG
        gfSetInitLinkToFFmpeg(&f);
#endif
    }
    else
    {
        DEBUG_PRINT("[%s] ERROR: malloc fail\n", __FUNCTION__);
    }
    gflower = f;

    return f;
}

void IteStreamUninit (IteFlower * f)
{
    if (f != NULL)
    {
        if (f->audiostream)
        {
            AudioStreamCancel(f);
        }
        free(f);
    }
    // f=NULL;
}
