/*
 * Copyright (c) 2004 ITE Corp. All Rights Reserved.
 */
/** @file
 *  Co-processor API functoin file.
 *      Date: 2005/09/30
 *
 */
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "ite/audio.h"
#include "ite/main_processor_message_queue.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "ite/ite_risc.h"
#include "i2s/i2s.h"

#define MESSAGE_SIZE                    (32 * 1024)

#define TOINT(n)                        (n)
#define TOSHORT(n)                      (n)

#define CODEC_BASE                      ((int32_t)iteRiscGetTargetMemAddress(RISC1_IMAGE_MEM_TARGET))
#define CODEC_REMAP_OFFSET              CODEC_BASE

#include "mmio.h"

#define ITE_AUDIO_READ_REG              ithReadRegA
#define ITE_AUDIO_WRITE_REG             ithWriteRegA
#define ITE_AUDIO_WRITE_REG_MASK        ithWriteRegMaskA

#ifndef MAX
    #define MAX(a, b)                   (((a) > (b)) ? (a) : (b))
#endif

#define AUDIO_WRAP_AROUND_THRESHOLD     (0x3E80000) // 65536 seconds
#define AUDIO_WRAP_AROUND_JUDGE_RANGE   (0x1F40000) // 36768 seconds

// #define AUDIO_DRIVER_SHOW_WRITE_BITRATE

#define HAVE_MP3                        (true)
#define HAVE_AAC                        (true)
#define HAVE_AC3                        (true)      // AC3 decode cannot work
#define HAVE_WMA                        (true)
#define HAVE_AMR                        (true)
#define HAVE_WAV                        (true)
#define HAVE_FLAC                       (true)
#define HAVE_SBC                        (true)
#define HAVE_MP2ENC                     (true)
#define HAVE_AACENC                     (true)
#define HAVE_OPUS                       (true)
#define HAVE_BSAC                       (false)
#define HAVE_MIDI                       (false)
#define HAVE_SPDIF_AC3                  (false)
#define HAVE_MIXER                      (false)
#define HAVE_PCM                        (false)
#define HAVE_OGG                        (true)

// #define CODEC_STREAM_START_ADDRESS 0x00001000+0x14

static uint32_t pcmidx;
typedef void * TaskHandle_t;

#include "openrtos/portmacro.h"
#include "openrtos/projdefs.h"

bool                            audioKeySoundPaused;
extern struct codec_api_t       codec_api;

static pthread_mutex_t          gp_mutex_ = PTHREAD_MUTEX_INITIALIZER;
static struct codec_header_t    * gp_header_;
static int32_t                  codec_start_addr;
static int32_t                  codec_end_addr;
#if defined(ENABLE_CODECS_ARRAY)
extern int32_t                  * codecptr[];
#endif

static uint32_t gPtsTimeBaseOffset;
static uint32_t gLastDecTime;

// =============================================================================
//                              Macro Definition
// =============================================================================
#define BufThreshold    64

#define LOG_ERROR
#define LOG_ENTER
#define LOG_END         ;

// =============================================================================
//                              Global Data Definition
// =============================================================================
static uint32_t         Audio_DefDecode_Bufptr;     // default decode buffer address
static uint32_t         Audio_Decode_Bufptr;        // decode buffer address
static uint32_t         Audio_Decode_Buflen;        // decode buffer length
static uint32_t         Audio_Decode_Rdptr;         // decode buffer read  port address
static uint32_t         Audio_Decode_Wrptr;         // decode buffer write port address

static uint32_t         Audio_DefEcho_Bufptr;       // default echo buffer address
static uint32_t         Audio_Echo_Bufptr;          // echo buffer address
static uint32_t         Audio_Echo_Buflen;          // echo buffer length

static uint32_t         Audio_DefRef_Bufptr;        // default reference buffer address
static uint32_t         Audio_Ref_Bufptr;           // reference buffer address
static uint32_t         Audio_Ref_Buflen;           // reference buffer length

static uint32_t         Audio_DefEchocc_Bufptr;     // default echo cancellation buffer address
static uint32_t         Audio_Echocc_Bufptr;        // echo cancellation buffer address
static uint32_t         Audio_Echocc_Buflen;        // echo cancellation buffer length

static uint32_t         Audio_DefParameter_Cmdptr;  // default command buffer address
static uint32_t         Audio_Parameter_Cmdptr;     // command buffer address
static uint32_t         Audio_Parameter_Cmdlen;     // command buffer length

static bool             AUD_SKIP;
static uint32_t         g_audio_base_address_;

static bool             gb_is_audio_inited_;
static ITE_AUDIO_ENGINE g_audio_engine_type_ = 0;       // engine type now
static ITE_AUDIO_ENGINE g_audio_plugin_type_;       // engine type now

static uint32_t         AUD_decSampleRate;
static uint32_t         AUD_decChannel;
static uint32_t         AUD_nCodecSampleRate;       // audio codec set sample rate
static uint32_t         AUD_nCodecChannels;         // audio codec set channels
static uint32_t         AUD_nCodecPCMBufferLength;  // audio codec set channels
static bool             AUD_bI2SInit;
static char             * AUD_pI2Sptr;
static bool             AUD_bMPlayer;
static bool             AUD_bMPlayerSTCReady;
static bool             AUD_bDecodeError;
static bool             AUD_bEnableSTC_Sync;
static uint32_t         AUD_nFfmpegPauseAudio;
static uint32_t         AUD_nDropData;
static uint32_t         AUD_nPauseSTCTime;

// static uint8_t          wave_header[48];
static uint32_t wave_ADPCM;

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
static int16_t              * gp_audio_plugin_message_buffer_;
static uint32_t             g_audio_plugin_message_buffer_length_;      // MESSAGE_SIZE / sizeof(int16_t);
static AUDIO_CODEC_STREAM   * gp_audio_stream_;                         // (AUDIO_CODEC_STREAM *)&gBuf[RISC1_SIZE + CODEC_EX_SIZE + MESSAGE_SIZE];
#endif

// #define decVolLevel 0x0101

// static uint32_t  nInputCount=0;

// #ifndef CFG_VIDEO_ENABLE
////#define DRIVER_CODEC
// #define CFG_COMPRESS_AUDIO_PLUGIN
// #else
#define DRIVER_CODEC
// #define CFG_COMPRESS_AUDIO_PLUGIN
// #endif

#if defined(DRIVER_CODEC) && !defined(CFG_COMPRESS_AUDIO_PLUGIN)

    #ifdef CFG_AUDIO_CODEC_MP3DEC
static uint8_t g_mp3_decoder_[] = {
        #include "mp3.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_WAV
static uint8_t g_wav_decoder_[] = {
        #include "wave.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_WMADEC
static uint8_t g_wma_decoder_[] = {
        #include "wma.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_AACDEC
static uint8_t g_aac_decoder_[] = {
        #include "aac.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_AMR
static uint8_t g_amr_decoder_[] = {
        #include "amr.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_EAC3DEC
static uint8_t g_ac3_decoder_[] = {
        #include "eac3.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_FLACDEC
static uint8_t g_flac_decoder_[] = {
        #include "flac.hex"
};
    #endif
    #ifdef CFG_AUDIO_CODEC_SBC
static uint8_t g_sbc_codec_[] = {
        #include "sbc.hex"
};
    #endif
    #ifdef CFG_AUDIO_CODEC_MP2ENC
static uint8_t g_mp2_encoder_[] = {
        #include "mp2enc.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_AACENC
static uint8_t g_aac_encoder_[] = {
        #include "aacenc.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_VORBISDEC
static uint8_t g_vorbis_decoder_[] = {
        #include "vorbis.hex"
};
    #endif

    #ifdef CFG_AUDIO_CODEC_OPUSDEC
static uint8_t g_opus_decoder_[] = {
        #include "opus.hex"
};
    #endif

#endif

// =============================================================================
//                  Private Function Declaration
// =============================================================================
static int32_t
audio_load_engine_ (
    ITE_AUDIO_ENGINE audio_type);

static int32_t
audio_reset_engine_ (
    void);

static int32_t
audio_config_engine_ (
    void);

static int32_t
audio_reset_audio_engine_type_ (
    void);

#ifdef ENABLE_AUDIO_PROCESSOR
static void
audio_reset_audio_processor_ (
    void);

static void
audio_fire_audio_processor_ (
    void);
#endif

static int32_t
audio_get_write_available_buffer_length (
    uint32_t * p_wr_buf_len);

int32_t
audio_initial_codec_ (
    void);

// =============================================================================
//                              Function Definition
// =============================================================================
#ifdef ENABLE_AUDIO_PROCESSOR

static void
audio_reset_audio_processor_ (
    void)
{
    uint16_t    reg     = 0U;
    int32_t     timeout = 0;

    if (g_audio_engine_type_ == ITE_RESERVED)
    {
        return;
    }

    if (i2s_get_DA_running() && (g_audio_engine_type_ != ITE_SBC_CODEC) && !iteAudioGetMusicCodecDump() && (g_audio_engine_type_ != 0))
    {
        do
        {
            // wait music codedec into wait state :see getStreamWrPtr(void)
            ITE_AUDIO_WRITE_REG(DrvDecode_WrPtr, 0xFFFF);
            (void)usleep(1000);
            reg = ITE_AUDIO_READ_REG(DrvAudioCtrl2);
            timeout++;
            if (timeout > 1000)
            {
                (void)printf("DrvAudio_RESET timeout\n");
                break;
            }
        } while (!(reg & DrvAudio_RESET));
    }
    else
    {
        for (int32_t i = 0; i < 64000; i++)
        {
            asm ("");
        }
    }
    iteRiscResetCpu(RISC1_CPU);

    ITE_AUDIO_WRITE_REG(DrvDecode_WrPtr, 0x0);
    ITE_AUDIO_WRITE_REG(DrvDecode_RdPtr, 0x0);
    ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl2, (0 << DrvAudio_Reset_Bits), (1 << DrvAudio_Reset_Bits));
    audio_reset_audio_engine_type_();
}

static void
audio_fire_audio_processor_ (
    void)
{
    iteRiscFireCpu(RISC1_CPU);
    ITE_AUDIO_WRITE_REG(AUDIO_DECODER_START_FALG, 0);
}
#endif // ENABLE_AUDIO_PROCESSOR

// stream
int32_t
audio_get_write_available_buffer_length (
    uint32_t * p_wr_buf_len)
{
    uint32_t    hwpr;
    uint32_t    swpr;

    swpr    = Audio_Decode_Wrptr;

    hwpr    = ITE_AUDIO_READ_REG(DrvDecode_RdPtr);

    // Get the avalible length of buffer
    if (swpr == hwpr)
    {
        (*p_wr_buf_len) = Audio_Decode_Buflen - 2;
    }
    else if (swpr > hwpr)
    {
        (*p_wr_buf_len) = Audio_Decode_Buflen - (swpr - hwpr) - 2;
    }
    else
    {
        (*p_wr_buf_len) = hwpr - swpr - 2;
    }

    return 0;
}

int32_t
audio_initial_codec_ (
    void)
{
    /*
       audio codec init : sdk\driver\itp\dac\itp_dac_xxxx.c
       DA :void itp_codec_playback_init()
       AD :void itp_codec_rec_init()
     */

    return 0; // AUD_SetRISCIIS();
}

int32_t
AUD_CheckProcessor ()
{
    uint16_t nTemp, nCount;

    nCount = 0U;
    do
    {
        nTemp = ITE_AUDIO_READ_REG(AUDIO_DECODER_START_FALG);
        ithDelay(1);
        nCount++;
    } while (nTemp == 0U && nCount <= 20000U);

    if (nCount > 20000U)
    {
        (void)printf("[Audio Driver] wait decoder start fail #line %d\n",
                     __LINE__);
    }
    return 0;
}

int32_t
audio_load_engine_ (
    ITE_AUDIO_ENGINE audio_type)
{
    AUD_SKIP            = false;
    Audio_Decode_Wrptr  = 0;
    Audio_Decode_Rdptr  = 0;

    if (audio_reset_engine_() != 0)
    {
        return AUDIO_ERROR_ENGINE_RESET_ENGINE_ERROR;
    }

#if defined(ENABLE_CODECS_PLUGIN) && !defined(ENABLE_CODECS_ARRAY)
    // Load Plug-Ins to memory
    if (1) // (g_audio_plugin_type_ != audio_type)
    {
        // signed portBASE_TYPE ret = pdFAIL;

        switch (audio_type)
        {
            case ITE_MP3_DECODE:
                AUD_SKIP = !HAVE_MP3;
                break;

            case ITE_WMA_DECODE:
                AUD_SKIP = !HAVE_WMA;
                break;

            case ITE_AAC_DECODE:
                AUD_SKIP = !HAVE_AAC;
                break;

            case ITE_OGG_DECODE:
                AUD_SKIP = !HAVE_OGG;
                break;

            case ITE_AC3_DECODE:
            case ITE_AC3_SPDIF_DECODE:
                break;

            case ITE_AMR_CODEC:
                AUD_SKIP = !HAVE_AMR;
                break;

            case ITE_BSAC_DECODE:
                AUD_SKIP = !HAVE_BSAC;
                break;

            case ITE_WAV_DECODE:
                AUD_SKIP = !HAVE_WAV;
                break;

            case ITE_FLAC_DECODE:
                AUD_SKIP = !HAVE_FLAC;
                break;

            case ITE_SBC_CODEC:
                AUD_SKIP = !HAVE_SBC;
                break;

            case ITE_MP2_ENCODE:
                AUD_SKIP = !HAVE_MP2ENC;
                break;

            case ITE_AAC_ENCODE:
                AUD_SKIP = !HAVE_AACENC;
                break;

            case ITE_OPUS_DECODE:
                AUD_SKIP = !HAVE_OPUS;
                break;

            case EXTERNAL_ENGINE_PLUGIN:
                break;

            default:
                return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
                break;
        }
        if (AUD_SKIP)
        {
            return 0;
        }

        {
            codec_start_addr    = (CODEC_BASE + CODEC_START_ADDR);
            gp_header_            = (struct codec_header_t *)(CODEC_BASE + CODEC_START_ADDR);
            codec_end_addr      = CODEC_BASE + CODEC_START_ADDR + RISC1_IMAGE_SIZE;
            if ((TOINT(gp_header_->magic) != (CODEC_MAGIC)) || (TOSHORT(gp_header_->target_id) != (TARGET_ID)) ||
                (TOSHORT(gp_header_->api_version) < (CODEC_API_VERSION)) ||
                (TOINT((int32_t)gp_header_->load_addr) != (codec_start_addr - CODEC_REMAP_OFFSET)) ||
                (TOINT((int32_t)gp_header_->end_addr) > (codec_end_addr - CODEC_REMAP_OFFSET)))
            {
                (void)printf("[Audio Driver]CODEC header error\n");
                (void)printf(" gp_header_->magic       0x%08x, expect value = 0x%08x  %d\n", (uint32_t)TOINT(gp_header_->magic),
                       (uint32_t)(CODEC_MAGIC), TOINT(gp_header_->magic) != (CODEC_MAGIC));
                (void)printf(" gp_header_->target_id   0x%04x, expect value = 0x%04x  %d\n", TOSHORT(gp_header_->target_id),
                       (TARGET_ID), TOSHORT(gp_header_->target_id) != (TARGET_ID));
                (void)printf(" gp_header_->api_version 0x%04x, expect value < 0x%04x %d\n", TOSHORT(gp_header_->api_version),
                       (CODEC_API_VERSION), TOSHORT(gp_header_->api_version) < (CODEC_API_VERSION));
                (void)printf(" gp_header_->load_addr   0x%08x, expect value = 0x%08x %d\n", TOINT((int32_t)gp_header_->load_addr),
                       (codec_start_addr - CODEC_REMAP_OFFSET),
                       TOINT((int32_t)gp_header_->load_addr) != (codec_start_addr - CODEC_REMAP_OFFSET));
                (void)printf(" gp_header_->end_addr    0x%08x, expect value < 0x%08x %d\n", TOINT((int32_t)gp_header_->end_addr),
                       (codec_end_addr - CODEC_REMAP_OFFSET),
                       TOINT((int32_t)gp_header_->end_addr) > (codec_end_addr - CODEC_REMAP_OFFSET));
                AUD_SKIP = true;
                return AUDIO_ERROR_CODEC_DO_NOT_INITIALIZE;
            }
        }

    #if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
        (void)printf("[Audio Driver] load_addr 0x%08x end_addr 0x%08x stream 0x%08x \n", TOINT((int32_t)gp_header_->load_addr),
               TOINT((int32_t)gp_header_->end_addr), TOINT((int32_t)gp_header_->pStream));
        // write audio stream structure
        gp_audio_stream_                            = (AUDIO_CODEC_STREAM *)iteRiscGetTargetMemAddress(SHARE_MEM1_TARGET);
        gp_header_->pStream                           = (uint8_t *)TOINT((int32_t)gp_audio_stream_ - CODEC_REMAP_OFFSET);
        gp_audio_stream_->codecAudioPluginBuf       = TOINT((int32_t)gp_audio_plugin_message_buffer_ - CODEC_REMAP_OFFSET);
        gp_audio_stream_->codecAudioPluginLength    = TOINT(g_audio_plugin_message_buffer_length_);

        if ((int32_t)gp_audio_plugin_message_buffer_ < CODEC_BASE)
        {
            (void)printf("gp_audio_plugin_message_buffer_ %p < CODEC_BASE 0x%x\n", gp_audio_plugin_message_buffer_,
                   (uint32_t)CODEC_BASE);
            AUD_SKIP = true;
            return AUDIO_ERROR_CODEC_DO_NOT_INITIALIZE;
        }
    #endif // defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
        //  Flush Instruction Cacahe

        // Restart Task

        ithFlushDCacheRange((void *)gp_audio_stream_, SHARE_MEM1_SIZE);
        ithFlushDCacheRange((void *)codec_start_addr, codec_end_addr - codec_start_addr);
        ithFlushMemBuffer();

    #ifdef ENABLE_AUDIO_PROCESSOR
        audio_fire_audio_processor_();
        // ret = 1;
    #endif
        /*if (pdFAIL == ret)
           {
            LOG_ERROR "Failed to create audio task\n" LOG_END
            AUD_SKIP = true;
            return AUDIO_ERROR_ENGINE_LOAD_FAIL_ERROR;
           }*/
        g_audio_plugin_type_ = audio_type;
    }    // g_audio_plugin_type_ != audio_type
    else // g_audio_plugin_type_ == audio_type
    {
        // Resume the task that stores in the memory
    #ifdef ENABLE_AUDIO_PROCESSOR
        audio_reset_audio_processor_();
        audio_fire_audio_processor_();
    #endif
    }
#elif defined(ENABLE_CODECS_PLUGIN) && defined(ENABLE_CODECS_ARRAY)
    switch (audio_type)
    {
        case ITE_MP3_DECODE:
            AUD_SKIP = !HAVE_MP3;
            break;

        case ITE_WMA_DECODE:
            AUD_SKIP = !HAVE_WMA;
            break;

        case ITE_AAC_DECODE:
            AUD_SKIP = !HAVE_AAC;
            break;

        case ITE_OGG_DECODE:
            AUD_SKIP = !HAVE_OGG;
            break;

        case ITE_AC3_DECODE:
            // AUD_SKIP = !HAVE_AC3;
            break;

        case ITE_AMR_CODEC:
            AUD_SKIP = !HAVE_AMR;
            break;

        case ITE_BSAC_DECODE:
            AUD_SKIP = !HAVE_BSAC;
            break;

        case ITE_MIDI:
            AUD_SKIP = !HAVE_MIDI;
            break;

        case ITE_WAV_DECODE:
            AUD_SKIP = !HAVE_WAV;
            break;

        case ITE_AC3_SPDIF_DECODE:
            break;

        case ITE_MP2_ENCODE:
            AUD_SKIP = !HAVE_MP2ENC;
            break;

        case ITE_AAC_ENCODE:
            AUD_SKIP = !HAVE_AACENC;
            break;

        case ITE_OPUS_DECODE:
            AUD_SKIP = !HAVE_OPUS;
            break;

        default:
            return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
            break;
    }
    if (AUD_SKIP)
    {
        return 0;
    }

    // Load Plug-Ins to memory
    if ((g_audio_plugin_type_ == 0) && (audio_type == ITE_MP3_DECODE))
    {
        // The MP3 CODEC had already in memory on initial state.
        if ((gp_header_->magic != CODEC_MAGIC) || (gp_header_->target_id != TARGET_ID) ||
            (gp_header_->api_version < CODEC_API_VERSION) || ((int32_t)gp_header_->load_addr != (int32_t)&codec_start_addr) ||
            ((int32_t)gp_header_->end_addr > (int32_t)&codec_end_addr))
        {
            (void)printf("CODEC header error\n");
            (void)printf(" gp_header_->magic       0x%08x, expect value = 0x%08x\n", gp_header_->magic, CODEC_MAGIC);
            (void)printf(" gp_header_->target_id   0x%04x, expect value = 0x%04x\n", gp_header_->target_id, TARGET_ID);
            (void)printf(" gp_header_->api_version 0x%04x, expect value < 0x%04x\n", gp_header_->api_version, CODEC_API_VERSION);
            (void)printf(" gp_header_->load_addr   0x%08x, expect value = 0x%08x\n", gp_header_->load_addr,
                   (int32_t)&codec_start_addr);
            (void)printf(" gp_header_->end_addr    0x%08x, expect value < 0x%08x\n", gp_header_->end_addr,
                   (int32_t)&codec_end_addr);
            AUD_SKIP = true;
            return int32_t_ERROR;
        }
        g_audio_plugin_type_ = audio_type;
    #ifdef ENABLE_AUDIO_PROCESSOR
        audio_reset_audio_processor_();
        audio_fire_audio_processor_();
    #endif
    }
    else if (g_audio_plugin_type_ != audio_type)
    {
        signed portBASE_TYPE ret = pdFAIL;
        if (codecptr[audio_type] != (int32_t *)codecbuf)
        {
            int32_t i, tmp;
            int32_t * src, * dst;

    #ifdef ENABLE_AUDIO_PROCESSOR
            audio_reset_audio_processor_(); // Stop CPU
    #endif

            // SWAP the CODEC
            dst = (int32_t *)codecbuf;
            src = codecptr[audio_type];
            for (i = 0; i < CODEC_ARRAY_SIZE / sizeof(int32_t); i++)
            {
                tmp     = dst[i];
                dst[i]  = src[i];
                src[i]  = tmp;
            }

            // swap the CODEC pointer
            tmp                             = (int32_t)codecptr[audio_type];
            codecptr[audio_type]            = codecptr[g_audio_plugin_type_];
            codecptr[g_audio_plugin_type_]  = (int32_t *)tmp;

            // Flush Instruction Cache
    #ifdef __OR32__
            ic_invalidate();
    #endif
        }
        if ((gp_header_->magic != CODEC_MAGIC) || (gp_header_->target_id != TARGET_ID) ||
            (gp_header_->api_version < CODEC_API_VERSION) || ((int32_t)gp_header_->load_addr != (int32_t)&codec_start_addr) ||
            ((int32_t)gp_header_->end_addr > (int32_t)&codec_end_addr))
        {
            (void)printf("CODEC header error\n");
            (void)printf(" gp_header_->magic       0x%08x, expect value = 0x%08x\n", gp_header_->magic, CODEC_MAGIC);
            (void)printf(" gp_header_->target_id   0x%04x, expect value = 0x%04x\n", gp_header_->target_id, TARGET_ID);
            (void)printf(" gp_header_->api_version 0x%04x, expect value < 0x%04x\n", gp_header_->api_version, CODEC_API_VERSION);
            (void)printf(" gp_header_->load_addr   0x%08x, expect value = 0x%08x\n", gp_header_->load_addr,
                   (int32_t)&codec_start_addr);
            (void)printf(" gp_header_->end_addr    0x%08x, expect value < 0x%08x\n", gp_header_->end_addr,
                   (int32_t)&codec_end_addr);
            AUD_SKIP = true;
            return int32_t_ERROR;
        }
        // Restart Task
    #ifdef ENABLE_AUDIO_PROCESSOR
        audio_fire_audio_processor_();
        ret = 1;
    #endif
        if (pdFAIL == ret)
        {
            (void)printf("Failed to create audio task\n");
            AUD_SKIP = true;
            return int32_t_ERROR;
        }
        g_audio_plugin_type_ = audio_type;
    }
    else
    {
        // Resume the task that stores in the memory
    #ifdef ENABLE_AUDIO_PROCESSOR
        audio_reset_audio_processor_();
        audio_fire_audio_processor_();
    #endif
    }
#endif // !defined(ENABLE_CODECS_PLUGIN)

    wave_ADPCM = 0;

#if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
    AUD_CheckProcessor();
    ithInvalidateDCacheRange(gp_audio_stream_, SHARE_MEM1_SIZE);
#endif

    switch (audio_type)
    {
    (void)printf("[Audio Driver] %s %d audio_type = %d\n", __FUNCTION__, __LINE__, audio_type);

        case ITE_WMA_DECODE:
#if HAVE_WMA
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #endif
#endif
            AUD_SKIP = !HAVE_WMA;
            break;

        case ITE_AAC_DECODE:
#if HAVE_AAC
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #else
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
    #endif
#endif
            AUD_SKIP = !HAVE_AAC;
            break;

        case ITE_OGG_DECODE:
#if HAVE_OGG
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            (void)printf("[Audio Driver] ogg audio plugin message buffer-length %u 0x%x %p\n",
                   (uint32_t)g_audio_plugin_message_buffer_length_, (uint32_t)&g_audio_plugin_message_buffer_length_,
                   gp_audio_plugin_message_buffer_);
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #else
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
    #endif
#endif
            AUD_SKIP = !HAVE_OGG;
            break;

        case ITE_OPUS_DECODE:
#if HAVE_OPUS
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            (void)printf("[Audio Driver] OPUS audio plugin message buffer-length %u 0x%x %p\n",
                   (uint32_t)g_audio_plugin_message_buffer_length_, (uint32_t)&g_audio_plugin_message_buffer_length_,
                   gp_audio_plugin_message_buffer_);
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #else
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
    #endif
#endif
            AUD_SKIP = !HAVE_OPUS;
            break;

        case ITE_AC3_DECODE:
#if HAVE_AC3
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #else
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
    #endif
#endif
            break;

        case ITE_AC3_SPDIF_DECODE:
#if HAVE_SPDIF_AC3
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
#endif
            break;

        case ITE_AACPLUS_DECODE:
            AUD_SKIP = true;
            break;

        case ITE_AMR_CODEC:
#if HAVE_AMR
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #endif
#endif
            AUD_SKIP = !HAVE_AMR;
            break;

        case ITE_BSAC_DECODE:
            AUD_SKIP = !HAVE_BSAC;
            break;

        case ITE_MIXER:
            AUD_SKIP = !HAVE_MIXER;
            break;

        case ITE_PCM_CODEC:
            AUD_SKIP = !HAVE_PCM;
            break;

        case ITE_MIDI:
            AUD_SKIP = !HAVE_MIDI;
            break;

        case ITE_WAV_DECODE:
        case ITE_MP3_DECODE:
#if HAVE_MP3
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #endif
#endif
            AUD_SKIP = !HAVE_MP3;
            break;

        case ITE_FLAC_DECODE:
#if defined(HAVE_FLAC)
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            (void)printf("[Audio Driver] flac audio plugin buffer-length %u 0x%x %p\n",
                   (uint32_t)g_audio_plugin_message_buffer_length_, (uint32_t)&g_audio_plugin_message_buffer_length_,
                   gp_audio_plugin_message_buffer_);
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
            (void)printf("[Audio Driver] stream buffer 0x%08x 0x%x length %u  0x%08x %u \n", (uint32_t)Audio_Decode_Bufptr,
                   (uint32_t)TOINT(gp_audio_stream_->codecStreamBuf), (uint32_t)Audio_Decode_Buflen,
                   (uint32_t)gp_audio_stream_->codecStreamBuf, (uint32_t)gp_audio_stream_->codecStreamLength);
    #else
            gp_header_->codec_info(&Audio_Decode_Bufptr, &Audio_Decode_Buflen);
    #endif
#endif
            break;

        case ITE_SBC_CODEC:
#if HAVE_SBC
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            Audio_Echo_Bufptr       = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Echo_Buflen       = TOINT(gp_audio_stream_->codecStreamLength);
            Audio_Ref_Bufptr        = TOINT(gp_audio_stream_->codecStreamBuf1) + CODEC_REMAP_OFFSET;
            Audio_Ref_Buflen        = TOINT(gp_audio_stream_->codecStreamLength1);
            Audio_Echocc_Bufptr     = TOINT(gp_audio_stream_->codecStreamBuf2) + CODEC_REMAP_OFFSET;
            Audio_Echocc_Buflen     = TOINT(gp_audio_stream_->codecStreamLength2);
            Audio_Parameter_Cmdptr  = TOINT(gp_audio_stream_->codecParameterBuf) + CODEC_REMAP_OFFSET;
            Audio_Parameter_Cmdlen  = TOINT(gp_audio_stream_->codecParameterLength);
    #endif
#endif
            AUD_SKIP = !HAVE_SBC;
            break;

        case ITE_AAC_ENCODE:
#if HAVE_AACENC
    #if defined(ENABLE_AUDIO_PROCESSOR) && defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
            (void)printf("[Audio Driver] aac enc audio plugin g_audio_plugin_message_buffer_length_ %u 0x%x %p\n",
                   (uint32_t)g_audio_plugin_message_buffer_length_, (uint32_t)&g_audio_plugin_message_buffer_length_,
                   gp_audio_plugin_message_buffer_);
            Audio_Decode_Bufptr = TOINT(gp_audio_stream_->codecStreamBuf) + CODEC_REMAP_OFFSET;
            Audio_Decode_Buflen = TOINT(gp_audio_stream_->codecStreamLength);
    #endif
#endif
            AUD_SKIP = !HAVE_MP2ENC;
            break;

        case EXTERNAL_ENGINE_PLUGIN:
            break;

        default:
            return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
            break;
    }

    Audio_DefDecode_Bufptr = Audio_Decode_Bufptr;

    // add for sbc codec on risc
    Audio_DefEcho_Bufptr        = Audio_Echo_Bufptr;
    Audio_DefRef_Bufptr         = Audio_Ref_Bufptr;
    Audio_DefEchocc_Bufptr      = Audio_Echocc_Bufptr;
    Audio_DefParameter_Cmdptr   = Audio_Parameter_Cmdptr;

    g_audio_engine_type_        = audio_type;
    goto END;

/*RESET_ENGINE:
    if (audio_reset_engine_() != 0)
        return AUDIO_ERROR_ENGINE_RESET_ENGINE_ERROR;*/
END:
    if (audio_config_engine_() != 0)
    {
        return AUDIO_ERROR_ENGINE_RESET_ENGINE_ERROR;
    }

    if (AUD_SKIP)
    {
        return 0;
    }

    return audio_initial_codec_();
}

int32_t
audio_config_engine_ (
    void)
{
    // LOG_ENTER "audio_config_engine_()\n" LOG_END

    switch (g_audio_engine_type_)
    {
        case ITE_AMR_CODEC:
            // fix amr codec do not use AEC
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (0x2 << 8), (0x3 << 8));
            // fix amr codec decode rate to 5.15kbps
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, ITE_AUDIO_AMR_ENCODE_1220, (0x7 << 11));
            break;

#if HAVE_PCM
        case ITE_PCM_CODEC:
            // codec mode.
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (0x3 << 8), (0x3 << 8));
            break;

#endif
        default:
            break;
    }
    return 0;
}

int32_t
audio_reset_engine_ (
    void)
{
    // reset engine buffer address to default.
    Audio_Decode_Bufptr = Audio_DefDecode_Bufptr;

    // add for sbc codec risc
    Audio_Echo_Bufptr       = Audio_DefEcho_Bufptr;
    Audio_Ref_Bufptr        = Audio_DefRef_Bufptr;
    Audio_Echocc_Bufptr     = Audio_DefEchocc_Bufptr;
    Audio_Parameter_Cmdptr  = Audio_DefParameter_Cmdptr;

    Audio_Decode_Wrptr      = 0;
    Audio_Decode_Rdptr      = 0;

    return 0;
}

/*
 *  reset AudioEngineType
 *
 */
int32_t
audio_reset_audio_engine_type_ (
    void)
{
    g_audio_engine_type_ = ITE_RESERVED;
    return 1;
}

int32_t
iteAudioInitialize (
    void)
{
#ifdef AUDIO_FREERTOS
    lastTime = PalGetClock();
#endif

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)
    gp_audio_plugin_message_buffer_         = (int16_t *)iteRiscGetTargetMemAddress(AUDIO_MESSAGE_MEM_TARGET);
    g_audio_plugin_message_buffer_length_   = AUDIO_MESSAGE_SIZE / sizeof(int16_t);
    gp_audio_stream_                        = (AUDIO_CODEC_STREAM *)iteRiscGetTargetMemAddress(SHARE_MEM1_TARGET);
#endif

    g_audio_base_address_   = 0U;

    gb_is_audio_inited_     = true;
    return 0;
}

int32_t
iteAudioGetAvailableBufferLength (
    ITE_AUDIO_BUFFER_TYPE   bufferType,
    uint32_t                * bufferLength)
{
    if (AUD_SKIP)
    {
        *bufferLength = 65535;
        return 0;
    }

    switch (bufferType)
    {
        case ITE_AUDIO_INPUT_BUFFER:
            return 0;
            break;

        case ITE_AUDIO_OUTPUT_BUFFER:
            if (audio_get_write_available_buffer_length((uint32_t *)bufferLength) == 0)
            {
                if (*bufferLength < BufThreshold)
                {
                    *bufferLength = 0;
                }

                return 0;
            }
            break;

        case ITE_AUDIO_MIXER_BUFFER:
            return 0;

        default:
            break;
    }

    *bufferLength = 0;
    return AUDIO_ERROR_CODEC_UNKNOW_ERROR;
}

int32_t
iteAudioGetFlashAvailableBufferLength (
    int32_t     nAudioInput,
    uint32_t    * bufferLength)
{
    return 0;
}

int32_t
iteAudioWriteStream (
    uint8_t     * stream,
    uint32_t    length)
{
    uint32_t    drvTimeout = 0;
    uint32_t    writableQueueLen;
    uint32_t    bottomLen;
    uint32_t    topLen;
    uint32_t    residualLength;

    if (AUD_SKIP)
    {
        return 0;
    }

    drvTimeout = 0;

    switch (g_audio_engine_type_)
    {
        case ITE_AMR_ENCODE:
            return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;

        default:
            break;
    }

    residualLength = length;

    while (residualLength)
    {
        // Wait buffer avaliable
        // PRECONDITION(Audio_Decode_Buflen > BufThreshold);

        do
        {
            audio_get_write_available_buffer_length(&writableQueueLen);
            if (writableQueueLen < BufThreshold)
            {
                ithDelay(1);
            }
            else
            {
                break;
            }
            drvTimeout++;
        } while (drvTimeout < 200);

        if ((drvTimeout == 200) && (writableQueueLen < BufThreshold))
        {
            continue;
        }

        if (residualLength <= writableQueueLen)
        {
            writableQueueLen = residualLength;
        }

        if (writableQueueLen != 0)
        {
            bottomLen = Audio_Decode_Buflen - Audio_Decode_Wrptr;
            if (bottomLen > writableQueueLen)
            {
                bottomLen = writableQueueLen;
            }

            memcpy((void *)(g_audio_base_address_ + Audio_Decode_Bufptr + Audio_Decode_Wrptr), (const void *)stream,
                   bottomLen);
            ithFlushDCacheRange((void *)(g_audio_base_address_ + Audio_Decode_Bufptr + Audio_Decode_Wrptr), bottomLen);

            stream  += bottomLen;
            topLen  = writableQueueLen - bottomLen;

            if (topLen > 0)
            {
                memcpy((void *)(g_audio_base_address_ + Audio_Decode_Bufptr), (const void *)(stream), topLen);
                ithFlushDCacheRange((void *)(g_audio_base_address_ + Audio_Decode_Bufptr), topLen);

                stream += topLen;
            }

            ithFlushMemBuffer();
        }

        // Update Write Pointer (word alignment)
        Audio_Decode_Wrptr  = (uint16_t)(Audio_Decode_Wrptr + writableQueueLen);

        residualLength      -= writableQueueLen;

        if (Audio_Decode_Wrptr >= Audio_Decode_Buflen)
        {
            Audio_Decode_Wrptr -= Audio_Decode_Buflen;
        }

        ITE_AUDIO_WRITE_REG(DrvDecode_WrPtr, Audio_Decode_Wrptr);
    }

    return 0;
}

int32_t
iteAudioWriteFlashStream (
    int32_t     nAudioInput,
    uint8_t     * stream,
    uint32_t    length)
{
    return 0;
}

#if defined(DRIVER_CODEC) && !defined(CFG_COMPRESS_AUDIO_PLUGIN)
    #define ADDR_AND_SIZE(img)  (img), sizeof(img)
#else
    #define ADDR_AND_SIZE(img)  NULL, 0
#endif

#define END_OF_CODEC_IMAGE_TABLE                                                                                       \
    {                                                                                                                  \
        NULL, ITE_RESERVED, NULL, 0                                                                                    \
    }

typedef struct AUDIO_CODEC_IMAGE_TAG
{
    const char * const      name;
    const ITE_AUDIO_ENGINE  id;
    const uint8_t * const   addr;
    const uint32_t          size;
} AUDIO_CODEC_IMAGE;

static const AUDIO_CODEC_IMAGE g_codec_image_[] = {
#ifdef CFG_AUDIO_CODEC_MP3DEC
    {"mp3",    ITE_MP3_DECODE,    ADDR_AND_SIZE(g_mp3_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_AACDEC
    {"aac",    ITE_AAC_DECODE,    ADDR_AND_SIZE(g_aac_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_EAC3DEC
    {"eac3",   ITE_AC3_DECODE,    ADDR_AND_SIZE(g_ac3_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_WAV
    {"wave",   ITE_WAV_DECODE,    ADDR_AND_SIZE(g_wav_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_WMADEC
    {"wma",    ITE_WMA_DECODE,    ADDR_AND_SIZE(g_wma_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_AMR
    {"amr",    ITE_AMR_DECODE,    ADDR_AND_SIZE(g_amr_decoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_FLACDEC
    {"flac",   ITE_FLAC_DECODE,   ADDR_AND_SIZE(g_flac_decoder_)     },
#endif
#ifdef CFG_AUDIO_CODEC_SBC
    {"sbc",    ITE_SBC_CODEC,     ADDR_AND_SIZE(g_sbc_codec_)        },
#endif
#ifdef CFG_AUDIO_CODEC_MP2ENC
    {"mp2",    ITE_MP2_ENCODE,    ADDR_AND_SIZE(g_mp2_encoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_AACENC
    {"aacenc", ITE_AAC_ENCODE,    ADDR_AND_SIZE(g_aac_encoder_)      },
#endif
#ifdef CFG_AUDIO_CODEC_VORBISDEC
    {"vorbis", ITE_OGG_DECODE,    ADDR_AND_SIZE(g_vorbis_decoder_)   },
#endif
#ifdef CFG_AUDIO_CODEC_OPUSDEC
    {"opus",   ITE_OPUS_DECODE,   ADDR_AND_SIZE(g_opus_decoder_)     },
#endif

    END_OF_CODEC_IMAGE_TABLE
};

/**
 * @brief  Read audio codec from file to buffer and load to RISC1
 *
 * @param  audio_type audio engine type, mp3, aac, etc.
 *
 * @return
 *     - nImagesize
 *
 * @note
 *     -# In driver mode, the codec file is located on CFG_PRIVATE_DRIVE.
 *     -# The RISC1 will load data from buffer to its memory.
 *     -# In plugin mode, the codec file is located on A:/, the plugin buffer.
 *     -# The plugin will load data from file to buffer.
 *     -# If the size of codec file is larger than plugin buffer,
 *        the reading process will be stopped and return 0.
 */
uint32_t
audioReadCodec (
    ITE_AUDIO_ENGINE audio_type)
{
    uint32_t    img_size = 0U;              // image size
    int32_t     i;                          // loop index

#if defined(DRIVER_CODEC) && !defined(CFG_COMPRESS_AUDIO_PLUGIN)
    const uint8_t * p_img_buf = NULL;       // image buffer address

    // find the corresponded codec engine type in table
    for (i = 0; g_codec_image_[i].id != ITE_RESERVED; ++i)
    {
        if (g_codec_image_[i].id == audio_type)
        {
            (void)printf("open %s engine\n", g_codec_image_[i].name);
            p_img_buf   = g_codec_image_[i].addr;   // get image buffer address
            img_size    = g_codec_image_[i].size;   // get image size
            break;
        }
    }

    // copy audio codec (.hex) to gBuf
    if ((p_img_buf != NULL) && (img_size != 0U))
    {
        iteRiscLoadData(RISC1_IMAGE_MEM_TARGET, (uint8_t *)p_img_buf, img_size);    // load data from buffer to RISC1
    }

#elif !defined(DRIVER_CODEC) && !defined(CFG_COMPRESS_AUDIO_PLUGIN)
    FILE        * p_file    = NULL;                                                 // file pointer
    uint32_t    result      = 0U;                                                   // read result

    // find the corresponded codec engine type in table
    for (i = 0; g_codec_image_[i].id != ITE_RESERVED; ++i)
    {
        if (g_codec_image_[i].id == audio_type)
        {
            char codec_file_name[40];        // ex. CFG_PRIVATE_DRIVE ":/codec/mp3.codecs"

            (void)printf("open %s engine\n", g_codec_image_[i].name);

            // check buffer size
            assert(strlen(g_codec_image_[i].name) <
                   (sizeof(codec_driver_name) - strlen(CFG_PRIVATE_DRIVE ":/codec/.codecs")));

            // get codec file name
            snprintf(codec_file_name, sizeof(codec_file_name),
                     CFG_PRIVATE_DRIVE ":/codec/%s.codecs", g_codec_image_[i].name);

            p_file = fopen(codec_file_name, "rb");                                  // open file

            // if not find file on CFG_PRIVATE_DRIVE, try to find it on A:/
            if (!p_file)
            {
                sprintf(codec_file_name, "A:/%s.codecs", g_codec_image_[i].name);   // ex. "A:/mp3.codecs"
                p_file = fopen(codec_file_name, "rb");
            }

            break;
        }
    }

    // if not find the corresponded codec engine in table
    if (!p_file)
    {
        (void)printf("can not open codec \n");
        return 0;
    }

    // set file pointer to end
    fseek(p_file, 0, SEEK_END);

    // get file size
    img_size = ftell(p_file);

    // set file pointer to start
    fseek(p_file, 0, SEEK_SET);

    // check buffer size
    if (img_size < sizeof(gBuf))
    {
        result = fread(gBuf, 1, img_size, p_file);  // read data into buffer

        // if reading size is not equal to file size, print error message
        if (result != img_size)
        {
            (void)printf(" image size %d  wrong  %d \n", img_size, result);
        }
    }
    else
    {
        (void)printf("image size(%u) is too large.\n", img_size);
        img_size = 0;
    }

    // print buffer data to debug
    (void)printf(" buf 0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x \n",
           gBuf[0], gBuf[1], gBuf[2], gBuf[3], gBuf[4], gBuf[5], gBuf[6], gBuf[7], gBuf[8], gBuf[9], gBuf[10], gBuf[11]);

    fclose(p_file);                               // close file

#else
    for (i = 0; g_codec_image_[i].id != ITE_RESERVED; ++i)
    {
        if (g_codec_image_[i].id == audio_type)
        {
            char    codec_driver_name[40];    // ex. ":codec:mp3"
            int32_t fd;

            (void)printf("open %s engine\n", g_codec_image_[i].name);

            // check buffer size
            assert(strlen(g_codec_image_[i].name) < (sizeof(codec_driver_name) - strlen(":codec:")));

            // get codec file name
            snprintf(codec_driver_name, sizeof(codec_driver_name), ":codec:%s", g_codec_image_[i].name);

            fd = open(codec_driver_name, 0);                                                    // open codec file

            if (fd != -1)
            {
                ioctl(fd, ITP_IOCTL_GET_SIZE, &img_size);                                       // get file size

                if (img_size < RISC1_IMAGE_SIZE)
                {
                    int32_t read_bytes = 0;
                    read_bytes = read(fd, iteRiscGetTargetMemAddress(RISC1_IMAGE_MEM_TARGET), img_size);     // load data from file to RISC1
                    if (read_bytes != (int32_t)img_size)
                    {
                        (void)printf("Error reading codec file: expected %u bytes, got %d bytes\n", img_size, read_bytes);
                        img_size = 0;
                    }
                }
                else
                {
                    (void)printf("image size(%u) is too large.\n", img_size);
                    img_size = 0;
                }

                close(fd);
            }

            break;
        }
    }
#endif

    return img_size;    // return image size
}

int32_t
iteAudioOpenEngine (
    ITE_AUDIO_ENGINE audio_type)
{
    // LOG_ENTER "iteAudioOpenEngine()\n" LOG_END
    pthread_mutex_lock(&gp_mutex_);
    if (!gb_is_audio_inited_)
    {
        iteAudioInitialize();
    }

#ifdef ENABLE_AUDIO_PROCESSOR
    audio_reset_audio_processor_();
#endif
    // get the audio codec address , length
    audioReadCodec(audio_type);
    // load engine
    if (audio_load_engine_(audio_type) != 0)
    {
        pthread_mutex_unlock(&gp_mutex_);
        return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
    }
    pthread_mutex_unlock(&gp_mutex_);
    return 0;
}

int32_t
iteAudioStopEngine (
    void)
{
    // LOG_ENTER "iteAudioStopEngine()\n" LOG_END

#ifdef ENABLE_AUDIO_PROCESSOR
    if (gb_is_audio_inited_)
    {
        audio_reset_audio_processor_(); // HX stop Audioprocessor
    }
#endif
    gb_is_audio_inited_ = false;        // need to init again
    return 0;
}

int32_t
iteAudioSetAttrib (
    const ITE_AUDIO_ATTRIB  attrib,
    const void              * value)
{
    int32_t     result = 0;
    uint16_t    nTemp;
    // LOG_ENTER "iteAudioSetAttrib()\n" LOG_END

    switch (attrib)
    {
        case ITE_AUDIO_STREAM_SAMPLERATE:
            AUD_decSampleRate = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_STREAM_CHANNEL:
            AUD_decChannel = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_CODEC_SET_SAMPLE_RATE:
            AUD_nCodecSampleRate    = *((uint32_t *)(value));
            AUD_decSampleRate       = AUD_nCodecSampleRate;
            break;

        case ITE_AUDIO_CODEC_SET_CHANNEL:
            AUD_nCodecChannels  = *((uint32_t *)(value));
            AUD_decChannel      = AUD_nCodecChannels;
            break;

        case ITE_AUDIO_CODEC_SET_BUFFER_LENGTH:
            AUD_nCodecPCMBufferLength = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_I2S_INIT:
            AUD_bI2SInit = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_I2S_PTR:
            AUD_pI2Sptr = (char *)(value);
            break;

        case ITE_AUDIO_ENABLE_MPLAYER:
            AUD_bMPlayer = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_MPLAYER_STC_READY:
            AUD_bMPlayerSTCReady = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_DECODE_ERROR:
            if ((*((uint32_t *)(value)) == 0) || (*((uint32_t *)(value)) == 1))
            {
                AUD_bDecodeError    = *((uint32_t *)(value));
                nTemp               = AUD_bDecodeError;
                ITE_AUDIO_WRITE_REG(AUDIO_DECODER_WRITE_DECODE_ERROR, nTemp);
            }
            break;

        case ITE_AUDIO_DECODE_DROP_DATA:
        {
            AUD_nDropData   = *((uint32_t *)(value));
            nTemp           = AUD_nDropData;
            (void)printf("[Audio Driver]set drop audio %d \n", nTemp);
            ITE_AUDIO_WRITE_REG(AUDIO_DECODER_DROP_DATA, nTemp);
        }
        break;

        case ITE_AUDIO_PAUSE_STC_TIME:
            AUD_nPauseSTCTime = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_ENABLE_STC_SYNC:
            AUD_bEnableSTC_Sync = *((uint32_t *)(value));
            break;

        case ITE_AUDIO_FFMPEG_PAUSE_AUDIO:
            AUD_nFfmpegPauseAudio = *((uint32_t *)(value));
            break;

        default:
            result = AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
            break;
    }

    return result;
}

int32_t
iteAudioGetAttrib (
    const ITE_AUDIO_ATTRIB  attrib,
    void                    * value)
{
    int32_t     result  = 0;
    uint16_t    data    = 0;

    // LOG_ENTER "iteAudioSetAttrib()\n" LOG_END

    switch (attrib)
    {
        case ITE_AUDIO_GET_IS_EOF:
            //            *((uint32_t*)(value)) = isEOF();
            break;

        case ITE_AUDIO_CODEC_SET_SAMPLE_RATE:
            *((uint32_t *)(value)) = AUD_nCodecSampleRate;
            break;

        case ITE_AUDIO_CODEC_SET_CHANNEL:
            *((uint32_t *)(value)) = AUD_nCodecChannels;
            break;

        case ITE_AUDIO_CODEC_SET_BUFFER_LENGTH:
            *((uint32_t *)(value)) = AUD_nCodecPCMBufferLength;
            break;

        case ITE_AUDIO_I2S_PTR:
            *((uint32_t *)(value)) = (uint32_t)AUD_pI2Sptr;
            break;

        case ITE_AUDIO_I2S_INIT:
            *((uint32_t *)(value)) = AUD_bI2SInit;
            break;

        case ITE_AUDIO_ENABLE_MPLAYER:
            *((uint32_t *)(value)) = AUD_bMPlayer;
            break;

        case ITE_AUDIO_MPLAYER_STC_READY:
            *((uint32_t *)(value)) = AUD_bMPlayerSTCReady;
            break;

        case ITE_AUDIO_DECODE_ERROR:
            AUD_bDecodeError    = 0;
            data                = ITE_AUDIO_READ_REG(AUDIO_DECODER_WRITE_DECODE_ERROR);
            if (data > 0)
            {
                AUD_bDecodeError = 1;
            }
            *((uint32_t *)(value)) = AUD_bDecodeError;
            break;

        case ITE_AUDIO_DECODE_DROP_DATA:
            AUD_nDropData   = 0;
            data            = ITE_AUDIO_READ_REG(AUDIO_DECODER_DROP_DATA);
            if (data > 0)
            {
                AUD_nDropData = data;
            }
            *((uint32_t *)(value)) = AUD_nDropData;
            break;

        case ITE_AUDIO_PAUSE_STC_TIME:
            *((uint32_t *)(value)) = AUD_nPauseSTCTime;
            break;

        case ITE_AUDIO_ENABLE_STC_SYNC:
            *((uint32_t *)(value)) = AUD_bEnableSTC_Sync;
            break;

        case ITE_AUDIO_DIRVER_DECODE_BUFFER_LENGTH:
            *((uint32_t *)(value)) = Audio_Decode_Buflen;
            break;

        case ITE_AUDIO_FFMPEG_PAUSE_AUDIO:
            *((uint32_t *)(value)) = AUD_nFfmpegPauseAudio;
            break;

        default:
            result = AUDIO_ERROR_CODEC_UNKNOW_CODEC_MODE;
            break;
    }
    return result;
}

int32_t
iteAudioGetDecodeTimeV2 (
    uint32_t * p_time)
{
    uint16_t    data = 0;
    uint32_t    I2SBufLen;
    uint32_t    I2SRdPtr;
    uint32_t    I2SWrPtr;
    uint32_t    I2SDataLength;
    uint32_t    decTime = 0;
    uint32_t    offset = 0;
    int32_t     result = 0;
    uint16_t    nTemp, timeout = 0;

    if (AUD_SKIP)
    {
#ifdef AUDIO_FREERTOS
        *p_time = PalGetDuration(lastTime);
#endif
        return 0;
    }
#ifndef CFG_RISC_TS_DEMUX_PLUGIN
    if ((AUD_decChannel == 0) || (AUD_decSampleRate == 0))
    {
        (void)printf("[Audio Driver]iteAudioGetDecodeTimeV2  AUD_decChannel %u AUD_decSampleRate %u \n",
               (uint32_t)AUD_decChannel, (uint32_t)AUD_decSampleRate);
        result = AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
    }
    else
    {
WAIT:
        timeout = 0;
        do
        {
            nTemp = iteAudioGetAuidoProcessorWritingStatus();
            if (nTemp > 0)
            {
                (void)printf("[Audio Driver]wait %d \n", timeout);
                ithDelay(1);
                timeout++;
            }
        } while (nTemp > 0 && timeout < 5);

        data    = ITE_AUDIO_READ_REG(DrvDecode_Frame_Hi);
        decTime = (uint32_t)(data << 16);

        data    = ITE_AUDIO_READ_REG(DrvDecode_Frame_Lo);
        decTime += (uint32_t)data;

        // data = ITE_AUDIO_READ_REG(0x1654);
        I2SBufLen   = AUD_nCodecPCMBufferLength;    // (uint32_t)data;
        // data = ITE_AUDIO_READ_REG(0x165C);
        I2SRdPtr    = I2S_DA32_GET_RP();            // (uint32_t)data;
        nTemp       = iteAudioGetAuidoProcessorWritingStatus();
        // need to re-read register
        if (nTemp > 0)
        {
            (void)printf("[Audio Driver]wait write \n");
            goto WAIT;
        }
        // data = ITE_AUDIO_READ_REG(0x1656);

        I2SWrPtr        = I2S_DA32_GET_WP(); // (uint32_t)data;

        I2SDataLength   = (I2SRdPtr > I2SWrPtr) ? (I2SBufLen - (I2SRdPtr - I2SWrPtr)) : (I2SWrPtr - I2SRdPtr);
        I2SDataLength   = (AUD_decChannel == 2) ? (I2SDataLength >> 2) : (I2SDataLength >> 1);
        I2SDataLength   = ((I2SDataLength << 16) / AUD_decSampleRate);

        if (gLastDecTime)
        {
            // time increment wrap around
            if ((decTime < gLastDecTime) && (gLastDecTime - decTime >= AUDIO_WRAP_AROUND_JUDGE_RANGE))
            {
                gPtsTimeBaseOffset += AUDIO_WRAP_AROUND_THRESHOLD;
            }
            else if ((decTime - gLastDecTime) >= AUDIO_WRAP_AROUND_JUDGE_RANGE) // time decrement wrap around
            {
                if (gPtsTimeBaseOffset)
                {
                    gPtsTimeBaseOffset -= AUDIO_WRAP_AROUND_THRESHOLD;
                }
            }
        }

        gLastDecTime    = decTime;
        offset          = gPtsTimeBaseOffset;

        if (decTime > I2SDataLength)
        {
            decTime -= I2SDataLength;
        }
        else // decTime <= I2SDataLength
        {
            if (offset)
            {
                offset -= (((((I2SDataLength - decTime) & 0xffff) * 1000) >> 16) +
                    (((I2SDataLength - decTime) >> 16) * 1000));
            }
            else
            {
                decTime = I2SDataLength;
            }
        }

        (*p_time) = ((((decTime) & 0xffff) * 1000) >> 16) + (((decTime) >> 16) * 1000) + offset;
    }
#endif
    return result;
}

int32_t
iteAudioGetDecodeTime (
    uint32_t * p_time)
{
    uint16_t data = 0;

    if (AUD_SKIP)
    {
#ifdef AUDIO_FREERTOS
        *p_time = PalGetDuration(lastTime);
#endif
        return 0;
    }

    if (g_audio_engine_type_ == ITE_AMR_ENCODE)
    {
        return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
    }

    data        = ITE_AUDIO_READ_REG(DrvDecode_Frame_Hi);
    (*p_time)   = (uint32_t)data * 1000;

    data        = ITE_AUDIO_READ_REG(DrvDecode_Frame_Lo);
    (*p_time)   += ((uint32_t)(data * 1000) >> 16);

    return 0;
}

int32_t
iteAudioPause (
    bool enable)
{
    // LOG_ENTER "iteAudioPause(%d)\n", enable LOG_END

    if (AUD_SKIP)
    {
        return 0;
    }

    if (AUD_bEnableSTC_Sync == 1)
    {
        // mplayer using iteAudioSTCSyncPause()
        if (!AUD_bMPlayer)
        {
            if (enable == 1)
            {
                ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 1, DrvDecode_PAUSE);
            }
            else
            {
                ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 0, DrvDecode_PAUSE);
            }
        }
    }
    else
    {
        if (enable == 1)
        {
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 1, DrvDecode_PAUSE);
        }
        else
        {
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 0, DrvDecode_PAUSE);
        }
    }
    return 0;
}

int32_t
iteAudioSTCSyncPause (
    bool enable)
{
    ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, !!enable, DrvDecode_PAUSE);
    return 0;
}

int32_t
iteAudioSeek (
    bool enable)
{
    return 0;
}

int32_t
iteAudioStop (
    void)
{
    uint16_t    regData = 0U;
    uint32_t    timeout = 3000U;
    uint32_t    nTmp    = 0U;
    uint32_t    nTemp   = 0U;

    // LOG_ENTER "iteAudioStop()\n" LOG_END

#ifdef AUDIO_FREERTOS
    lastTime = PalGetClock();
#endif
    if (AUD_SKIP || !gb_is_audio_inited_)
    {
        return 0;
    }

    iteAudioSetAttrib(ITE_AUDIO_MPLAYER_STC_READY, (void *)&nTemp);
    iteAudioSetAttrib(ITE_AUDIO_DECODE_DROP_DATA, (void *)&nTemp);
    iteAudioSetAttrib(ITE_AUDIO_PAUSE_STC_TIME, (void *)&nTmp);

    ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (1 << 6), DrvDecode_STOP);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_WRIDX, 0);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_HI, 0);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_LO, 0);
    AUD_SKIP = true;
    do
    {
        regData = ITE_AUDIO_READ_REG(DrvAudioCtrl);
        if (!(regData & DrvDecode_STOP))
        {
            Audio_Decode_Wrptr  = 0;
            // Audio_Decode_Wrptr3 = 0;
            gPtsTimeBaseOffset  = 0;
            gLastDecTime        = 0;
#ifndef CFG_RESERVE_FILTER
            i2s_deinit_DAC();
#endif
            // Clear pause status
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 0, DrvDecode_PAUSE);
            return 0;
        }
        (void)usleep(1000);
        timeout--;
        if (timeout == 0)
        {
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (0 << 6), DrvDecode_STOP);
            // Clear pause status
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 0, DrvDecode_PAUSE);

#ifndef CFG_RESERVE_FILTER
            i2s_deinit_DAC();
#endif
            (void)printf("[Audio Driver] Stop %s\n", __FUNCTION__);
            return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
        }
    } while (true);
}

int32_t
iteAudioStopQuick (
    void)
{
    uint16_t    regData = 0;
    uint32_t    timeout = 200;

    if (AUD_SKIP || !gb_is_audio_inited_)
    {
        return 0;
    }

    ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (1 << 6), DrvDecode_STOP);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_WRIDX, 0);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_HI, 0);
    ITE_AUDIO_WRITE_REG(MMIO_PTS_LO, 0);

    do
    {
        regData = ITE_AUDIO_READ_REG(DrvAudioCtrl);
        if (!(regData & DrvDecode_STOP))
        {
            Audio_Decode_Wrptr = 0;
            return 0;
        }
        (void)usleep(1000);
        timeout--;
        if (timeout == 0)
        {
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, (0 << 6), DrvDecode_STOP);
            // Clear pause status
            ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl, 0, DrvDecode_PAUSE);
            (void)printf("[Audio Driver] Stop %s\n", __FUNCTION__);
            return AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE;
        }
    } while (true);
}

int32_t
iteAudioGenWaveDecodeHeader (
    ITE_WaveInfo    * wavInfo,
    uint8_t         * wave_header)
{
#if HAVE_WAV
    uint32_t    nSamplesPerSec = 44100;
    uint16_t    wFormatTag;
    uint16_t    i;
    uint8_t     * ref;

    // LOG_ENTER "iteAudioGenWaveDecodeHeader()\n" LOG_END
    AUD_decSampleRate = wavInfo->sampRate;
    if ((wavInfo->nChans > 2) && (wavInfo->nChans <= 6))
    {
        (void)printf("[Audio Driver] set wav channels %d \n", (int32_t)wavInfo->nChans);
        AUD_decChannel = 2;
    }
    else if (wavInfo->nChans <= 2)
    {
        AUD_decChannel = wavInfo->nChans;
    }

    // set decode header
    for (i = 0; i < 48; i++)
    {
        wave_header[i] = 0;
    }

    // ckID
    wave_header[0]  = 'R';
    wave_header[1]  = 'I';
    wave_header[2]  = 'F';
    wave_header[3]  = 'F';
    // cksize
    wave_header[4]  = 0x24;
    wave_header[5]  = 0x60;
    wave_header[6]  = 0x01;
    wave_header[7]  = 0x00;
    // WAVEID
    wave_header[8]  = 'W';
    wave_header[9]  = 'A';
    wave_header[10] = 'V';
    wave_header[11] = 'E';
    // ckID
    wave_header[12] = 'f';
    wave_header[13] = 'm';
    wave_header[14] = 't';
    wave_header[15] = ' ';
    // cksize ADPCM is 20, others is 16
    if (wavInfo->format == 3)
    {
        wave_header[16] = 0x14;
        wave_ADPCM      = 4;
        ref             = wavInfo->pvData;
        wave_header[36] = 0x02;
        wave_header[37] = 0;
        wave_header[38] = ref[0];
        wave_header[39] = ref[1];
    }
    else
    {
        wave_header[16] = 0x10;
    }
    wave_header[17] = 0;
    wave_header[18] = 0;
    wave_header[19] = 0;

    // wFormatTag
    //  set format
    switch (wavInfo->format)
    {
        case ITE_WAVE_FORMAT_PCM:
            wFormatTag = 1;
            if (wavInfo->bitsPerSample == 8)
            {
                wave_header[34] = 0x08;
            }
            else
            {
                wave_header[34] = 0x10;
            }
            break;

        case ITE_WAVE_FORMAT_ALAW:
            wFormatTag      = 6;
            wave_header[34] = 0x08;
            break;

        case ITE_WAVE_FORMAT_MULAW:
            wFormatTag      = 7;
            wave_header[34] = 0x08;
            break;

        case ITE_WAVE_FORMAT_DVI_ADPCM:
            wFormatTag      = 0x11;
            wave_header[34] = 0x04;
            break;

        case ITE_WAVE_FORMAT_SWF_ADPCM:
            // 0xFE is ITE defined
            wFormatTag      = 0xFE;
            // block align is frame length
            wave_header[33] = (uint8_t)((wavInfo->nDataSize >> 8U) & 0xFFU);
            wave_header[32] = (uint8_t)((wavInfo->nDataSize) & 0xFFU);
            wave_header[34] = 0x10;
            break;

        default:
            wFormatTag      = 0;
            wave_header[34] = 0x10;
            break;
    }

    wave_header[21] = (uint8_t)((wFormatTag >> 8U) & 0xFFU);
    wave_header[20] = (uint8_t)((wFormatTag) & 0xFFU);

    if (wavInfo->nChans <= 6)
    {
        wave_header[23] = 0;
        wave_header[22] = wavInfo->nChans;
    }
    else
    {
        wave_header[23] = 0;
        wave_header[22] = 0x01;
        (void)printf("[Audio Driver] set wave header channels %d > 6\n", (int32_t)wavInfo->nChans);
    }

    if (wavInfo->sampRate <= 96000)
    {
        nSamplesPerSec = wavInfo->sampRate;
    }

    wave_header[27]                 = (uint8_t)((nSamplesPerSec >> 24U) & 0xFFU);
    wave_header[26]                 = (uint8_t)((nSamplesPerSec >> 16U) & 0xFFU);
    wave_header[25]                 = (uint8_t)((nSamplesPerSec >> 8U) & 0xFFU);
    wave_header[24]                 = (uint8_t)((nSamplesPerSec) & 0xFFU);

    wave_header[36 + wave_ADPCM]    = 'd';
    wave_header[37 + wave_ADPCM]    = 'a';
    wave_header[38 + wave_ADPCM]    = 't';
    wave_header[39 + wave_ADPCM]    = 'a';

    wave_header[40 + wave_ADPCM]    = 0;
    wave_header[41 + wave_ADPCM]    = 0;
    wave_header[42 + wave_ADPCM]    = 0;
    wave_header[43 + wave_ADPCM]    = 0;
#endif

    return 0;
}

int32_t
iteAudioGetMusicCodecDump ()
{
    uint16_t regdata;
    regdata = ITE_AUDIO_READ_REG(DrvAudioCtrl2);
    return ((regdata >> DrvCodecDump_Bits) & 1);
}

void
iteAudioSetMusicCodecDump (
    int32_t nEnable)
{
    ITE_AUDIO_WRITE_REG_MASK(DrvAudioCtrl2, (!!nEnable) << DrvCodecDump_Bits, DrvCodecDump);
}

uint16_t
iteAudioGetAuidoProcessorWritingStatus ()
{
    uint16_t nRegData;
    nRegData = ITE_AUDIO_READ_REG(AUDIO_PROCESSOR_WRITE_REGISTER_PROTECTION);
    return nRegData;
}

#if defined(AUDIO_PLUGIN_MESSAGE_QUEUE)

uint16_t *
iteAudioGetAudioCodecAPIBuffer (
    uint32_t * nLength)
{
    ithInvalidateDCacheRange(gp_audio_plugin_message_buffer_, g_audio_plugin_message_buffer_length_);
    *nLength = g_audio_plugin_message_buffer_length_;
    return (uint16_t *)gp_audio_plugin_message_buffer_;
}
#endif

int32_t
iteAudioSetFlashInputStatus (
    int32_t nInputNumber,
    int32_t nFormat,
    int32_t nUsing)
{
    return 0;
}

// get audio codec buffer base address
uint32_t
iteAudioGetAudioCodecBufferBaseAddress ()
{
    return CODEC_REMAP_OFFSET;
}

uint32_t
iteAecCommand (
    uint16_t    command,
    uint32_t    param0,
    uint32_t    param1,
    uint32_t    param2,
    uint32_t    param3,
    uint32_t    * value)
{
    uint16_t    reply;
    uint32_t    * pParameter;
    uint32_t    result          = 0xE000; // TODO: define error code
    uint32_t    timeout_count   = 100;
    // uint32_t   debug         = 0;

    // uint32_t   i;
    uint8_t     * src;
    uint8_t     * det;
    uint32_t    size;

    if ((command <= AEC_CMD_NULL) || (command >= AEC_CMD_LAST))
    {
        return result;
    }

    switch (command)
    {
        case AEC_CMD_PROCESS:
            // write data to echo buffer in
            size    = param3;
            det     = (uint8_t *)Audio_Echo_Bufptr;
            src     = (uint8_t *)param0;

            memcpy((void *)det, (void *)src, size);

            ithFlushDCacheRange((void *)Audio_Echo_Bufptr, size);

            // write data to ref buffer in
            det = (uint8_t *)Audio_Ref_Bufptr;
            src = (uint8_t *)param1;

            memcpy((void *)det, (void *)src, size);

            ithFlushDCacheRange((void *)Audio_Ref_Bufptr, size);
            break;

        case NR_CMD_PROCESS:
            size    = param3;
            // write data to echo buffer in
            det     = (uint8_t *)Audio_Echo_Bufptr;
            src     = (uint8_t *)param0;
            memcpy((void *)det, (void *)src, size);
            ithFlushDCacheRange((void *)Audio_Echo_Bufptr, size);

        default:
            break;
    }

    pParameter      = (uint32_t *)Audio_Parameter_Cmdptr;
    pParameter[0]   = param0;
    pParameter[1]   = param1;
    pParameter[2]   = param2;
    pParameter[3]   = param3;
    ithFlushDCacheRange((void *)pParameter, 16);
    ithFlushMemBuffer();

    // check if processing then send command to risc
    do
    {
        if ((ITE_AUDIO_READ_REG(AEC_COMMAND_SEND) == 0) && (ITE_AUDIO_READ_REG(AEC_COMMAND_REPLY) == 0))
        {
            break;
        }
        if (timeout_count == 0)
        {
            (void)printf("aec command process timeout, cmd: %d, reg cmd: %d, reg reply: %d\n", (int32_t)command,
                   (int32_t)ITE_AUDIO_READ_REG(AEC_COMMAND_SEND), (int32_t)ITE_AUDIO_READ_REG(AEC_COMMAND_REPLY));
            return result;
        }
        else
        {
            timeout_count--;
        }
        (void)usleep(1000);
    } while (true);
    ITE_AUDIO_WRITE_REG(AEC_COMMAND_SEND, command);

    if (command == AEC_CMD_RESET)
    {
        // exceptional case, no need to wait for reply
        return 0;
    }

    // processing in risc, wait for reply
    switch (command)
    {
        case AEC_CMD_PROCESS:
            (void)usleep(12000);
            break;

        case NR_CMD_PROCESS:
            break;

        default:
            break;
    }

    timeout_count = 100;
    do
    {
        if ((ITE_AUDIO_READ_REG(AEC_COMMAND_REPLY) == command) && (ITE_AUDIO_READ_REG(AEC_COMMAND_SEND) == 0))
        {
            break;
        }
        if (timeout_count == 0)
        {
            (void)printf("aec command wait timeout, cmd: %d, reg cmd: %d, reg reply: %d\n", (int32_t)command,
                   (int32_t)ITE_AUDIO_READ_REG(AEC_COMMAND_SEND), (int32_t)ITE_AUDIO_READ_REG(AEC_COMMAND_REPLY));
            return result;
        }
        else
        {
            timeout_count--;
        }
        (void)usleep(1000);
    } while (true);

    // get reply data
    reply   = ITE_AUDIO_READ_REG(AEC_COMMAND_REPLY);
    ithInvalidateDCacheRange((void *)Audio_Parameter_Cmdptr, 16);
    result  = pParameter[4];
    if (value)
    {
        *value = pParameter[5];
    }
    // debug  = pParameter[6];

    switch (reply)
    {
        case AEC_CMD_PROCESS:
            // read data to echo cancellation buffer out
            size    = param3;
            det     = (uint8_t *)param2;
            src     = (uint8_t *)Audio_Echocc_Bufptr;
            ithInvalidateDCacheRange((void *)Audio_Echocc_Bufptr, size);
            memcpy((void *)det, (void *)src, size);

            break;

        case NR_CMD_PROCESS:
            // read data to echo cancellation buffer out
            size    = param3;
            det     = (uint8_t *)param2;
            src     = (uint8_t *)Audio_Echocc_Bufptr;
            ithInvalidateDCacheRange((void *)Audio_Echocc_Bufptr, size);
            memcpy((void *)det, (void *)src, size);

            break;

        default:
            break;
    }

    ITE_AUDIO_WRITE_REG(AEC_COMMAND_REPLY, 0);
    return result;
}

void
iteAudioUpdateMessageQ (
    void)
{
    uint16_t Reg = 0;
    Reg = getAudioPluginStatus();
    if (((Reg & 0xc000) >> 14) == SMTK_AUDIO_PROCESSOR_ID)
    {
        smtkMainProcessorExecuteAudioPluginCmd(Reg);
    }
}

void
iteAudioCodecSetPcmIdx (
    int32_t index)
{
    pcmidx = index;
}

int32_t
iteAudioCodecGetPcmIdx (
    void)
{
    return pcmidx;
}
