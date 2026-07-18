/***************************************************************************
 * Copyright ITE Tech. Inc. All Rights Reserved.
 *
 * @file
 * PCM/uLaw/aLaw/adpcm Decoder
 *
 * @author
 * @version 1.0
 * @date 2022
 *
 ***************************************************************************/
#include "mm.h"
#include "opus_config.h"
#include "opusfile.h"
#include "include/opusfile.h"
#include "io/internal.h"


#if defined(__OR32__)
#  include "mmio.h"
#  include "i2s.h"
#else
#  include "win32.h"
#endif

#include "debug.h"

#if defined(__FREERTOS__)
#  include "FreeRTOS.h"
#  include "task.h"
#endif
#include <stdarg.h>
#include <string.h>

#if defined(ENABLE_CODECS_PLUGIN)
#  include "plugin.h"
#endif

///////////////////////////////////////////////////////////////////////////
//                              Constant Definition
///////////////////////////////////////////////////////////////////////////
#if defined(__FREERTOS__)
#  define READBUF_SIZE          (8*1024)
#  define I2SBUFSIZE           READBUF_SIZE
#else
#  define READBUF_SIZE          (8*1024)
#  define I2SBUFSIZE           READBUF_SIZE
#endif // __FREERTOS__


#if READBUF_SIZE > 65535 || I2SBUFSIZE > 65535
#  error "The buffer exceed 64K bytes."
#endif

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
/* Code for general_printf() */
#define BITS_PER_BYTE    8
#define MINUS_SIGN       1
#define RIGHT_JUSTIFY    2
#define ZERO_PAD         4
#define CAPITAL_HEX      8

struct parameters
{
    int   number_of_output_chars;
    short minimum_field_width;
    char  options;
    short edited_string_length;
    short leading_zeros;
};

static unsigned int* gtAudioPluginBufferLength;
static unsigned short* gtAudioPluginBuffer;
static int gnTemp;
int gPrint = 0;
unsigned char *gBuf;
#endif
///////////////////////////////////////////////////////////////////////////
//                              Local Variable
//              [streamBuf]->[opusBuf :decoder]->[pcmWriteBuf]
//
///////////////////////////////////////////////////////////////////////////
#if defined(__GNUC__)
static unsigned char streamBuf[READBUF_SIZE] __attribute__ ((aligned(16), section (".sbss")));
static unsigned char pcmWriteBuf[I2SBUFSIZE] __attribute__ ((aligned(16), section (".sbss")));
#else
static unsigned char streamBuf[READBUF_SIZE];
static unsigned char pcmWriteBuf[I2SBUFSIZE];
#endif // defined(__GNUC__)
static unsigned int pcmReadIdx=0;
static unsigned int pcmWriteIdx=0;
static int streamRdPtr = 0;
static int streamWrPtr = 0;
static int gCh = 2;
static int gSampleRate = 44100;

void DebugPrintf(int Line,int param){
    ithPrintf("Line=%d param=%d",Line,param);
    AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
}

///////////////////////////////////////////////////////////////////////////
//                              Function Decleration
///////////////////////////////////////////////////////////////////////////
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SLEEP(x)  {for(int i=0;i<x;i++) asm("");}

// For Decoder Get the stream of write pointer, if the pointer is 0xffff then stop decoding.
static __inline unsigned int getStreamWrPtr(void) {
    unsigned int wrPtr;

    wrPtr = MMIO_Read(DrvDecode_WrPtr);
    if (0xffff == wrPtr)
    {
        setAudioReset();
        while (1);
    }
    return wrPtr;
}

/*get streamBuf Remain Bytes*/
int streamRemaindBytes()
{
	if(isEOF())
    {
        MMIO_Write(DrvDecode_RdPtr, getStreamWrPtr());
        return -1;
    }

	streamWrPtr = getStreamWrPtr();
	int nbytes = (streamRdPtr <= streamWrPtr) ? (streamWrPtr - streamRdPtr) : (READBUF_SIZE - (streamRdPtr - streamWrPtr));

	return nbytes;
}
/*read streamBuf(cpu1 cpu2 share memory) & copy to bufptr(cpu2)*/
//return streamBuf be copyed size
int streamRead(unsigned char *bufptr, int nbytes)
{

    if(streamRdPtr+nbytes<READBUF_SIZE){
        #ifdef CFG_CPU_WB
        ithInvalidateDCacheRange((void *)streamBuf + streamRdPtr, nbytes);
        #else
        dc_invalidate(); // Flush Data Cache
        #endif
        memcpy(bufptr, streamBuf + streamRdPtr, nbytes);
        streamRdPtr+=nbytes;
    }else{
        int size0=READBUF_SIZE-streamRdPtr;
        int size1=nbytes-size0;
        #ifdef CFG_CPU_WB
        ithInvalidateDCacheRange((void *)streamBuf + streamRdPtr, size0);
        ithInvalidateDCacheRange((void *)streamBuf              , size1);
        #else
        dc_invalidate(); // Flush Data Cache
        #endif
        memcpy(bufptr      , streamBuf + streamRdPtr, size0);
        memcpy(bufptr+size0, streamBuf              , size1);
        streamRdPtr=size1;
    }
    MMIO_Write(DrvDecode_RdPtr, streamRdPtr & 0xfffe);

    return nbytes;
}

/*read input data to decode buffer(mblkbuf),
return read Data Size, if not enough input data return 0.
return aviliable data bytes in mblkbuf to decode.
*/
static int DecFillReadBuffer(mblkbuf *im, int readBytes){
    int inputBufBytes = streamRemaindBytes();
    int ReadSize=0;
    if(readBytes>=inputBufBytes){
        ReadSize = streamRead(im->b_wptr, inputBufBytes);
    }else{
        ReadSize = streamRead(im->b_wptr, readBytes);
    }
    im->b_wptr+=ReadSize;
    return ReadSize;
}

/**************************************************************************************
 * Function     : DecClearRdBuffer
 **************************************************************************************/
static void ClearRdBuffer(void) {
	MMIO_Write(DrvDecode_WrPtr, 0);
    MMIO_Write(DrvDecode_RdPtr, 0);
	streamWrPtr = 0;
    streamRdPtr = 0;
	MMIO_Write(DrvAudioCtrl, MMIO_Read(DrvAudioCtrl) & ~DrvDecode_STOP);
	#if defined(__OPENRTOS__)
    dc_invalidate(); // Flush DC Cache
    #endif
}

int IsPcmBufFull(int len)
{
	int freebytes;
	if(isMusicCodecDump())
		pcmReadIdx = MMIO_Read(DrvDumpPCM_RdPtr);
	else
		pcmReadIdx = CODEC_I2S_GET_OUTRD_PTR();

	freebytes = pcmReadIdx - pcmWriteIdx;
    if (freebytes <= 0)
    {
        freebytes += sizeof(pcmWriteBuf);
    }
	if(freebytes < len + 2){
		return 1;
    }
	else
		return 0;
}

/*put pcm data to pcmWriteBuf & update rwpoint*/
void OutPutPcmData(unsigned char* databuf, int len)
{
    if((pcmWriteIdx + len) > I2SBUFSIZE){
		int size0 = I2SBUFSIZE - pcmWriteIdx;
		int size1 = len - size0;
		memcpy(pcmWriteBuf + pcmWriteIdx, databuf, size0);
		ithFlushDCacheRange((void*)(pcmWriteBuf + pcmWriteIdx), size0);
		ithFlushMemBuffer();

		memcpy(pcmWriteBuf, databuf+size0, size1);
		ithFlushDCacheRange((void*)(pcmWriteBuf), size1);
		ithFlushMemBuffer();
		pcmWriteIdx = size1;

    }else{
        memcpy(pcmWriteBuf + pcmWriteIdx, databuf, len);
		ithFlushDCacheRange((void*)(pcmWriteBuf + pcmWriteIdx), len);
		ithFlushMemBuffer();
        pcmWriteIdx += len;
    }
    if (pcmWriteIdx == I2SBUFSIZE) pcmWriteIdx = 0;

	MMIO_Write(AUDIO_PROCESSOR_WRITE_REGISTER_PROTECTION, 1);
	if(isMusicCodecDump())
		AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PCMIDX);//update rwp
	else
		CODEC_I2S_SET_OUTWR_PTR(pcmWriteIdx);//put to i2s buf and update rwp
	MMIO_Write(AUDIO_PROCESSOR_WRITE_REGISTER_PROTECTION, 0);

}

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
static void output_and_count(struct parameters *p, int c)
{
    if (p->number_of_output_chars >= 0)
    {
        int n = c;
        gBuf[gPrint++] = c;
        if (n >= 0)
            p->number_of_output_chars++;
        else
            p->number_of_output_chars = n;
    }
}

static void output_field(struct parameters *p, char *s)
{
    short justification_length =
        p->minimum_field_width - p->leading_zeros - p->edited_string_length;
    if (p->options & MINUS_SIGN)
    {
        if (p->options & ZERO_PAD)
            output_and_count(p, '-');
        justification_length--;
    }
    if (p->options & RIGHT_JUSTIFY)
        while (--justification_length >= 0)
            output_and_count(p, p->options & ZERO_PAD ? '0' : ' ');
    if (p->options & MINUS_SIGN && !(p->options & ZERO_PAD))
        output_and_count(p, '-');
    while (--p->leading_zeros >= 0)
        output_and_count(p, '0');
    while (--p->edited_string_length >= 0){
        output_and_count(p, *s++);
    }
    while (--justification_length >= 0)
        output_and_count(p, ' ');
}

int ithGPrintf(const char *control_string, va_list va)/*const int *argument_pointer)*/
{
    struct parameters p;
    char              control_char;
    p.number_of_output_chars = 0;
    control_char             = *control_string++;

    while (control_char != '\0')
    {
        if (control_char == '%')
        {
            short precision     = -1;
            short long_argument = 0;
            short base          = 0;
            control_char          = *control_string++;
            p.minimum_field_width = 0;
            p.leading_zeros       = 0;
            p.options             = RIGHT_JUSTIFY;
            if (control_char == '-')
            {
                p.options    = 0;
                control_char = *control_string++;
            }
            if (control_char == '0')
            {
                p.options   |= ZERO_PAD;
                control_char = *control_string++;
            }
            if (control_char == '*')
            {
                //p.minimum_field_width = *argument_pointer++;
                control_char          = *control_string++;
            }
            else
            {
                while ('0' <= control_char && control_char <= '9')
                {
                    p.minimum_field_width =
                        p.minimum_field_width * 10 + control_char - '0';
                    control_char = *control_string++;
                }
            }
            if (control_char == '.')
            {
                control_char = *control_string++;
                if (control_char == '*')
                {
                    //precision    = *argument_pointer++;
                    control_char = *control_string++;
                }
                else
                {
                    precision = 0;
                    while ('0' <= control_char && control_char <= '9')
                    {
                        precision    = precision * 10 + control_char - '0';
                        control_char = *control_string++;
                    }
                }
            }
            if (control_char == 'l')
            {
                long_argument = 1;
                control_char  = *control_string++;
            }
            if (control_char == 'd')
                base = 10;
            else if (control_char == 'x')
                base = 16;
            else if (control_char == 'X')
            {
                base       = 16;
                p.options |= CAPITAL_HEX;
            }
            else if (control_char == 'u')
                base = 10;
            else if (control_char == 'o')
                base = 8;
            else if (control_char == 'b')
                base = 2;
            else if (control_char == 'c')
            {
                base       = -1;
                p.options &= ~ZERO_PAD;
            }
            else if (control_char == 's')
            {
                base       = -2;
                p.options &= ~ZERO_PAD;
            }
            if (base == 0) /* invalid conversion type */
            {
                if (control_char != '\0')
                {
                    output_and_count(&p, control_char);
                    control_char = *control_string++;
                }
            }
            else
            {
                if (base == -1) /* conversion type c */
                {
                    //char c = *argument_pointer++;
                    char c = (char)(va_arg(va, int));
                    p.edited_string_length = 1;
                    output_field(&p, &c);
                }
                else if (base == -2) /* conversion type s */
                {
                    char *string;
                    p.edited_string_length = 0;
                    //string                 = *(char **) argument_pointer;
                    //argument_pointer      += sizeof(char *) / sizeof(int);
                    string = va_arg(va, char*);
                    while (string[p.edited_string_length] != 0)
                        p.edited_string_length++;
                    if (precision >= 0 && p.edited_string_length > precision)
                        p.edited_string_length = precision;
                    output_field(&p, string);
                }
                else /* conversion type d, b, o or x */
                {
                    unsigned long x = 0;
                    char          buffer[BITS_PER_BYTE * sizeof(unsigned long) + 1];
                    p.edited_string_length = 0;
                    if (long_argument)
                    {
                        //x                 = *(unsigned long *) argument_pointer;
                        //argument_pointer += sizeof(unsigned long) / sizeof(int);
                        va_arg(va, unsigned int);
                    }
                    else if (control_char == 'd')
                        //x = (long) *argument_pointer++;
                        x = va_arg(va, long);
                    else
                        //x = (unsigned) *argument_pointer++;
                        x = va_arg(va, int);
                    if (control_char == 'd' && (long) x < 0)
                    {
                        p.options |= MINUS_SIGN;
                        x          = -(long) x;
                    }
                    do
                    {
                        int c;
                        c = x % base + '0';
                        if (c > '9')
                        {
                            if (p.options & CAPITAL_HEX)
                                c += 'A' - '9' - 1;
                            else
                                c += 'a' - '9' - 1;
                        }
                        buffer[sizeof(buffer) - 1 - p.edited_string_length++] = c;
                    } while ((x /= base) != 0);
                    if (precision >= 0 && precision > p.edited_string_length)
                        p.leading_zeros = precision - p.edited_string_length;
                    output_field(&p, buffer + sizeof(buffer) - p.edited_string_length);
                }
                control_char = *control_string++;
            }
        }
        else
        {
            output_and_count(&p, control_char);
            control_char = *control_string++;
        }
    }
    return p.number_of_output_chars;
}

int ithPrintf(char* control, ...)
{
    va_list va;
    va_start(va,control);
    gPrint = 0;
    gBuf = (unsigned char*)gtAudioPluginBuffer;
    ithGPrintf(control, va);
    va_end(va);
    return 0;
}
void AudioPluginAPI(int nType)
{
    unsigned short nRegister;
    int i;
    int nTemp,nTemp1;
    unsigned char* pBuf;

    nRegister = (SMTK_AUDIO_PROCESSOR_ID<<14) | nType;
    switch (nType)
    {
        case SMTK_AUDIO_PLUGIN_CMD_ID_FILE_OPEN:
           break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_FILE_WRITE:
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_FILE_CLOSE:

            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC:
                nTemp  = (int)pcmWriteBuf;
                nTemp1 = sizeof(pcmWriteBuf);
                pBuf = (unsigned char*)gtAudioPluginBuffer;
                memcpy(&pBuf[0], &nTemp, sizeof(int));
                memcpy(&pBuf[4], &gCh, sizeof(int));
                memcpy(&pBuf[8], &gSampleRate, sizeof(int));
                memcpy(&pBuf[12], &nTemp1, sizeof(int));
                //printf("[Wav] 0x%x %d %d %d \n",nTemp,gCh,gSampleRate,nTemp1);
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_ADC:
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_PAUSE_DAC:
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_DEACTIVE_DAC:
        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_DEACTIVE_ADC:
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF:
            break;

		case SMTK_AUDIO_PLUGIN_CMD_ID_PCMIDX:
			pBuf = (unsigned char*)gtAudioPluginBuffer;
			memcpy(&pBuf[0], &pcmWriteIdx, sizeof(int));
            break;

        default:
            break;

    }
    ithFlushDCacheRange((void*)gtAudioPluginBuffer, (int)gtAudioPluginBufferLength);
    ithFlushMemBuffer();

    setAudioPluginMessageStatus(nRegister);
    i=200000*20;

    do
    {
        nRegister = getAudioPluginMessageStatus();
        nRegister = (nRegister & 0xc000)>>14;
        if (nRegister== SMTK_MAIN_PROCESSOR_ID)
        {
            break;
        }
        i--;
    }while(i && !isSTOP());
}
#endif // defined(AUDIO_PLUGIN_MESSAGE_QUEUE)

/*get memory block buf availe size*/
static int mblk_get_avail_size(mblkbuf *m){
    unsigned size;
    if(m->b_wptr>=m->b_rptr){
        size = m->b_wptr-m->b_rptr;
    }else{
        size = m->b_wptr - m->b_rptr + m->size;
    }
    return size;
}
/*rearrange memory block buf data ,
mblkbuf with avail data rearrange R point set initial pos,W point set to availe new avail pos
*/
static void mblk_buf_rearrange(mblkbuf *m){
    int avail=mblk_get_avail_size(m);
    if(avail){
        memmove(m->b_datap,m->b_rptr,avail);
        m->b_rptr=m->b_datap;
        m->b_wptr=m->b_datap+avail;
    }else{
        m->b_rptr=m->b_datap;
        m->b_wptr=m->b_datap;
    }
}

/*decoder memory block buf(mblkbuf) with rwpoint*/
static void mblk_init(mblkbuf *m, unsigned char *ptr, int length)
{
    m->b_datap= ptr;
    m->b_rptr = ptr;
    m->b_wptr = ptr;
    m->size   = length;
}

/**************************************************************************************
 * Function     : main
 **************************************************************************************/
#if defined(__FREERTOS__) && !defined(ENABLE_CODECS_PLUGIN)
portTASK_FUNCTION(opusdecode_task, params)
#else
int main(int argc, char **argv)
#endif
{
#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
    int* codecStream;
    AUDIO_CODEC_STREAM* audioStream;

    codecStream=(int*)CODEC_STREAM_START_ADDRESS;

    audioStream = (AUDIO_CODEC_STREAM*)(*codecStream);
    audioStream->codecStreamBuf = &streamBuf[0];
    audioStream->codecStreamLength =  sizeof(streamBuf);

    gtAudioPluginBuffer = audioStream->codecAudioPluginBuf;
    gtAudioPluginBufferLength = (unsigned int*)(audioStream->codecAudioPluginLength);

    ithFlushDCacheRange((void*)audioStream, sizeof(AUDIO_CODEC_STREAM));
    ithFlushMemBuffer();

	MMIO_Write(AUDIO_DECODER_START_FALG, 1);

#endif

    #if defined(WIN32) || defined(__CYGWIN__)
    GetParam(argc, argv);
    win32_init();
    #endif // defined(WIN32) || defined(__CYGWIN__)
    if (mm_init() < 0) {
        ithPrintf("mm_init failed\n");
        AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
        return 0;
    }
    for (;;)
    {  // Forever Loop
        #define RINGBUFFSIZE (1024*25)
        mblkbuf blk,*opusblk=&blk;
        OggOpusFile *OPUS;
        unsigned char  buf[RINGBUFFSIZE];
        /*opusblk init*/
        memset(buf, 0, RINGBUFFSIZE);
        mblk_init(opusblk, buf, RINGBUFFSIZE);


        int DACinit=0;
        {
            int availSize=0;
            int ReadSize;
            while(availSize<1024*24){
                int empty= (RINGBUFFSIZE)-mblk_get_avail_size(opusblk);//get opus buf empty size;
                ReadSize= DecFillReadBuffer(opusblk, empty);//fill opus buf from streambuf
                SLEEP(128*1000);//wait;
                availSize=mblk_get_avail_size(opusblk);
            }
            OPUS=op_open_memory(opusblk->b_rptr,RINGBUFFSIZE,NULL);
            {
                const OpusHead *head=op_head(OPUS,0);
                if(DACinit == 0){
                    gCh = head->channel_count;
                    gSampleRate = head->input_sample_rate;
                    AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC);
                    DACinit = 1;
                }
            }
            op_raw_mem_seek(OPUS,opusblk->b_rptr,availSize,(opus_int64)0);
        }

        while(1){
            char pcm[4096];
            int ret=op_read(OPUS,(opus_int16 *)pcm,sizeof(pcm),NULL);
            if(ret>0){
                int pcmlen=2*ret*gCh;
                while(IsPcmBufFull(pcmlen)){//wait some time for pcmWriteBuf reduce;
                    SLEEP(128*1000);
                }

                OutPutPcmData(pcm, pcmlen);//put pcm to buf

            }else{
                int ReadSize,empty;
                opusblk->b_rptr+=(int)OPUS->offset;
                mblk_buf_rearrange(opusblk);
                empty = (RINGBUFFSIZE)-mblk_get_avail_size(opusblk);//get opus buf empty size
                ReadSize= DecFillReadBuffer(opusblk, empty);//fill opus buf from streambuf

                int availSize=mblk_get_avail_size(opusblk);
                op_raw_mem_seek(OPUS,opusblk->b_rptr,availSize,(opus_int64)0);

			}
            SLEEP(128*1000);//wait;
            if(isSTOP()) break;

        }

		ClearRdBuffer();
		pcmReadIdx = 0;
		pcmWriteIdx = 0;


    }
	setAudioReset();
    #if !defined(__FREERTOS__)
    return 0;
    #endif
}
