/***************************************************************************
 * Copyright ITE Tech. Inc. All Rights Reserved.
 *
 * @file
 * mp3 decoder 
 *
 * @author 
 * @version (ffmpeg version)
 * @date 2022/11/01
 *
 ***************************************************************************/

#include "coder.h"
#include "mp3decode.h"

#if defined(__OR32__)
#  include "mmio.h"
#  include "i2s.h"
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
//#define FRAME_SIZE              (512)   // Number of sample per frame
//#define MAX_BYTES_PER_SAMPLE    (4)
//#define PCM_BIT_PERSAMPLE       (16)


#if READBUF_SIZE > 65535
#  error "The buffer exceed 64K bytes."
#endif

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
static unsigned int* gtAudioPluginBufferLength;
static unsigned short* gtAudioPluginBuffer;
//static int gnTemp;
//int gPause = 0 ;
int gPrint = 0;
unsigned char *gBuf;
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
#endif        
///////////////////////////////////////////////////////////////////////////
//                              Local Variable
//
// PCM -> 16bits PCM data to/from I2S
// WAV -> Stream data to/from driver
//
///////////////////////////////////////////////////////////////////////////
#if defined(__GNUC__)
static unsigned char streamBuf[READBUF_SIZE] __attribute__ ((aligned(16), section (".sbss")));//16384
static unsigned char pcmWriteBuf[I2SBUFSIZE] __attribute__ ((aligned(16), section (".sbss")));//27648
#else
static unsigned char streamBuf[READBUF_SIZE];
static unsigned char pcmWriteBuf[I2SBUFSIZE];
#endif // defined(__GNUC__)


static unsigned int  pcmReadIdx;
static unsigned int  pcmWriteIdx;

int streamLen = sizeof(streamBuf);
static int streamRdPtr = 0;
static int streamWrPtr = 0;

static int gCh = 2;
static int gSampleRate = 44100;
static int DACinit = 0;

///////////////////////////////////////////////////////////////////////////
//                              Function Decleration
///////////////////////////////////////////////////////////////////////////
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define SLEEP(x)  {for(int i=0;i<x;i++) asm("");}

/*get memory block buf availe size*/
static int mblk_get_avail_size(mblkbuf *m){
    int size;
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

static unsigned int getStreamWrPtr(void) 
{
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
	int nbytes = (streamRdPtr <= streamWrPtr) ? (streamWrPtr - streamRdPtr) : (streamLen - (streamRdPtr - streamWrPtr));
	
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

void ClearRdBuffer(void)
{
	MMIO_Write(DrvDecode_WrPtr, 0);
    MMIO_Write(DrvDecode_RdPtr, 0);
	streamWrPtr = 0;
    streamRdPtr = 0;
	MMIO_Write(DrvAudioCtrl, MMIO_Read(DrvAudioCtrl) & ~DrvDecode_STOP);
	#if defined(__OPENRTOS__)
    dc_invalidate(); // Flush DC Cache
    #endif
}

/*check pcm buf full or not 1:pcm free buf < len  0:pcm free buf > len*/
int IsPcmBufFull(int len)
{
	int freebytes;
	if(isMusicCodecDump())
		pcmReadIdx=MMIO_Read(DrvDumpPCM_RdPtr);
	else
		pcmReadIdx = CODEC_I2S_GET_OUTRD_PTR();
	
	freebytes = pcmReadIdx - pcmWriteIdx;
    if (freebytes <= 0)
    {
        freebytes += sizeof(pcmWriteBuf);
    }
	if(freebytes < len + 2)
		return 1;
	else
		return 0;
}

/*put pcm data to pcmWriteBuf & update rwpoint*/
void OutPutPcmData(uint8_t* databuf, int len)
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
    #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
	if(isMusicCodecDump())
		AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PCMIDX);//update rwp
	else
		CODEC_I2S_SET_OUTWR_PTR(pcmWriteIdx);//put to i2s buf and update rwp
    #endif
	MMIO_Write(AUDIO_PROCESSOR_WRITE_REGISTER_PROTECTION, 0);

}



/**************************************************************************************
 * Function     : AudioPluginAPI
 *
 * Description  : AudioPluginAPI
 *
 * Input        : plugin type
 *
 * Output       : None
 *
 * Note         :
 **************************************************************************************/
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
                        va_arg(va, unsigned int);
                    }
                    else if (control_char == 'd')
                        x = va_arg(va, long);
                    else
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
    }while(i);


}
#endif // defined(AUDIO_PLUGIN_MESSAGE_QUEUE)

/**************************************************************************************
 * Function     : main
 *
 * Description  :
 *
 * Inputs       :
 *
 * Outputs      :
 *
 * Return       :
 *
 * TODO         :
 *
 * Note         :
 *
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

#endif

    #if defined(WIN32) || defined(__CYGWIN__)
    GetParam(argc, argv);
    win32_init();
    #endif // defined(WIN32) || defined(__CYGWIN__)
    ithPrintf("[MP3] ffmpeg decoder");
    #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
    AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);
    #endif
    for (;;)
    {  // Forever Loop
        AVCodecContext gMpegAudioContext;
        AVFrame        gFrame;
		mblkbuf        blk,*mp3blk=&blk;
		unsigned char  buf[READBUF_SIZE];
        /*mp3blk init*/
        memset(buf, 0, READBUF_SIZE);
        mblk_init(mp3blk, buf, READBUF_SIZE);
        
        /*mp3 decoder parm*/
		memset(&gMpegAudioContext, 0, sizeof(gMpegAudioContext));
		decode_init(&gMpegAudioContext);
        /*pcmbuf reset*/

		while(1){
            int got_picture = 0;
            int readbytes   = 0;
            memset(&gFrame, 0, sizeof(gFrame));
            readbytes = decode_frame(&gMpegAudioContext, &gFrame, &got_picture, mp3blk->b_rptr, mblk_get_avail_size(mp3blk));
			
            if(got_picture){
                int len = gFrame.linesize[0];//4608

                if(DACinit == 0){
                    gCh = gMpegAudioContext.channels;
					gSampleRate = gMpegAudioContext.sample_rate;
                    #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
					AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC);
					#endif
                    DACinit = 1;
                }

                while(IsPcmBufFull(len)){//wait some time for pcmWriteBuf reduce;
                    SLEEP(128*1000);
                }
                OutPutPcmData(gFrame.data[0], len);//put pcm to buf

                if(readbytes>=0){
                    mp3blk->b_rptr+=readbytes;
                }
			}else{
                int ReadSize,empty;
                if(mblk_get_avail_size(mp3blk)==READBUF_SIZE) mp3blk->b_rptr+=2; //skip 2 bytes data while mp3 decode fail.
                mblk_buf_rearrange(mp3blk);
                empty = READBUF_SIZE-mblk_get_avail_size(mp3blk);//get mp3 buf empty size
                ReadSize= DecFillReadBuffer(mp3blk, empty);//fill mp3 buf from streambuf
            }

            SLEEP(128*1000);//wait;

			if(isSTOP())
			{
				decode_deinit(&gMpegAudioContext);
				break;
			}

		}
		ClearRdBuffer();
		pcmReadIdx = 0;
		pcmWriteIdx = 0;

    } /* end of forever loop */
	setAudioReset();
#if !defined(__FREERTOS__)
    return 0;
#endif
}

