#include <stdio.h>
#include <sys/ioctl.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itp.h"

static void filewriter_callback_default ()
{
}
void (*filewriter_callback)(void * arg) = filewriter_callback_default;

static void Snapshot_success (void * arg)
{
    if (filewriter_callback)
    {
        filewriter_callback(arg);
    }
}

static void filewriter_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void filewriter_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void filewriter_process (IteFilter * f)
{
    IteQueueblk blk         = {0};
    mblk_ite *  im          = NULL;
    char *      Filename    = NULL;
    uint8_t *   GetStream   = NULL;
    uint32_t    GetFilesize = 0;
    FILE *      fout        = NULL;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            im       = (mblk_ite *)blk.datap;
            if (im == NULL) {
                DEBUG_PRINT("[%s] ERROR: im is NULL\n", __FUNCTION__);
                continue;
            }
            Filename = (char *)im->b_rptr;
            im->b_rptr += DEF_FileStream_Name_LENGTH;

            GetStream   = im->b_rptr;
            GetFilesize = im->b_wptr - im->b_rptr;

            if ((fout = fopen(Filename, "wb")))
            {
                fwrite(GetStream, 1, GetFilesize, fout);
                fclose(fout);

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
                ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
#if (CFG_LFS_CACHE_SIZE > 0)
                ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
#endif
                printf("file_write success\n");
                Snapshot_success((void *)Filename);
            }
            else
            {
                printf("open savefile fail !!\n");
            }

            freemsg_ite(im);
        }

        usleep(1000);
    }
}

// clang-format off
IteFilterDes FilterFileWriter = {
    ITE_FILTER_FILE_WRITER_ID,
    filewriter_init,
    filewriter_uninit,
    filewriter_process,
};
// clang-format on
