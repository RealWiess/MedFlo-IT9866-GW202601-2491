#include "ite/audio.h"
#include "ite/main_processor_message_queue.h"
#include "i2s/i2s.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define TOINT(n)    n
#define TOSHORT(n)  n

int32_t
ExcuteI2SInitDac (
    void)
{
    uint8_t         * I2SBuf;
    int32_t         nch;
    int32_t         rate;
    int32_t         bufflen;
    int32_t         nTemp = 1;
    uint32_t        * pBuf;
    uint32_t        nLength;
    STRC_I2S_SPEC   spec;
#if defined(__OPENRTOS__)

    pBuf    = (uint32_t *)iteAudioGetAudioCodecAPIBuffer(&nLength);
    I2SBuf  = (uint8_t *)(TOINT(pBuf[0]) + iteAudioGetAudioCodecBufferBaseAddress());
    nch     = TOINT(pBuf[1]);
    rate    = TOINT(pBuf[2]);
    bufflen = TOINT(pBuf[3]);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_SAMPLE_RATE, &rate);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_CHANNEL, &nch);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_BUFFER_LENGTH, &bufflen);
    iteAudioSetAttrib(ITE_AUDIO_I2S_PTR, I2SBuf);
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &nTemp);

    (void)printf("[Main Processor Message Queue]sdl  I2SInitDac 0x%x  %d %d %d \n", (uint32_t)I2SBuf, nch, rate, bufflen);
    if ((nch > 2) || (nch <= 0))
    {
        return 0;
    }

    if ((rate > 48000) || (rate < 8000))
    {
        return 0;
    }

    if (iteAudioGetMusicCodecDump())
    {
        return 0;
    }

    /* init I2S */
    if (i2s_get_DA_running())
    {
        i2s_deinit_DAC();
    }

    memset(&spec, 0, sizeof(spec));
    spec.sample_rate    = rate;
    spec.channels       = nch;
    spec.sample_size    = 16;
    spec.base_i2s       = I2SBuf;
    spec.buffer_size    = bufflen;
    spec.is_big_endian  = false;
    spec.enable_Speaker = 1;

    /*hdmi*/
    spec.num_hdmi_audio_buffer  = 1;
    spec.base_hdmi[0]           = I2SBuf;
    spec.base_hdmi[1]           = I2SBuf;
    spec.base_hdmi[2]           = I2SBuf;
    spec.base_hdmi[3]           = I2SBuf;

    i2s_init_DAC(&spec);

#endif
    return 0;
}

int32_t
ExcutePauseDAC (
    void)
{
    return 0;
}

int32_t
ExcuteDeactiveDAC (
    void)
{
    if (iteAudioGetMusicCodecDump())
    {
        return 0;
    }
#if defined(__OPENRTOS__)
    i2s_deinit_DAC();
#endif
    (void)printf("[Main Processor Message Queue] deactiveDAC \n");
    return 0;
}

int32_t
ExcutePrintf (
    void)
{
    uint8_t     * pBuf;
    uint32_t    nLength;

#if defined(__OPENRTOS__)
    pBuf = (uint8_t *)iteAudioGetAudioCodecAPIBuffer(&nLength);
    puts((const char *)pBuf);
    memset(pBuf, 0, nLength);

    ithFlushDCacheRange((void *)pBuf, nLength);
    ithFlushMemBuffer();
#endif
    return 0;
}

int32_t
ExcuteSpectrum (
    void)
{
    return 0;
}

int32_t
ExcutePcmIdx (
    void)
{
    uint32_t    * pBuf;
    uint32_t    nLength;
    int32_t     nTemp;

#if defined(__OPENRTOS__)

    pBuf    = (uint32_t *)iteAudioGetAudioCodecAPIBuffer(&nLength);
    nTemp   = TOINT(pBuf[0]);
    iteAudioCodecSetPcmIdx(nTemp);
    // (void)printf("nTemp=%d\n",nTemp);
#endif
    return 0;
}

int32_t
ExcuteAudioPluginCmd (
    uint16_t RegStatus)
{
    uint32_t    nTemp;
    int32_t     nResult = 0;

    // ithInvalidateDCache();

    // execute audio plugin cmd
    nTemp = RegStatus & 0x3fff;
    // (void)printf("[Main Processor] cmd  %d\n",nTemp);
    switch (nTemp)
    {
        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC:
            nResult = ExcuteI2SInitDac();
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_PAUSE_DAC:
            nResult = ExcutePauseDAC();
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_DEACTIVE_DAC:
            nResult = ExcuteDeactiveDAC();
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF:
            nResult = ExcutePrintf();
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_SPECTRUM:
            nResult = ExcuteSpectrum();
            break;

        case SMTK_AUDIO_PLUGIN_CMD_ID_PCMIDX:
            nResult = ExcutePcmIdx();       // dump to filter (flower OR
                                            // mediastreamer filter)
            break;

        default:
            break;
    }
    return nResult;
}

int32_t
smtkMainProcessorExecuteAudioPluginCmd (
    uint16_t nRegStatus)
{
    int32_t     nResult = 0;
    uint16_t    reg;

    nResult = ExcuteAudioPluginCmd(nRegStatus);
    reg     = SMTK_MAIN_PROCESSOR_ID << 14 | nResult;

    setAudioPluginStatus(reg);

    return nResult;
}
