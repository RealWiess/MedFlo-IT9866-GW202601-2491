#include <malloc.h>
#include <string.h>
#include "flower/flower.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
extern IteFilterDes FilterA;
extern IteFilterDes FilterB;
extern IteFilterDes FilterC;
extern IteFilterDes FilterD;
extern IteFilterDes FilterE;
extern IteFilterDes FilterF;
#if defined(CFG_NET_ENABLE)
extern IteFilterDes FilterUDPsend;
extern IteFilterDes FilterUDPrecv;
extern IteFilterDes FilterTCPsend;
extern IteFilterDes FilterTCPrecv;
extern IteFilterDes FilterCurl;
#endif
#if defined(CFG_VIDEO_ENABLE)
extern IteFilterDes FilterH264DEC;
extern IteFilterDes FilterMJpegDEC;
extern IteFilterDes FilterDisplay;
extern IteFilterDes FilterDisplayDual;
extern IteFilterDes FilterDisplayOSD;
extern IteFilterDes FilterNV12JpgEnc;
extern IteFilterDes FilterJpegWriter;
extern IteFilterDes FilterFileWriter;
extern IteFilterDes FilterRecAVI;
extern IteFilterDes FilterMJpegRecAVI;
extern IteFilterDes FilterIPCam;
#endif
#if defined(CFG_CAPTURE_ENABLE)
extern IteFilterDes FilterCapture;
#endif
#if defined(CFG_UVC_ENABLE)
extern IteFilterDes FilterUVC;
#endif
#ifdef CFG_AUDIO_ENABLE
extern IteFilterDes FilterDAWtire;
extern IteFilterDes FilterADRead;
extern IteFilterDes FilterPcmGrab;
extern IteFilterDes FilterMgrGrab;
extern IteFilterDes FilterFileRec;
extern IteFilterDes FilterVoid;
extern IteFilterDes FilterTee;
extern IteFilterDes FilterChadapt;
extern IteFilterDes FilterQLay;
extern IteFilterDes FilterQWrite;
extern IteFilterDes FilterQRead;
extern IteFilterDes FilterCodecMgr;
    #if CFG_FADE_FILTER
    extern IteFilterDes FilterFade;
    #endif
    #if CFG_BUILD_AUDIO_PREPROCESS
extern IteFilterDes FilterLoopBack;
extern IteFilterDes	FilterDenoise;
extern IteFilterDes FilterAec;
extern IteFilterDes FilterStretch;
    #endif
    #if CFG_BUILD_ASR
extern IteFilterDes FilterAsr;
    #endif
    #if CFG_BUILD_SPEEX
extern IteFilterDes FilterResample;
//extern IteFilterDes FilterFFT;
    #endif
    #if CFG_BUILD_FFMPEG
extern IteFilterDes FilterM4aGrab;
extern IteFilterDes FilterDeM4A;
extern IteFilterDes FilterFfmpegGrab;
    #endif
    #if CFG_AV_MIX_FILTER
extern IteFilterDes FilterPcmpool;
    #endif
    #if CFG_BUILD_MP3DEC
extern IteFilterDes FilterMp3Grab;
extern IteFilterDes FilterCodecMp3;
    #endif
    #if CFG_BUILD_OPUS
//extern IteFilterDes FilterOpusDec;
//extern IteFilterDes FilterOpusEnc;
extern IteFilterDes FilterOpusGrab;
extern IteFilterDes FilterCodecOpus;
    #endif
    #if CFG_BUILD_VORBISDEC
extern IteFilterDes FilterVorbisGrab;
    #endif
    #if CFG_I2S_HDMI_RX_CLK
extern IteFilterDes FilterSndRx;
    #endif 
    #if CFG_BUILD_AACDEC
extern IteFilterDes FilterAACGrab;
extern IteFilterDes FilterCodecAAC;
    #endif
    
    #if CFG_RESERVE_FILTER
    extern IteFilterDes FilterMultiMix;
    extern IteFilterDes FilterFdGrab;
    #endif
#endif

// Filter Set
IteFilterDes *gFilterDesSet[] = {
    &FilterA,
    &FilterB,
    &FilterC,
    &FilterD,
    &FilterE,
    &FilterF,
    #if defined(CFG_NET_ENABLE)    
    &FilterUDPsend,
    &FilterUDPrecv,
    &FilterTCPsend,
    &FilterTCPrecv,
    &FilterCurl,
    #endif
    #if defined(CFG_VIDEO_ENABLE)
	&FilterH264DEC,
	&FilterMJpegDEC,
	&FilterDisplay,
	&FilterDisplayDual,
	&FilterNV12JpgEnc,
	&FilterJpegWriter,
	&FilterFileWriter,
	&FilterRecAVI,
	&FilterMJpegRecAVI,
	&FilterIPCam,
    #endif
    #if defined(CFG_CAPTURE_ENABLE)
	&FilterCapture,
    #endif
    #if defined(CFG_TIME_FRAME_FUN_ENABLE)
    &FilterDisplayOSD,
    #endif
    #if defined(CFG_UVC_ENABLE)
	&FilterUVC,
    #endif
#ifdef CFG_AUDIO_ENABLE
    &FilterDAWtire,
    &FilterADRead,
    &FilterPcmGrab,
    &FilterMgrGrab,
    &FilterFileRec,
    &FilterVoid,
    &FilterTee, 
    &FilterChadapt,
    &FilterQLay,
	//&FilterMix,
    &FilterQWrite,
    &FilterQRead,
    &FilterCodecMgr,
    #if CFG_FADE_FILTER
    &FilterFade,
    #endif
    #if CFG_BUILD_AUDIO_PREPROCESS
    &FilterLoopBack,
    &FilterAec,
    &FilterDenoise,
    &FilterStretch,
    #endif
    #if CFG_BUILD_ASR
    &FilterAsr,
    #endif
    #if CFG_BUILD_SPEEX
    &FilterResample,
    //&FilterFFT,
    #endif
    #if CFG_BUILD_FFMPEG
    &FilterM4aGrab,
    &FilterDeM4A,
    &FilterFfmpegGrab,
    #endif
    #if CFG_AV_MIX_FILTER
    &FilterPcmpool,
    #endif
    #if CFG_BUILD_MP3DEC
    &FilterMp3Grab,
    &FilterCodecMp3,
    #endif
    #if CFG_BUILD_OPUS
    //&FilterOpusDec,
    //&FilterOpusEnc,
    &FilterOpusGrab,
    &FilterCodecOpus,
    #endif
    #if CFG_BUILD_VORBISDEC
    &FilterVorbisGrab,
    #endif
    #if CFG_BUILD_AACDEC
    &FilterAACGrab,
    &FilterCodecAAC,
    #endif
    #if CFG_I2S_HDMI_RX_CLK
    &FilterSndRx,
    #endif
    #if CFG_RESERVE_FILTER
    &FilterMultiMix,
    &FilterFdGrab,
    #endif
#endif
    NULL
};



