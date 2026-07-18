/*
 * Copyright (c) 2005 ITE technology Corp. All Rights Reserved.
 */
/** @file
 * ITE Audio Driver API header file.
 *
 * @author Peter Lin
 * @version 1.0
 */
#ifndef ITE_AUDIO_H
#define ITE_AUDIO_H

// #include "ith_cfg.h"
#include "ite/ith.h"
#define ENABLE_AUDIO_PROCESSOR

#ifdef __cplusplus
extern "C"
{
#endif

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Macro Definition
//=============================================================================
#define ITE_AUDIO_PCM_FORMAT                         0x2000
#define ITE_AUDIO_MP3_FORMAT                         0x2001
#define ITE_AUDIO_AAC_FORMAT                         0x2002
#define ITE_AUDIO_AMR_FORMAT                         0x2003
#define ITE_AUDIO_MID_FORMAT                         0x2004
#define ITE_AUDIO_BSAC_FORMAT                        0x2005
#define ITE_AUDIO_AC3_FORMAT                         0x2006
#define ITE_AUDIO_NIL_FORMAT                         0x7FFF

#define ITE_AUDIO_AMR_ENCODE_475                     (0x0 << 11)
#define ITE_AUDIO_AMR_ENCODE_515                     (0x1 << 11)
#define ITE_AUDIO_AMR_ENCODE_590                     (0x2 << 11)
#define ITE_AUDIO_AMR_ENCODE_670                     (0x3 << 11)
#define ITE_AUDIO_AMR_ENCODE_740                     (0x4 << 11)
#define ITE_AUDIO_AMR_ENCODE_795                     (0x5 << 11)
#define ITE_AUDIO_AMR_ENCODE_1020                    (0x6 << 11)
#define ITE_AUDIO_AMR_ENCODE_1220                    (0x7 << 11)

#define AUDIO_ERROR_BASE                             0

#define AUDIO_ERROR_ENGINE_ALIGNMENT_ERROR           (AUDIO_ERROR_BASE + 0x0001)
#define AUDIO_ERROR_ENGINE_ILLEGAL_INSTRUCTION_ERROR (AUDIO_ERROR_BASE + 0x0002)
#define AUDIO_ERROR_ENGINE_RESET_ENGINE_ERROR        (AUDIO_ERROR_BASE + 0x0002)
#define AUDIO_ERROR_ENGINE_LOAD_FAIL_ERROR           (AUDIO_ERROR_BASE + 0x0003)
#define AUDIO_ERROR_ENGINE_UNKNOW_STATUS_ERROR       (AUDIO_ERROR_BASE + 0x0009)

#define AUDIO_ERROR_ENGINE_CAN_NOT_START             (AUDIO_ERROR_BASE + 0x1001)
#define AUDIO_ERROR_ENGINE_CAN_NOT_STOP              (AUDIO_ERROR_BASE + 0x1002)
#define AUDIO_ERROR_ENGINE_DO_NOT_SUPPORT_ENCODE     (AUDIO_ERROR_BASE + 0x1003)
#define AUDIO_ERROR_ENGINE_DO_NOT_SUPPORT_DECODE     (AUDIO_ERROR_BASE + 0x1004)
#define AUDIO_ERROR_ENGINE_IS_RUNNING                (AUDIO_ERROR_BASE + 0x1005)
#define AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE        (AUDIO_ERROR_BASE + 0x1005)

#define AUDIO_ERROR_CODEC_DO_NOT_INITIALIZE          (AUDIO_ERROR_BASE + 0x1001)
#define AUDIO_ERROR_CODEC_UNKNOW_CODEC_MODE          (AUDIO_ERROR_BASE + 0x1002)
#define AUDIO_ERROR_CODEC_CAN_NOT_FOUND_ADC_RATIO    (AUDIO_ERROR_BASE + 0x1004)
#define AUDIO_ERROR_CODEC_CAN_NOT_FOUND_DAC_RATIO    (AUDIO_ERROR_BASE + 0x1005)
#define AUDIO_ERROR_CODEC_UNKNOW_ERROR               (AUDIO_ERROR_BASE + 0x1009)

#define AUDIO_ERROR_MIDI_CAN_NOT_ALLOCATE_SYS_RAM    (AUDIO_ERROR_BASE + 0x2001)
#define AUDIO_ERROR_MIDI_CAN_NOT_ALLOCATE_VRAM       (AUDIO_ERROR_BASE + 0x2002)

#define FLASH_AUDIO_INPUT                            3

/* Additional region for nand boot */
// #if defined(HAVE_NANDBOOT)
// #if defined(DEBUG)
// #define BOOTSTRAP_SIZE                       0x8000
// #else
// #define BOOTSTRAP_SIZE                       0x2000
// #endif // defined(DEBUG)
// #else
// #define BOOTSTRAP_SIZE                           0x0
// #endif

/* puts the plugin to array */
// #define CODEC_ARRAY_SIZE                             (180 * 1024) // the maximun size of each plug-ins

/* the start address of CODEC plugin */
// #define CODEC_START_ADDR                             (0x1000 + BOOTSTRAP_SIZE)

/* magic for normal codecs */
// #define CODEC_MAGIC                                  0x534D3020 // "SM0 "

/* increase this every time the api struct changes */
// #define CODEC_API_VERSION                            0x00000002

/* machine target, no used currently */
// #define TARGET_ID                                    0x00000220

/* the maximun size of codec plug-ins */
// #if defined(DEBUG)
// #define RISC1_SIZE                               (900 * 1024)
// #else
// #ifdef ENABLE_XCPU_MSGQ
// #define RISC1_SIZE                           (900 * 1024)
// #else
// #if defined(HAVE_HEAACV2)
// #define RISC1_SIZE                       (900 * 1024)  // aac plugin takes >350KB.
// #elif defined(HAVE_WMA)
// #define RISC1_SIZE                       (900 * 1024)  // eac3 plugin takes ~350KB.
// #else
// #define RISC1_SIZE                       (900 * 1024) // Others plugin take ~198KB.
// #endif
// #endif
// #endif
// #define RISC2_SIZE                       (130 * 1024)
//=============================================================================
//                              Structure Definition
//=============================================================================
//=============================
//  Type Definition
//=============================
struct codec_api_t
{
    void * eqinfo;
    void * revbinfo;
};

struct codec_header_t
{
    uint32_t  magic;
    uint16_t  target_id;
    uint16_t  api_version;
    uint8_t * load_addr;
    uint8_t * end_addr;
    int32_t (*entry_point)(void);
    uint8_t * pStream;
    int32_t (*codec_info)(void);
};

typedef struct AUDIO_CODEC_STREAM_TAG
{
    int32_t codecStreamBuf;
    int32_t codecStreamLength;
    int32_t codecStreamBuf1;
    int32_t codecStreamLength1;
    int32_t codecStreamBuf2;
    int32_t codecStreamLength2;
    int32_t codecAudioPluginBuf;
    int32_t codecAudioPluginLength;
    // int32_t codecStreamBuf3;
    // int32_t codecStreamLength3;
    // int32_t codecStreamBuf4;
    // int32_t codecStreamLength4;
    // int32_t codecStreamBuf5;
    // int32_t codecStreamLength5;
    // int32_t codecStreamBuf6;
    // int32_t codecStreamLength6;
    int32_t codecParameterBuf;
    int32_t codecParameterLength;
    int32_t codecEchoStateBuf;
    int32_t codecEchoStateLength;
} AUDIO_CODEC_STREAM;

typedef struct AUDIO_ENCODE_DATA_TAG
{
    uint32_t nTimeStamp;
    uint32_t nDataSize;
    char *   ptData;
} AUDIO_ENCODE_DATA;

typedef struct AUDIO_ENCODE_PARAM_TAG
{
    int32_t   nInputBufferType; // 0: buffer from audio encoder, 1:buffer from other RISC AP, 2 : buffer from I2S
    int32_t   nEnableMixer;     // 0: not use mixer
    uint32_t  nStartPointer;    // if nInputBufferType == 2, it can setup start pointer
    uint32_t  nStartTime;       // ms, time offset
    uint8_t * pInputBuffer;     // I2S has most 5 input buffer pointers
    uint8_t * pInputBuffer1;
    uint8_t * pInputBuffer2;
    uint8_t * pInputBuffer3;
    uint8_t * pInputBuffer4;
    uint8_t * pMixBuffer;       // buffer to be mixed
    uint32_t  nBufferLength;    // the length of the input buffer
    uint32_t  nMixBufferLength; // the length of the mix buffer
    uint32_t  nChannels;        // most support to 7.1
    uint32_t  nSampleRate;      // input sample rate
    uint32_t  nOutSampleRate;   // output sample rate;
    uint32_t  nBitrate;         // encode bitrate
    int32_t   nInputAudioType;  // 0:PCM
    int32_t   nSampleSize;      // 16 or 32, if audio type == pcm
    uint32_t (*GetRP)();        // get buffer read porinter
    uint32_t (*GetWr)();        // get buffer write porinter
    void (*SetRP)(uint32_t data);
} AUDIO_ENCODE_PARAM;
//=============================
//  Enumeration Type Definition
//=============================

//=============================
//  Type Definition
//=============================
#define MMP_AUDIO_ENCODE_SIZE        1000 // 3000
#define MMP_AUDIO_ENCODE_BUFFER_SIZE 48000

/**
 * The structure defines audio engine type
 */
typedef enum ITE_AUDIO_ENGINE_TAG
{
    ITE_MP3_DECODE         = 1,  /**< MP3 decode engine */
    ITE_AAC_DECODE         = 2,  /**< AAC decode engine */
    ITE_AACPLUS_DECODE     = 3,  /**< AAC plus decode engine */
    ITE_BSAC_DECODE        = 4,  /**< BSAC decode engine */
    ITE_WMA_DECODE         = 5,  /**< WMA decode engine */
    ITE_AMR_ENCODE         = 6,  /**< AMR encode engine */
    ITE_AMR_DECODE         = 7,  /**< AMR decode engine */
    ITE_AMR_CODEC          = 8,  /**< AMR encode/decode engine */
    ITE_MIXER              = 9,  /**< MIXER engine */
    ITE_MIDI               = 10, /**< MIDI engine */
    ITE_PCM_CODEC          = 11, /**< PCM engine */
    ITE_WAV_DECODE         = 12, /**< WAV (PCM16/PCM8/a-law/mu-law engine */
    ITE_AC3_DECODE         = 13, /**< AC3 engine */
    ITE_OGG_DECODE         = 14, /**< OGG Vorbis engine */
    ITE_AC3_SPDIF_DECODE   = 15, /**< AC3 SPDIF engine */
    ITE_FLAC_DECODE        = 16, /**< FLAC decode engine */
    ITE_SBC_CODEC          = 17, /**< SBC engine */
    ITE_MP2_ENCODE         = 18, /**< MP2 encode engine */
    ITE_AAC_ENCODE         = 19, /**< AAC encode engine */
    EXTERNAL_ENGINE_PLUGIN = 20, /**< None audio engine */
    ITE_RESERVED           = 21,
    ITE_OPUS_DECODE        = 22, /**< OGG Opus engine */
} ITE_AUDIO_ENGINE;

typedef enum ITE_AUDIO_ENGINE_ATTRIB_TAG
{
    ITE_AUDIO_ENGINE_TYPE   = 0,
    ITE_AUDIO_ENGINE_STATUS = 1
} ITE_AUDIO_ENGINE_ATTRIB;

typedef enum ITE_AUDIO_ENGINE_STATE_TAG
{
    ITE_AUDIO_ENGINE_IDLE        = 0,
    ITE_AUDIO_ENGINE_RUNNING     = 1,
    ITE_AUDIO_ENGINE_STOPPING    = 2,
    ITE_AUDIO_ENGINE_NO_RESPONSE = 3,
    ITE_AUDIO_ENGINE_UNKNOW      = 4,
    ITE_AUDIO_ENGINE_ERROR       = 9
} ITE_AUDIO_ENGINE_STATE;

/**
 * The structure defines codec parameters
 */
typedef enum ITE_AUDIO_CODEC_ATTRIB_TAG
{
    ITE_AUDIO_CODEC_NONE              = 0,
    ITE_AUDIO_CODEC_INITIALIZE        = 1,
    ITE_AUDIO_CODEC_TERMINATE         = 2,
    ITE_AUDIO_CODEC_SETVOLUME         = 3,
    ITE_AUDIO_CODEC_MODE              = 4,
    ITE_AUDIO_CODEC_ADC_RATIO         = 5,
    ITE_AUDIO_CODEC_DAC_RATIO         = 6,
    ITE_AUDIO_CODEC_OVERSAMPLING_RATE = 7
} ITE_AUDIO_CODEC_ATTRIB;

typedef enum ITE_AUDIO_CODEC_MODE_TYPE_TAG
{
    ITE_AUDIO_CODEC_ADMS_DAMS_MODE,   // ADC master / DAC master mode
    ITE_AUDIO_CODEC_ADSL_DASL_MODE,   // ADC slave  / DAC slave  mode
    ITE_AUDIO_CODEC_ADMS_DASL_MODE,   // ADC master / DAC slave  mode
    ITE_AUDIO_CODEC_ADSL_DAMS_MODE,   // ADC slave  / DAC master mode
    ITE_AUDIO_CODEC_CODEC0_MS_MODE,   // Codec 0 master mode
    ITE_AUDIO_CODEC_CODEC0_SL_MODE,   // Codec 0 slave  mode
    ITE_AUDIO_CODEC_CODEC1_MS_MODE,   // Codec 1 master mode
    ITE_AUDIO_CODEC_CODEC1_SL_MODE,   // Codec 1 slave  mode
    ITE_AUDIO_CODEC_BT_ONLY_MSB_MODE, // Bluetooth only Long  frame (MSB) mode
    ITE_AUDIO_CODEC_BT_ONLY_I2S_MODE, // Bluetooth only short frame (I2S) mode
    ITE_AUDIO_CODEC_BT_DAC_MSB_MODE,  // Bluetooth with DAC long  frame (MSB) mode
    ITE_AUDIO_CODEC_BT_DAC_I2S_MODE,  // Bluetooth with DAC short frame (I2S) mode
    ITE_AUDIO_CODEC_BT_ADC_MSB_MODE,  // ADC with Bluttooth long  frame (MSB) mode
    ITE_AUDIO_CODEC_BT_ADC_I2S_MODE   // ADC with Bluttooth short frame (I2S) mode
} ITE_AUDIO_CODEC_TYPE_MODE;

typedef enum ITE_AUDIO_BUFFER_TYPE_TAG
{
    ITE_AUDIO_INPUT_BUFFER,
    ITE_AUDIO_OUTPUT_BUFFER,
    ITE_AUDIO_MIXER_BUFFER
} ITE_AUDIO_BUFFER_TYPE;

typedef enum ITE_AUDIO_EQTYPE_TAG
{
    EQ_CLASSICAL,
    EQ_CLUB,
    EQ_LIVE,
    EQ_POP,
    EQ_ROCK,
    EQ_USERDEF
} ITE_AUDIO_EQTYPE;

typedef enum ITE_AUDIO_REVBTYPE_TAG
{
    REVB_TEST1,
    REVB_TEST2,
    REVB_TEST3
} ITE_AUDIO_REVBTYPE;

typedef enum
{
    ITE_WAVE_FORMAT_PCM       = 0, /* Microsoft PCM Format */
    ITE_WAVE_FORMAT_ALAW      = 1, /* Microsoft ALAW */
    ITE_WAVE_FORMAT_MULAW     = 2, /* Microsoft MULAW */
    ITE_WAVE_FORMAT_DVI_ADPCM = 3,
    ITE_WAVE_FORMAT_SWF_ADPCM = 4,
} ITE_WAVE_FORMAT;

typedef enum
{
    ITE_WAVE_SRATE_6000  = 0,
    ITE_WAVE_SRATE_8000  = 1,
    ITE_WAVE_SRATE_11025 = 2,
    ITE_WAVE_SRATE_12000 = 3,
    ITE_WAVE_SRATE_16000 = 4,
    ITE_WAVE_SRATE_22050 = 5,
    ITE_WAVE_SRATE_24000 = 6,
    ITE_WAVE_SRATE_32000 = 7,
    ITE_WAVE_SRATE_44100 = 8,
    ITE_WAVE_SRATE_48000 = 9
} ITE_WAVE_SRATE;

//=============================
//  Structure Type Definition
//=============================

typedef struct ITE_AUDIO_EQINFO_TAG
{
    int16_t updEQ;
    int16_t endian;
    int16_t bandcenter[16];
    int16_t dbtab[16];
} ITE_AUDIO_EQINFO;

typedef struct ITE_AUDIO_REVBINFO_TAG
{
    uint16_t updReverb;
    uint16_t endian;
    uint32_t src_gain;
    uint32_t reverb_gain;
    uint32_t cross_gain;
    uint32_t filter[2][8];
} ITE_AUDIO_REVBINFO;

/**
 * The structure defines audio attributes.
 */
typedef enum ITE_AUDIO_ATTRIB_TAG
{
    // ITE_AUDIO_DEVICE_STATUS,

    // ITE_AUDIO_STREAM_MODES,
    // ITE_AUDIO_STREAM_PCM_TIME,
    // ITE_AUDIO_STREAM_ENC_TIME,
    // ITE_AUDIO_STREAM_DEC_TIME,
    // ITE_AUDIO_STREAM_PCM_AVAIL_LENGTH,
    // ITE_AUDIO_STREAM_DEC_AVAIL_LENGTH,
    // ITE_AUDIO_STREAM_DEC_I2S_AVAIL_LENGTH,
    // ITE_AUDIO_STREAM_ENC_AVAIL_LENGTH,
    // ITE_AUDIO_STREAM_ENC_I2S_AVAIL_LENGTH,
    // ITE_AUDIO_STREAM_TYPE,
    ITE_AUDIO_STREAM_SAMPLERATE,
    // ITE_AUDIO_STREAM_BITRATE,
    ITE_AUDIO_STREAM_CHANNEL,
    // ITE_AUDIO_STREAM_FREQUENCY_INFO,
    // ITE_AUDIO_STREAM_PLAYBACKRATE,

    // ITE_AUDIO_EFFECT_EN_DRC,
    // ITE_AUDIO_EFFECT_EN_EQUALIZER,
    // ITE_AUDIO_EFFECT_EN_REVERB,
    // ITE_AUDIO_EFFECT_EN_VOICE,
    // ITE_AUDIO_EFFECT_EQ_TYPE,
    // ITE_AUDIO_EFFECT_EQ_TABLE,
    // ITE_AUDIO_EFFECT_REVERB_TYPE,
    // ITE_AUDIO_EFFECT_REVERB_TABLE,
    ITE_AUDIO_PLUGIN_PATH,
    ITE_AUDIO_GET_IS_EOF,
    ITE_AUDIO_CODEC_SET_SAMPLE_RATE,
    ITE_AUDIO_CODEC_SET_CHANNEL,
    ITE_AUDIO_CODEC_SET_BUFFER_LENGTH,
    // ITE_AUDIO_MUSIC_PLAYER_ENABLE,
    // ITE_AUDIO_PTS_SYNC_ENABLE,
    // ITE_AUDIO_ENGINE_ADDRESS,
    // ITE_AUDIO_ENGINE_LENGTH,
    ITE_AUDIO_I2S_INIT,
    ITE_AUDIO_I2S_PTR,
    // ITE_AUDIO_FADE_IN,
    // ITE_AUDIO_I2S_OUT_FULL,
    ITE_AUDIO_ENABLE_MPLAYER,
    // ITE_AUDIO_ADJUST_MPLAYER_PTS,
    ITE_AUDIO_MPLAYER_STC_READY,
    ITE_AUDIO_DECODE_ERROR,
    ITE_AUDIO_DECODE_DROP_DATA,
    ITE_AUDIO_PAUSE_STC_TIME,
    // ITE_AUDIO_IS_PAUSE,
    ITE_AUDIO_ENABLE_STC_SYNC,
    ITE_AUDIO_DIRVER_DECODE_BUFFER_LENGTH,
    // ITE_AUDIO_SPDIF_NON_PCM,
    ITE_AUDIO_FFMPEG_PAUSE_AUDIO,
    ITE_AUDIO_NULL_ATTRIB
} ITE_AUDIO_ATTRIB;

typedef struct ITE_AUDIO_DECODER_TAG
{
    ITE_AUDIO_ENGINE engineType;
    uint32_t         sampleRate;
    uint32_t         channelNum;
    uint32_t         maxBufferSize;

    // for PTS
    uint32_t         lastTimeStamp;
    uint32_t         timeStamp;

    // stc sync
    int32_t          nSTCDifferent;
    uint32_t         nChangeAudioTimeCount;
    uint32_t         nAudioBehindSTC;
    int32_t          nPreSTCAudioGap;
    uint32_t         nPauseSTCTime;
    uint32_t         nSTCStartTime;
    bool             bEngineExisted;
} ITE_AUDIO_DECODER;

typedef struct ITE_WaveInfo_TAG
{
    ITE_WAVE_FORMAT format;
    uint32_t        nChans;
    uint32_t        sampRate;
    uint32_t        bitsPerSample;
    void *          pvData;
    uint32_t        nDataSize;
} ITE_WaveInfo;

typedef struct ITE_WmaInfo_TAG
{
    uint32_t packet_size;
    int32_t  audiostream;
    uint16_t codec_id;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t bitrate;
    uint16_t blockalign;
    uint16_t bitspersample;
    uint16_t datalen;
    uint8_t  data[6];
} ITE_WmaInfo;

typedef enum ITE_AUDIO_MODE_TAG
{
    ITE_AUDIO_STEREO = 0,
    ITE_AUDIO_LEFT_CHANNEL,
    ITE_AUDIO_RIGHT_CHANNEL,
    ITE_AUDIO_MIX_LEFT_RIGHT_CHANNEL
} ITE_AUDIO_MODE;

// flash codec structure
typedef struct ITE_FlashInfo_TAG
{
    uint16_t     nAudioEnable;
    uint16_t     nFlashEnable;
    uint32_t     nInputUsing[FLASH_AUDIO_INPUT];
    ITE_WaveInfo wavInfo[FLASH_AUDIO_INPUT];
    uint16_t     nAudioFormat[FLASH_AUDIO_INPUT];
} ITE_FlashInfo;

enum
{
    AEC_CMD_NULL = 0,
    AEC_CMD_INIT,
    AEC_CMD_PROCESS,
    AEC_CMD_RESET,
    NR_CMD_INIT,
    NR_CMD_PROCESS,
    AEC_CMD_LAST
};

typedef enum ITE_FILE_EXTENSION_TAG
{
    ITE_WAV = 0,
    ITE_MP3,
    ITE_WMA,
    ITE_M4A,
    ITE_AAC,
    ITE_FLAC,
    ITE_M3U8,
    ITE_TTS,
    ITE_VORBIS,
    ITE_OPUS,
} ITE_FILE_EXTENSION_MODE;

//=============================================================================
//                              Extern Reference
//=============================================================================

//=============================================================================
//                              Function Declaration
//=============================================================================

/** @defgroup group8 ITE Audio Driver API
 *  The supported API for audio.
 *  @{
 */

/**
 * Initialize audio engine.
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Application must call this API first when it want to use audio API.
 * Before iteAudioTerminate() this API will be used once only.
 */
int32_t    iteAudioInitialize (void);

/**
 * Specifies which type audio engine will be used.
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Application must call this API to Specifies which type audio engine is needed.
 * Before iteAudioStopEngine() this API will be used once only.
 */
int32_t    iteAudioOpenEngine (ITE_AUDIO_ENGINE audio_type);

/**
 * Get available buffer length
 *
 * @param bufferType        Specifies buffer type
 * @param bufferLength      Pointer to get buffer size
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Now audio engine will play back MIDI.
 */
int32_t    iteAudioGetAvailableBufferLength (ITE_AUDIO_BUFFER_TYPE bufferType, uint32_t * bufferLength);

/**
 * Input stream data for audio engine to play back.
 *
 * @param stream                Specifies stream data buffer.
 * @param streamBufferSize      Specifies stream buffer size .
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Audio engine play back stream data, the data format is according to audio engine type chosen by
 * iteAudioOpenEngine() API.
 */
int32_t    iteAudioWriteStream (uint8_t * stream, uint32_t streamBufferSize);

int32_t    iteAudioGetFlashAvailableBufferLength (int32_t nAudioInput, uint32_t * bufferLength);
int32_t    iteAudioWriteFlashStream (int32_t nAudioInput, uint8_t * stream, uint32_t length);

/**
 * Stop audio engine.
 *
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Audio engine will stop immediately when this API is called.
 * Application can use iteAudioActiveEngine() or iteAudioActiveMidiEngine() to restart engine.
 */
int32_t    iteAudioStopEngine (void);

/**
 * Get decoding frame ID of audio engine working now.
 *
 * @return 0 if succeed, error codes of int32_t_ERROR
 *         otherwise.
 * @remark Application can use this API to know audio time form starting decoding .
 */
int32_t    iteAudioGetDecodeTime (uint32_t * p_time);

int32_t    iteAudioGetDecodeTimeV2 (uint32_t * p_time);
int32_t    iteAudioSetAttrib (const ITE_AUDIO_ATTRIB attrib, const void * value);
int32_t    iteAudioGetAttrib (const ITE_AUDIO_ATTRIB attrib, void * value);

int32_t    iteAudioPause (bool enable);

int32_t    iteAudioSTCSyncPause (bool enable);

int32_t    iteAudioSeek (bool enable);

int32_t    iteAudioStop (void);

int32_t    iteAudioStopQuick (void);

int32_t    iteAudioStopImmediately (void);

int32_t    iteAudioGenWaveDecodeHeader (ITE_WaveInfo * wavInfo, uint8_t * wave_header);

int32_t    iteAudioWriteWmaInfo ();

int32_t    iteAudioGetMusicCodecDump (void);

void       iteAudioSetMusicCodecDump (int32_t nEnable);

void       iteAudioSuspendEngine ();

uint16_t   iteAudioGetAuidoProcessorWritingStatus ();

// get audio codec api buffers
uint16_t * iteAudioGetAudioCodecAPIBuffer (uint32_t * nLength);

int32_t    iteAudioSetFlashInputStatus (int32_t nInputNumber, int32_t nFormat, int32_t nUsing);

// get audio codec buffer base address
uint32_t   iteAudioGetAudioCodecBufferBaseAddress (void);

uint32_t   iteAecCommand (uint16_t command, uint32_t param0, uint32_t param1, uint32_t param2, uint32_t param3,
                          uint32_t * value);

///////////////////////////////////////////////////////////////////////////////
//@}

// audio_mgr.c audio_mgr.h ,declare for fix build warning
/*get specturm*/
int32_t    smtkAudioMgrSetSpectrum (uint8_t * buffer);
void       iteAudioUpdateMessageQ ();
void       iteAudioCodecSetPcmIdx (int32_t index);
int32_t    iteAudioCodecGetPcmIdx (void);

#ifdef __cplusplus
}
#endif

#endif // ITE_AUDIO_H//
