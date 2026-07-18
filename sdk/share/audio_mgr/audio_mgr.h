/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * Audio manager.
 *
 * @author
 * @version 1.0
 */
#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
// #include "audioRecord.h"

#include "ite/audio.h"
#include "ite/mmp_types.h"
#include "ite/ith.h"
#include "ite/itp.h"

#include <pthread.h>

#include "i2s/i2s.h"

#define SMTK_AUDIO_MP3       1
#define SMTK_AUDIO_AAC       2
#define SMTK_AUDIO_WMA       5
#define SMTK_AUDIO_AMR       8
#define SMTK_AUDIO_WAV       12
#define SMTK_AUDIO_AC3       13

#define SMTK_AUDIO_DTS_SPDIF 14
#define SMTK_AUDIO_AC3_SPDIF 15

#define SMTK_AUDIO_FLAC      16

#define SMTK_AUDIO_SPECIAL_CASE_BUFFER_SIZE                                    \
    (CFG_AUDIO_SPECIAL_CASE_BUFFER_SIZE * 1024)

//=============================================================================
//                              Structure Definition
//=============================================================================

typedef enum SMTK_AUDIO_STATE_TAG
{
    SMTK_AUDIO_NONE = 0,
    SMTK_AUDIO_PLAY,
    SMTK_AUDIO_PAUSE,
    SMTK_AUDIO_STOP
} SMTK_AUDIO_STATE;

typedef enum SMTK_AUDIO_MODE_TAG
{
    SMTK_AUDIO_NORMAL = 0,
    SMTK_AUDIO_REPEAT
} SMTK_AUDIO_MODE;

typedef enum tagAUDIO_MGR_STATE_CALLBACK_E
{
    AUDIOMGR_STATE_CALLBACK_PLAYING_FINISH = 0,
    AUDIOMGR_STATE_CALLBACK_GET_METADATA,
    AUDIOMGR_STATE_CALLBACK_GET_LRC,
    AUDIOMGR_STATE_CALLBACK_GET_FINISH_NAME,
    AUDIOMGR_STATE_CALLBACK_PLAYING_INTERRUPT_SOUND_FINISH,
    AUDIOMGR_STATE_CALLBACK_PLAYING_SPECIAL_CASE_FINISH,
    AUDIOMGR_STATE_CALLBACK_MAX,
} AUDIO_MGR_STATE_CALLBACK_E;

typedef struct SMTK_AUDIO_PARAM_NETWORK_TAG
{
    void * pHandle; // handle of network
                    // #ifdef READ_HTTP
    int32_t (*NetworkRead)(void * Handle, char * buf, size_t * size,
                           int32_t timeout);
    size_t (*LocalRead)(void * ptr, size_t size, size_t count, FILE * fp);
    int32_t nReadLength; // the read length from network
    int32_t nType;       // audio type
    bool    bEOF;
    bool    bEOP;        // end of play
    int32_t nEOFWaitCount;
    int32_t nError;
    bool    bSeek;        // seek music 1, play from begining :0
    bool    bLocalPlay;
    bool    bSpectrum;    // show spectrum 1
    int32_t nARMDecode;   // not RISC decode
    int32_t nM4A;
    int32_t nSpecialCase; // for 907X can not reset I2S when DA & AD enable ,
                          // speical case for small size wav
    int32_t nLocalFileSize;
    char *  pFilename;    // file name to find lrc
    char *  pSpectrum;
    int32_t (*audioMgrCallback)(int32_t nCondition);
} SMTK_AUDIO_PARAM_NETWORK;

typedef struct SMTK_AUDIO_FILE_INFO_TAG
{
    uint8_t *  ptTitleUtf8;
    uint8_t *  ptArtistUtf8;
    uint8_t *  ptAlbumUtf8;
    uint8_t *  ptAttatchedPicture;
    uint32_t   nTotlaTime;
    bool       bTitle;
    bool       bArtist;
    bool       bAlbum;
    bool       bGenre;
    bool       bTools;
    bool       bYear;
    bool       bComment;
    bool       bAttatchedPicture;
    MMP_UINT32 nAttatchedPictureSize;
    bool       bSupported;
    bool       bSkipId3V2; // if it has id3 v1 ,then skip id3 v2
    int32_t    nFlag;
    bool       bUnLimitAttatchedPictureSize; // default limit picture size
} SMTK_AUDIO_FILE_INFO;

typedef struct SMTK_AUDIO_MGR_TAG
{
    MMP_CHAR *               filename;
    bool                     playing;
    bool                     pause;
    bool                     filePlay;
    bool                     bFilePlayEx;
    bool                     bNetworkPlay;
    bool                     bLocalPlay;
    bool                     destroying;
    uint32_t                 volume;
    int32_t                  nInit;
    bool                     mute;
    uint8_t *                sampleBuf;
    uint8_t *                streamBuf;  // for buffer simple play mode
    uint32_t                 streamSize; // for buffer simple play mode
    bool                     stop;
    bool                     parsing;
    int32_t                  jumpSecond; // unit : second
    int32_t                  nAudioType;
    int32_t                  nOffset;
    int32_t                  nExFileLength;
    bool                     bAudioEos;
    int32_t                  nShowSpectrum;
    int32_t                  nKeepTime; // keep eos time
    int32_t                  nAudioSeek;
    SMTK_AUDIO_MODE          mode;
    bool                     bDecrypt;  // need to decrypt or not
    bool                     bMp4Parsing;
    int32_t                  nMp4Index; // index start from 1
    int32_t                  nMp4Format;
    SMTK_AUDIO_FILE_INFO     tFileInfo;
    uint32_t                 nDriverSampleRate;
    int32_t                  nReadBufferStop;
    int32_t                  nCurrentReadBuffer;
    int32_t                  nCurrentWriteBuffer;
    SMTK_AUDIO_PARAM_NETWORK ptNetwork;
    uint32_t                 nNetwrokTagSize;
    bool                     bCheckMusicComplete;
    int32_t                  nDuration;
    int32_t                  nReading;
    int32_t                  nEnableSxaDmx;
    int32_t                  Nfilequeque;
    bool                     bInsertSound;
} SMTK_AUDIO_MGR;

//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================

void    AudioThreadFunc ();

/**
 * Audio initial function, should be called in AP initialize
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 * @see smtkAudioMgrTerminate().
 */
int32_t smtkAudioMgrInitialize (void);

/**
 * called in AP terminated
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 * @see smtkAudioMgrInitialize().
 */
int32_t smtkAudioMgrTerminate (void);

/**
 * play the audio file with the given name
 *
 * @param filename  send mp3 file name to audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 */

// check if playing,then stop
int32_t smtkAudioMgrQuickStop ();

/**
 * Stop audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 */
int32_t smtkAudioMgrStop (void);

/**
 * Pause audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrContinue().
 */
int32_t smtkAudioMgrPause (void);

/**
 * When audio player paused,this api can continue audio player.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrPause().
 */
int32_t smtkAudioMgrContinue (void);

/**
 * Set volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 */
int32_t smtkAudioMgrSetVolume (int32_t nVolume);

/**
 * Get volume of audio plyaer.
 *
 * @return volume
 */
int32_t smtkAudioMgrGetVolume (void);

/**
 * Increase volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrDecreaseVolume().
 */
int32_t smtkAudioMgrIncreaseVolume (void);

/**
 * Decrease volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrIncreaseVolume().
 */
int32_t smtkAudioMgrDecreaseVolume (void);

/**
 * Mute on
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrMuteOff().
 */
int32_t smtkAudioMgrMuteOn (void);

/**
 * Mute off
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrMuteOn().
 */
int32_t smtkAudioMgrMuteOff (void);

/**
 * Check mute param is on or not
 *
 * @return mute param
 * @see smtkAudioMgrIsMuteOn().
 */
bool    smtkAudioMgrIsMuteOn (void);

// return upnp (1) /local player (2)
// int32_t
// smtkAudioMgrGetPlayer(
//    void);

int32_t smtkAudioMgrGetTransitionState (void);

/**
 * Get state from audio player.
 * The state is play or pause or stop
 *
 * @return SMTK_AUDIO_STATE
 * @see smtkAudioMgrSetState().
 */
SMTK_AUDIO_STATE
smtkAudioMgrGetState(void);

/**
 * get the spectrum information with UINT8[20*5], get 5 frame's spectrum
 *
 * @param buffer
 * @return zero if succeed, otherwise return non-zero error codes.
 */
uint8_t * smtkAudioMgrGetSpectrum ();

/**
 * set the spectrum pointer
 *
 * @param buffer
 * @return zero if succeed, otherwise return non-zero error codes.
 */

// set spectrum pointer
int32_t   smtkAudioMgrSetSpectrum (uint8_t * buffer);

// 0:re-start spectrum, 1:pause/stop spectrum
int32_t   smtkAudioMgrPauseSpectrum (int32_t nPause);

/**
 * Get audio decoding time.
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
uint32_t  smtkAudioMgrGetTime (void);

/**
 * Set audio decoding time.
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
int32_t   smtkAudioMgrSetTime (int32_t nTime);

#if 0
/**
 * Get audio Network Music Tag Size
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
uint32_t
smtkAudioMgrGetNetworkMusicTagSize(
    void);

/**
 * Set audio Network Music Tag Size
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
void
smtkAudioMgrSetNetworkMusicTagSize(
    uint32_t nSize);
#endif

/**
 * Reset network read handle
 *
 * @return
 */
int32_t  smtkAudiomgrPlayNetworkResetHandle (void * handle);

/**
 * Set state from audio player.
 * The state is play or pause or stop
 *
 * @return SMTK_AUDIO_STATE
 * @see smtkAudioMgrGetState().
 */
void     smtkAudioMgrSetMode (SMTK_AUDIO_MODE mode);

uint32_t smtkAudioMgrSetTotalTime (int32_t nTime);

uint32_t smtkAudioMgrGetTotalTime (uint32_t * time);

/**
 * Set parsing status
 *
 */
void     smtkAudioMgrSetParsing (bool status);

// show spectrum
int32_t  smtkAudioMgrShowSpectrum (int32_t nMode);

void     smtkAudioMgrGetNetworkState (int32_t * pType, int32_t * pErr);

void     smtkAudioMgrSetNetworkError (int32_t nErr);

// Get file Info
int32_t  smtkAudioMgrGetFileInfo (SMTK_AUDIO_FILE_INFO * ptFileInfo);

// Close file Info
int32_t  smtkAudioMgrGetFileInfoClose (SMTK_AUDIO_FILE_INFO * ptFileInfo);

int32_t  smtkAudioMgrGetMetadata (SMTK_AUDIO_FILE_INFO * tgt_metadata);

/* copy lrc's pointer */
/*SMTK_AUDIO_LRC_INFO *
smtkAudioMgrCopyLrc();
*/
/*for quick play sound*/

int32_t  smtkAudioMgrPlayNetwork (SMTK_AUDIO_PARAM_NETWORK * pNetwork);
/*for inturrupt sound (video inturrupt audio)*/

bool     smtkAudioGetInsertSoundStatus (void);
void     smtkAudioSetInsertSoundStatus (bool pause);

int32_t (*smtkAudioMgrGetCallbackFunction())(int32_t);
void    smtkAudioMgrSetCallbackFunction(int32_t (*pApi)(int32_t));

/*record*/
int32_t smtkAudioMgrRecordSetLevel (int32_t nReclevel);

/*for inturrupt sound (audio inturrupt audio(.wav))*/
int32_t smtkAudioMgrSetInterruptStatus (
    int32_t nStatus); // return 1: interrupt sound playing ,return 0: interrupt
                      // sound not playing
int32_t smtkAudioMgrGetInterruptStatus (); // return 1: interrupt sound playing
                                           // ,return 0: interrupt sound not
                                           // playing
int32_t smtkAudioMgrInterruptSoundGetRemoveCard (); // nRemove :1 remove
                                                    // ,nRemove:0 finished
void    smtkAudioMgrInterruptSoundSetRemoveCard (
       int32_t nRemove); // nRemove :1 remove ,nRemove:0 finished
void    smtkAudioMgrSetInterruptOverwriteStatus (int32_t nStatus);

// 1: not overrite,only play wav
int32_t smtkAudioMgrGetInterruptOverwriteStatus ();

// input a wav file to play immediately
int32_t smtkAudioMgrInterruptSound (char * filename, int32_t overwrite,
                                    int32_t * pCallback);
// open audio engine
int32_t smtkAudioMgrOpenEngine (int32_t nEngineType);

int32_t smtkAudioMgrGetI2sPostpone ();

int32_t smtkAudioMgrSampleRate ();

int32_t smtkAudioMgrSetI2sPostpone (int32_t nValue);

int32_t smtkAudioMgrGetType ();

char *  smtkAudioMgrGetFinishName ();

// set callbakc function
void    smtkAudioMgrInterruptSoundFinish ();

// return 0:not wav hd, 1: wav hd
int32_t smtkAudioMgrWavHD (uint32_t * pBitPerSample);

int32_t smtkAudioMgrUnloadSBC ();

// set func pointer to reload sbc
void    smtkAudioMgrReloadSBC (void * func);

int32_t smtkAudioMgrRecordInitialize (void);

int32_t smtkAudioMgrRecordTerminate (void);

int32_t smtkAudioMgrRecordPause (void);

int32_t smtkAudioMgrRecordResume (void);

int32_t smtkAudioMgrRecordStop (void);

bool    mtal_pb_check_fileplayer_with_audio (void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */
