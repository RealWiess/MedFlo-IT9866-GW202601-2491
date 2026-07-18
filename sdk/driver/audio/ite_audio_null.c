
#include "ite/audio.h"
#include "ite/ith.h"
#include "ite/ite_risc.h"

bool    audioKeySoundPaused = false;

int32_t iteAudioPause (bool enable)
{
    (void)enable;
    return 0;
}

int32_t iteAudioStop (void)
{
    return 0;
}

int32_t iteAudioWriteStream (uint8_t * stream, uint32_t length)
{
    (void)stream;
    (void)length;
    return 0;
}

int32_t iteAudioGenWaveDecodeHeader (ITE_WaveInfo * wavInfo, uint8_t * wave_header)
{
    (void)wavInfo;
    (void)wave_header;
    return 0;
}

int32_t iteAudioGetFlashAvailableBufferLength (int32_t nAudioInput, uint32_t * bufferLength)
{
    (void)nAudioInput;
    (void)bufferLength;
    return 0;
}

int32_t iteAudioOpenEngine (ITE_AUDIO_ENGINE audio_type)
{
    (void)audio_type;
    return 0;
}

int32_t iteAudioStopQuick (void)
{
    return 0;
}

int32_t iteAudioWriteWmaInfo (void)
{
    return 0;
}

int32_t iteAudioWriteFlashStream (int32_t nAudioInput, uint8_t * stream, uint32_t length)
{
    (void)nAudioInput;
    (void)stream;
    (void)length;
    return 0;
}

int32_t iteAudioGetAvailableBufferLength (ITE_AUDIO_BUFFER_TYPE bufferType, uint32_t * bufferLength)
{
    (void)bufferType;
    (void)bufferLength;
    return 0;
}

int32_t iteAudioSetFlashInputStatus (int32_t nInputNumber, int32_t nFormat, int32_t nUsing)
{
    (void)nInputNumber;
    (void)nFormat;
    (void)nUsing;
    return 0;
}

int32_t iteAudioStopEngine (void)
{
    return 0;
}

int32_t iteAudioGetDecodeTime (uint32_t * p_time)
{
    (void)p_time;
    return 0;
}

int32_t iteAudioGetAttrib (const ITE_AUDIO_ATTRIB attrib, void * value)
{
    (void)attrib;
    (void)value;
    return 0;
}

int32_t iteAudioSetAttrib (const ITE_AUDIO_ATTRIB attrib, const void * value)
{
    (void)attrib;
    (void)value;
    return 0;
}

uint32_t iteAecCommand (uint16_t command, uint32_t param0, uint32_t param1, uint32_t param2, uint32_t param3,
                        uint32_t * value)
{
    (void)command;
    (void)param0;
    (void)param1;
    (void)param2;
    (void)param3;
    (void)value;
    return 0;
}

bool ithCodecCommand (int32_t command, int32_t parameter0, int32_t parameter1, int32_t parameter2)
{
    (void)command;
    (void)parameter0;
    (void)parameter1;
    (void)parameter2;
    return true;
}

void ithCodecCtrlBoardWrite (uint8_t * data, int32_t length)
{
    (void)data;
    (void)length;
}

void ithCodecCtrlBoardRead (uint8_t * data, int32_t length)
{
    (void)data;
    (void)length;
}

void ithCodecHeartBeatRead (uint8_t * data, int32_t length)
{
    (void)data;
    (void)length;
}

int32_t iteRiscOpenEngine (ITE_RISC_ENGINE engine_type, uint32_t bootmode)
{
    (void)engine_type;
    (void)bootmode;
    return 0;
}

int32_t iteRiscTerminateEngine (void)
{
    return 0;
}

int32_t iteAudioGetMusicCodecDump (void)
{
    return 0;
}

void iteAudioSetMusicCodecDump (int32_t nEnable)
{
    (void)nEnable;
}

void iteAudioCodecSetPcmIdx (int32_t index)
{
    (void)index;
}

int32_t iteAudioCodecGetPcmIdx (void)
{
    return 0;
}
