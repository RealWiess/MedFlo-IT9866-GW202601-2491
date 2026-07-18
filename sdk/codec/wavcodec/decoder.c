#if defined(ENABLE_CODECS_PLUGIN)
#  include "plugin.h"
#endif
#include <string.h>
#include "wav_config.h"
#include "g711.c"

static __inline int ENDIAN_LE32(unsigned char *n) {
    int num = (((unsigned int)n[0]      ) + ((unsigned int)n[1] <<  8) +
               ((unsigned int)n[2] << 16) + ((unsigned int)n[3] << 24) );
    return num;
}

static __inline short ENDIAN_LE16(unsigned char *n) {
    short num = (short)(((unsigned short)n[0]) + ((unsigned short)n[1] << 8));
    return num;
}

/**************************************************************************************
 * Function     : ParseWaveHeader
***************************************************************************************/
int ParseWaveHeader(mblkbuf *m,WaveInfoMode *Info) {
    unsigned char *header;
    int nWaitBytes;
    int done;
    int header_size;

    enum
    {
        INIT_STATE = 0,
        CHK_TYPE   = 1,
        FMT_TYPE   = 2,
        FACT_TYPE  = 3,
        DATA_TYPE  = 4,
        UNKNOW_TYPE= 5,
    } readState;

    nWaitBytes  = 12;    // Wait the wave header
    done        = 0;
    header_size = 0;
    header      = m->b_rptr;
    readState   = INIT_STATE;           

    while(!done)
    {

        switch(readState) {
        case INIT_STATE : if (!(header[ 0] == 'R' && header[ 1] == 'I' && header[ 2] == 'F' && header[ 3] == 'F' &&
                                header[ 8] == 'W' && header[ 9] == 'A' && header[10] == 'V' && header[11] == 'E')) {
                                #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)        
                                ithPrintf("[Wav] Not .wav file \n");
                                AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);        
                                #endif               
                              return 0;
                          }
                          header      += 12;
                          header_size += 12;
                          nWaitBytes   = 8;
                          readState = CHK_TYPE;
                          break;
        case CHK_TYPE   : if (!strncmp(&header[0], "fmt ", 4)) {
                                readState  = FMT_TYPE;
                                nWaitBytes = ENDIAN_LE32(&header[4]);
                          } else if(!strncmp(&header[0], "data", 4)) {
                                readState  = DATA_TYPE;
                                nWaitBytes = 0;
                                done       = 1;
                          } else if(!strncmp(&header[0], "fact", 4)) {
                                readState  = FACT_TYPE;
                                nWaitBytes = ENDIAN_LE32(&header[4]);
                          } else {
                                readState  = UNKNOW_TYPE;
                                nWaitBytes = ENDIAN_LE32(&header[4]);
                            //    #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)        
                            //        ithPrintf("[Wav] skip chunt type '%c%c%c%c'", header[0], header[1], header[2], header[3]);
                            //        AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);        
                            //    #else
                            //  PRINTF("Do not support the chunt type '%c%c%c%c'\n", header[0], header[1], header[2], header[3]);
                            //    #endif
                              //return 0;
                          }
                          header += 8;
                          header_size += (8+nWaitBytes);
                          break;
        case FMT_TYPE   : readState = CHK_TYPE;
                          Info->format         = (WAVE_FORMAT)ENDIAN_LE16(&header[0]);
                          Info->nChans         = (unsigned int)header[2];
                          Info->sampRate       = (unsigned int)ENDIAN_LE32(&header[4]);
                          Info->avgBytesPerSec = (unsigned int)ENDIAN_LE32(&header[8]);
                          Info->blockAlign     = (unsigned int)ENDIAN_LE16(&header[12]);
                          Info->bitsPerSample  = (unsigned int)header[14];
                          if (nWaitBytes > 16) { // extension header
                              Info->samplesPerBlock = (unsigned int)ENDIAN_LE16(&header[18]);
                          } else {
                              Info->samplesPerBlock = 0;
                          }
                          header    += nWaitBytes;
                          nWaitBytes = 8;
                          break;
        case FACT_TYPE  : readState  = CHK_TYPE;
                          header    += nWaitBytes;
                          nWaitBytes = 8;
                          break;
        case UNKNOW_TYPE: readState  = CHK_TYPE;
                          header    += nWaitBytes;
                          nWaitBytes = 8;
                          break;                          
        default         : break;
        }
        
        if(header_size >= 256) {
            #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)        
            ithPrintf("[Wav] header_size is too long (%d byte)",header_size);
            AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);        
            #endif
            return 0;
        }
        
    } // while(!done)

    if (!(Info->format != 0 && Info->nChans >= 1 && Info->nChans <= 6 &&
          Info->sampRate > 0 && Info->sampRate <= 96000 && Info->bitsPerSample >= 4 &&
          Info->bitsPerSample <= 16) ||
        (Info->format == WAVE_FORMAT_DVI_ADPCM && Info->samplesPerBlock == 0)) 
    {
        #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)        
        ithPrintf("[Wav]Unkown .wav parameter.\n");
        AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);        
        #endif                                             

        return 0;
    }

    if (Info->format != WAVE_FORMAT_ALAW && Info->format != WAVE_FORMAT_MULAW &&
        Info->format != WAVE_FORMAT_PCM  && Info->format != WAVE_FORMAT_DVI_ADPCM)
    {
        #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)        
        ithPrintf("[Wav]Unsupport WAV format 0x%x\n",Info->format);
        AudioPluginAPI(SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF);        
        #endif                                             
        return 0;
    }   
    return header_size;
}

int wavDecoder(mblkbuf *m,WaveInfoMode *Info,pcmData *Pcm){
    int header;
    int frame=m->b_wptr-m->b_rptr;
    if(!frame) return 0;
    if(frame>0 && Info->offset==0){
        header=ParseWaveHeader(m,Info);
        Info->offset=header;
        frame-=header;
        m->b_rptr+=header;   
    }
    switch(Info->format){
        case WAVE_FORMAT_PCM  :  frame=pcmDecode(m,Info,Pcm);break;
        case WAVE_FORMAT_ALAW :  frame=alawDecode(m,Info,Pcm);break;
        case WAVE_FORMAT_MULAW:  frame=ulawDecode(m,Info,Pcm);break;
        case WAVE_FORMAT_DVI_ADPCM:break;
        default : break;
    }
    
    return frame;
}