#ifndef __CODER_H__
#define __CODER_H__

#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(__OR32__)
#define __OR32__
#endif

//#define READBUF_SIZE        (1024*16)
#define READBUF_SIZE        (1024*8)

#define MAX_NGRAN       2       /* max granules */
#define MAX_NCHAN       2       /* max channels */
#define MAX_NSAMP       576     /* max samples per channel, per granule */
#define MAX_FRAMESIZE   MAX_NGRAN * MAX_NSAMP   // max samples per channel //

//#define WAV_SPECTREM_PERFORMANCE_TEST
//#define WAV_RESET_DECODED_BYTE
/******************************
 The Buffer size of output PCM
 ******************************/
#define MAX_FRAMEBYTES  (sizeof(short) * MAX_NCHAN * MAX_FRAMESIZE) //2*2*2*576 = 4608
//#  define I2SBUFSIZE (MAX_FRAMEBYTES * 6) //4608*6 = 27648
#  define I2SBUFSIZE (MAX_FRAMEBYTES * 8) //4608*6 = 27648

typedef struct _mblkbuf
{
    unsigned char *b_datap;
    unsigned char *b_rptr;
    unsigned char *b_wptr;
    int size;
} mblkbuf;

//mem declare
void ithInvalidateDCacheRange(void *addr, unsigned int size);
void ithFlushDCacheRange(void *addr, unsigned int size);
void ithFlushMemBuffer(void);
void dc_invalidate(void);

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
void AudioPluginAPI(int nType);
int ithPrintf(char* control, ...);
#endif

#endif  /* __CODER_H__ */
