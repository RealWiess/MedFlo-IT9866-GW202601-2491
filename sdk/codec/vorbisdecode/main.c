/***************************************************************************
 * Copyright ITE Tech. Inc. All Rights Reserved.
 *
 * @file
 * Ogg Vorbis Decoder
 *
 * @author 
 * @version 1.0
 * @date 2022
 *
 ***************************************************************************/

#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(__OR32__)
#define __OR32__
#endif

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

#include "mm.h"
#include "misc.h"
#include "ivorbiscodec.h"

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
//              [streamBuf]->[wavBuf :decoder]->[pcmWriteBuf]
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

unsigned char  inbuffer[4096];
ogg_int16_t outbuffer[4096]; /* take 8k out of the data segment, not the stack */

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

static int getFrame(char *buffer, mblkbuf *oggblk, int size)
{
    int ReadSize=0,empty;
    while(ReadSize==0 && !isEOF() && !isSTOP())
    {
        mblk_buf_rearrange(oggblk);
        empty = size-mblk_get_avail_size(oggblk);//get wav buf empty size
        ReadSize= DecFillReadBuffer(oggblk, empty);//fill wav buf from streambuf
        SLEEP(128*1000);
    }
    memmove(buffer,oggblk->b_rptr,ReadSize);
    oggblk->b_rptr+=ReadSize;
    return ReadSize;
}

/**************************************************************************************
 * Function     : main
 **************************************************************************************/
#if defined(__FREERTOS__) && !defined(ENABLE_CODECS_PLUGIN)
portTASK_FUNCTION(wavdecode_task, params)
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
    ithPrintf("start Ogg Vorbis Codec.\n");
    AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
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

    // for (;;)
    {  // Forever Loop
        ogg_sync_state   oy={0}; /* sync and verify incoming physical bitstream */
        ogg_stream_state os={0}; /* take physical pages, weld into a logical
                                stream of packets */
        ogg_page         og={0}; /* one Ogg bitstream page. Vorbis packets are inside */
        ogg_packet       op={0}; /* one raw packet of data for decode */

        vorbis_info      vi={0}; /* struct that stores all the static vorbis bitstream
                                settings */
        vorbis_comment   vc={0}; /* struct that stores all the bitstream user comments */
        vorbis_dsp_state vd={0}; /* central working state for the packet->PCM decoder */
        vorbis_block     vb={0}; /* local working space for packet->PCM decode */

        char *buffer;
        int  bytes=0;
        int convsize=4096;
        int eos=0;
        int i;

        // int DACinit=0;
        mblkbuf blk,*oggblk=&blk;
        unsigned char  *buf=inbuffer;
        int empty;
        
        /*blk init*/
        mblk_init(oggblk, buf, READBUF_SIZE);

        /********** Decode setup ************/

        ogg_sync_init(&oy); /* Now we can read pages */

        /* submit a 4k block to libvorbis' Ogg layer */
        buffer=ogg_sync_buffer(&oy,4096);
        // if(buffer==NULL){
            // ithPrintf("!!!buff NULL!!\n",bytes);
            // AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            // goto end;
        // }
        bytes=getFrame(buffer,oggblk,4096);
        ogg_sync_wrote(&oy,bytes);

        /* Get the first page. */
        if(ogg_sync_pageout(&oy,&og)!=1)
        {
            /* have we simply run out of data?  If so, we're done. */
            // if(bytes<4096)break;

            /* error case.  Must not be Vorbis data */
            ithPrintf("Input does not appear to be an Ogg bitstream.(%d)\n",bytes);
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            goto end;
        }

        /* Get the serial number and set up the rest of decode. */
        /* serialno first; use it to set up a logical stream */
        ogg_stream_init(&os,ogg_page_serialno(&og));

        /* extract the initial header from the first page and verify that the Ogg bitstream is in fact Vorbis data */

        /* I handle the initial header first instead of just having the code
        read all three Vorbis headers at once because reading the initial
        header is an easy way to identify a Vorbis bitstream and it's
        useful to see that functionality seperated out. */

        vorbis_info_init(&vi);
        vorbis_comment_init(&vc);
        if(ogg_stream_pagein(&os,&og)<0)
        {
            /* error; stream version mismatch perhaps */
            ithPrintf("Error reading first page of Ogg bitstream data.\n");
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            goto end;
        }

        if(ogg_stream_packetout(&os,&op)!=1)
        {
            /* no page? must not be vorbis */
            ithPrintf("Error reading initial header packet.\n");
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            goto end;
        }

        if(vorbis_synthesis_headerin(&vi,&vc,&op)<0)
        {
            /* error case; not a vorbis header */
            ithPrintf("This Ogg bitstream does not contain Vorbis audio data.\n");
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            goto end;
        }

        /* At this point, we're sure we're Vorbis. We've set up the logical
        (Ogg) bitstream decoder. Get the comment and codebook headers and
        set up the Vorbis decoder */

        /* The next two packets in order are the comment and codebook headers.
        They're likely large and may span multiple pages. Thus we read
        and submit data until we get our two packets, watching that no
        pages are missing. If a page is missing, error out; losing a
        header page is the only place where missing data is fatal. */

        i=0;
        while(i<2)
        {
            while(i<2)
            {
                int result=ogg_sync_pageout(&oy,&og);
                if(result==0)break; /* Need more data */
                /* Don't complain about missing or corrupt data yet. We'll catch it at the packet output phase */
                if(result==1)
                {
                    ogg_stream_pagein(&os,&og); /* we can ignore any errors here
                                                as they'll also become apparent
                                                at packetout */
                    while(i<2)
                    {
                        result=ogg_stream_packetout(&os,&op);
                        if(result==0)break;
                        if(result<0)
                        {
                            /* Uh oh; data at some point was corrupted or missing! We can't tolerate that in a header. Die. */
                            ithPrintf("Corrupt secondary header.  Exiting.\n");
                            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                            goto end;
                        }
                        result=vorbis_synthesis_headerin(&vi,&vc,&op);
                        if(result<0)
                        {
                            ithPrintf("Corrupt secondary header.  Exiting.\n");
                            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                            goto end;
                        }
                        i++;
                    }
                }
            }
            /* no harm in not checking before adding more */
            buffer=ogg_sync_buffer(&oy,4096);
            // if(buffer==NULL){
                // ithPrintf("!!!buff NULL!!\n",bytes);
                // AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                // goto end;
            // }
            bytes=getFrame(buffer,oggblk,4096);

            if(bytes==0 && i<2)
            {
                ithPrintf("End of file before finding all Vorbis headers!\n");
                AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                goto end;
            }
            ogg_sync_wrote(&oy,bytes);
        }

        /* Throw the comments plus a few lines about the bitstream we're decoding */
        // {
            // char **ptr=vc.user_comments;
            // while(*ptr)
            // {
                // ithPrintf("%s\n",*ptr);
                // ++ptr;
            // }
            // ithPrintf("\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
            // ithPrintf("Encoded by: %s\n\n",vc.vendor);
            
        // }
        // if(DACinit == 0)
        // {
            gCh = vi.channels;
            gSampleRate = vi.rate;
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC);
            // DACinit = 1;
        // }
        convsize=4096/vi.channels;

        /* OK, got and parsed all three headers. Initialize the Vorbis packet->PCM decoder. */
        if(vorbis_synthesis_init(&vd,&vi)==0)
        { /* central decode state */
            vorbis_block_init(&vd,&vb);         /* local state for most of the decode
                                                so multiple block decodes can
                                                proceed in parallel. We could init
                                                multiple vorbis_block structures
                                                for vd here */

            /* The rest is just a straight decode loop until end of stream */
            while(!eos)
            {
                while(!eos)
                {
                    int result=ogg_sync_pageout(&oy,&og);
                    if(result==0)break; /* need more data */
                    if(result<0)
                    {   /* missing or corrupt data at this page position */
                        ithPrintf("Corrupt or missing data in bitstream; continuing...\n");
                        AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                    }
                    else
                    {
                        ogg_stream_pagein(&os,&og); /* can safely ignore errors at this point */
                        while(1)
                        {
                            result=ogg_stream_packetout(&os,&op);

                            if(result==0)break; /* need more data */
                            if(result<0)
                            { 
                                /* missing or corrupt data at this page position */
                                /* no reason to complain; already complained above */
                            }
                            else
                            {
                                /* we have a packet.  Decode it */
                                ogg_int32_t **pcm;
                                int samples;

                                if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
                                {
                                    vorbis_synthesis_blockin(&vd,&vb);
                                }
                                /*
                                **pcm is a multichannel float vector.  In stereo, for
                                example, pcm[0] is left, and pcm[1] is right.  samples is
                                the size of each channel.  Convert the float values
                                (-1.<=range<=1.) to whatever PCM format and write it out */
                                while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0)
                                {
                                    int j;
                                    int bout=(samples<convsize?samples:convsize);

                                    for(i=0;i<vi.channels;i++)
                                    {
                                        ogg_int16_t *ptr=outbuffer+i;
                                        ogg_int32_t *src=pcm[i];
                                        for(j=0;j<bout;j++)
                                        {
                                            *ptr=CLIP_TO_15(src[j]>>9);
                                            ptr+=vi.channels;
                                        }
                                    }
                                    vorbis_synthesis_read(&vd,bout); /* tell libvorbis how
                                                                        many samples we
                                                                        actually consumed */
                                    
                                    while(IsPcmBufFull(2*vi.channels*bout)){//wait some time for pcmWriteBuf reduce; 
                                        SLEEP(128*1000);
                                    }
                                    OutPutPcmData((unsigned char*)outbuffer, 2*vi.channels*bout);//put pcm to buf
                                }
                            }
                        }
                        if(ogg_page_eos(&og))eos=1;
                    }
                }
                if(!eos)
                {
                    buffer=ogg_sync_buffer(&oy,4096);
                    // if(buffer==NULL){
                        // ithPrintf("!!!buff NULL!!\n",bytes);
                        // AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
                        // goto end;
                    // }
                    bytes=getFrame(buffer,oggblk,4096);
                    ogg_sync_wrote(&oy,bytes);
                    if(bytes==0)eos=1;
                }
            }
            /* ogg_page and ogg_packet structs always point to storage in libvorbis.
            They're never freed or manipulated directly */
            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);
        }
        else
        {
            ithPrintf("Error: Corrupt header during playback initialization.\n");
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
        }
end:
        /* clean up this logical bitstream; before exit we see if we're
        followed by another [chained] */
        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);  /* must be called last */
        /* OK, clean up the framer */
        ogg_sync_clear(&oy);

		ClearRdBuffer();
		pcmReadIdx = 0;
		pcmWriteIdx = 0;
        // if(eos)
        // {
            // ithPrintf("Codec Done.\n");
            // AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
            // break;
        // }
    } 

	setAudioReset();
    #if !defined(__FREERTOS__)
    return 0;
    #endif
}

