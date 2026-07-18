#ifndef __CODER_H__
#define __CODER_H__

#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(__OR32__)
#define __OR32__
#endif

#define READBUF_SIZE (8*1024)
#define I2SBUFSIZE   (8*1024) 

typedef struct _mblkbuf
{
    unsigned char *b_datap;
    unsigned char *b_rptr;
    unsigned char *b_wptr;
    int size;
} mblkbuf;

void ithInvalidateDCacheRange(void *addr, unsigned int size);
void ithFlushDCacheRange(void *addr, unsigned int size);
void ithFlushMemBuffer(void);
void dc_invalidate(void);

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
void AudioPluginAPI(int nType);
__inline int ithPrintf(char* control, ...);
#endif


#endif  /* __CODER_H__ */
