

#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(__OR32__)
#define __OR32__
#endif

typedef enum {
  WAVE_FORMAT_UNKNOWN    = 0x0000, /* Microsoft Unknown Wave Format */
  WAVE_FORMAT_PCM        = 0x0001, /* Microsoft PCM Format */
  WAVE_FORMAT_ADPCM      = 0x0002, /* Microsoft ADPCM Format */
  WAVE_FORMAT_ALAW       = 0x0006, /* Microsoft ALAW */
  WAVE_FORMAT_MULAW      = 0x0007, /* Microsoft MULAW */
  WAVE_FORMAT_DVI_ADPCM  = 0x0011, /* Intel's DVI ADPCM */
  WAVE_FORMAT_G723_ADPCM = 0x0014, /* G.723 ADPCM */
  WAVE_FORMAT_G726_ADPCM = 0x0064, /* G.726 ADPCM */
  WAVE_FORMAT_G722_ADPCM = 0x0065  /* G.722 ADPCM */
} WAVE_FORMAT;

typedef struct _WaveInfo {
    WAVE_FORMAT format;
    unsigned int nChans;
    unsigned int sampRate;
    unsigned int avgBytesPerSec;
    unsigned int blockAlign;
    unsigned int bitsPerSample;
    unsigned int samplesPerBlock;
    unsigned int offset;//header size;
} WaveInfoMode;

typedef struct _mblkbuf
{
    unsigned char *b_datap;
    unsigned char *b_rptr;
    unsigned char *b_wptr;
    int size;
} mblkbuf;

typedef struct _pcm
{
    unsigned char *data_p;
    int len;
}pcmData;

//mem declare
void ithInvalidateDCacheRange(void *addr, unsigned int size);
void ithFlushDCacheRange(void *addr, unsigned int size);
void ithFlushMemBuffer(void);
void dc_invalidate(void);
//wav decoder
int wavDecoder(mblkbuf *m,WaveInfoMode *Info,pcmData *Pcm);
//debug
#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
void AudioPluginAPI(int nType);
int ithPrintf(char* control, ...);
#endif