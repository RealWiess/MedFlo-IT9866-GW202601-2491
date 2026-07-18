#include <stdio.h>
#include "flower/flower.h"
#define FIXED_POINT
#include "speex/fftwrap.h"
#include "speex/math_approx.h"

#define FFTRESOLOTION 512 // sample length

typedef struct _FFTData
{
    int        frame_size; // bytes size
    void *     fft_table;
    mblkq      dataQ;
    mblk_ite * w;
} FFTData;

mblk_ite * hanningW_init (int N)
{ // init hanning window mblk
    mblk_ite *      wt   = allocb_ite(N * 2);
    unsigned char * fptr = wt->b_wptr;
    unsigned char * lptr = wt->b_wptr + wt->size - 2;
    for (int i = 0; i < N >> 1; i++, fptr += 2, lptr -= 2)
    {
        *(spx_word16_t *)fptr =
            (16383 - SHL16(spx_cos(DIV32_16(MULT16_16(25736, i << 1), N)), 1));
        *(spx_word16_t *)lptr = *(spx_word16_t *)fptr;
    }
    wt->b_wptr += wt->size;
    return wt;
}

static void fft_init (IteFilter * f)
{
    FFTData * s = (FFTData *)ite_new(FFTData, 1);
    if (s != NULL)
    {
        s->frame_size =
            FFTRESOLOTION * 2; // size of bytes (1 sample = 2 byte (16 bit))
        s->fft_table = NULL;
        mblkQShapeInit(&s->dataQ, 4096);
        s->fft_table         = spx_fft_init(FFTRESOLOTION);
        s->w                 = hanningW_init(FFTRESOLOTION);

        f->pthread_stacksize = 32 * 1024;
    }
    f->data = s;
}

static void fft_uninit (IteFilter * f)
{
    FFTData * s = (FFTData *)f->data;
    if (s != NULL)
    {
        spx_fft_destroy(s->fft_table);
        mblkQShapeUninit(&s->dataQ);
        free(s);
    }
}

void print16_data (mblk_ite * im)
{
    int16_t * point;
    int       i = 0;
    int       N = im->size / 2;
    for (i = 0, point = (int16_t *)im->b_rptr; point < (int16_t *)im->b_wptr;
         ++point, i++)
    {
        printf("[%d] %d\n", i, *point);
    }
}

static void print16_spectrum (mblk_ite * im, int sr)
{
#define dBVALUE(a) (10 * log10((double)((a) * (a))))
    int16_t * point;
    int       i = 0;
    int       N = im->size / 2;
    printf("num of samples in a frame =%d\n", N);
    for (i = 0, point = (int16_t *)im->b_rptr; point < (int16_t *)im->b_wptr;
         ++point, i++)
    {
        double dB = dBVALUE(*point);
        if (dB < -100)
        {
            dB = -100;
        }
        printf("[%d][f:%d] %f %d\n", i, (sr * 1000 / 2 / N * i) / 1000, dB,
               *point);
        // printf("[%d][f:%d] %d + (%d)i\n",i,sr*100/N*i,*point ,*(point));
    }
}

void window_apply (mblk_ite * wt, mblk_ite * x)
{ // apply window
    int             N     = wt->size / 2;
    unsigned char * wfptr = wt->b_rptr;
    unsigned char * xfptr = x->b_rptr;
    for (int i = 0; i < N; i++, wfptr += 2, xfptr += 2)
    {
        *(spx_word16_t *)xfptr =
            MULT16_16_Q15(*(spx_word16_t *)wfptr, *(spx_word16_t *)xfptr);
    }
}

static void fft_process (IteFilter * f)
{
    FFTData *   s      = (FFTData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    IteQueueblk blk    = {0};
    mblk_ite *  Specm  = NULL; // temp spectrum
    int         count  = 0;
    Specm              = allocb_ite(s->frame_size);
    Specm->b_wptr += Specm->size;
    // while(f->run && !flower->dInfo.init) usleep(100000);
    printf("%f\n", log10(0.12));
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

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblkQShapePut(&s->dataQ, blk.datap, s->frame_size);
        }

        while (getmblkqavail(&s->dataQ))
        {
            mblk_ite * im    = getmblkq(&s->dataQ);
            mblk_ite * dupom = allocb_ite(im->size);
            spx_fft(s->fft_table, (spx_word16_t *)im->b_rptr,
                    (spx_word16_t *)Specm->b_rptr);
            if (count == 50)
            {
                window_apply(s->w, im);
                spx_fft(s->fft_table, (spx_word16_t *)im->b_rptr,
                        (spx_word16_t *)Specm->b_rptr);
                print16_spectrum(Specm, 48000);
            }
            spx_ifft(s->fft_table, (spx_word16_t *)Specm->b_rptr,
                     (spx_word16_t *)dupom->b_wptr);
            dupom->b_wptr += im->size;

            blk.datap = dupom;
            ite_queue_put(f->output[0].Qhandle, &blk);
            if (im)
            {
                freemsg_ite(im);
            }
            count++;
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
        if (flower->dInfo.nch != 1)
        {
            printf("error channel=%d only support 1 channel\n",
                   flower->dInfo.nch);
        }
    }
    if (Specm)
    {
        freemsg_ite(Specm);
    }
}

static void fft_set_value (IteFilter * f, void * arg)
{
    FFTData * d = (FFTData *)f->data;
}

static IteMethodDes fft_methods[] = {
    {ITE_FILTER_SET_VALUE, fft_set_value},
    {                   0,          NULL}
};

// clang-format off
IteFilterDes FilterFFT = {
    ITE_FILTER_FFT_ID,
    fft_init,
    fft_uninit,
    fft_process,
    fft_methods
};
// clang-format on
