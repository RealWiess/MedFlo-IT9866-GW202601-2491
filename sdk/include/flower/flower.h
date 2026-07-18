/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * Flower.
 *
 * @author
 * @version 1.0
 */
#ifndef FLOWER_H
#define FLOWER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#ifndef _WIN32
#include "flower/ite_queue.h"
#include "flower/ite_buffer.h"
#include "flower/fliter_priv_def.h"
#include "ite/audio.h"

#define DEBUG_PRINT(...)
//#define DEBUG_PRINT     printf
#define STREAM_QUEUE_SIZE      32

#define ite_new(type,count)    (type*)malloc(sizeof(type)*(count))
#define ite_new0(type,count)   (type*)calloc(1,sizeof(type)*(count))
#define ARRAY_COUNT_OF(array)            (sizeof(array) / sizeof(array[0]))

typedef struct _IteFilter IteFilter;
typedef void (*FilterFunc)(IteFilter *f);
typedef void (*MethodFunc)(IteFilter *f, void *arg);

//=============================================================================
//                              Structure Definition
//=============================================================================

/**
 * Filter ID
 */
typedef enum _IteFilterId {
    ITE_FILTER_A_ID = 1,
    ITE_FILTER_B_ID,
    ITE_FILTER_C_ID,
    ITE_FILTER_D_ID,
    ITE_FILTER_E_ID,
    ITE_FILTER_F_ID,
    ITE_FILTER_ADREAD_ID,
    ITE_FILTER_DAWRITE_ID,
    ITE_FILTER_PCMGRAB_ID,
    ITE_FILTER_MGRGRAB_ID,
    ITE_FILTER_MP3GRAB_ID,
    ITE_FILTER_M4AGRAB_ID,
	ITE_FILTER_PCMPOOL_ID,
    ITE_FILTER_FILEREC_ID,
    ITE_FILTER_AEC_ID,
    ITE_FILTER_ASR_ID,
	ITE_FILTER_CAP_ID,
	ITE_FILTER_H264DEC_ID,
	ITE_FILTER_MJPEGDEC_ID,
    ITE_FILTER_DISPLAY_ID,
    ITE_FILTER_DISPLAY_DUAL_ID,
    ITE_FILTER_DISPLAY_OSD_ID,
    ITE_FILTER_NV12_JPGENC_ID,
    ITE_FILTER_JPEG_WRITER_ID,
    ITE_FILTER_FILE_WRITER_ID,
    ITE_FILTER_REC_AVI_ID,
    ITE_FILTER_MJPEG_REC_AVI_ID,
    ITE_FILTER_UVC_ID,
    ITE_FILTER_IPCAM_ID,
    ITE_FILTER_UDP_SEND_ID,
    ITE_FILTER_UDP_RECV_ID,
    ITE_FILTER_TCP_SEND_ID,
    ITE_FILTER_TCP_RECV_ID,
    ITE_FILTER_VOID_ID,
    ITE_FILTER_TEE_ID,
    ITE_FILTER_DENOISE_ID,
    ITE_FILTER_VOLUME_ID,
    ITE_FILTER_RESAMPLE_ID,
    //ITE_FILTER_FFT_ID,
    ITE_FILTER_CHADAPT_ID,
    ITE_FILTER_LOOPBACK_ID,
    ITE_FILTER_QWRITE_ID,
    ITE_FILTER_QREAD_ID,
    ITE_FILTER_SNDRX_ID,
    ITE_FILTER_CURL_ID,
    ITE_FILTER_CODECMP3_ID,
    ITE_FILTER_AACGRAB_ID,
    ITE_FILTER_CODECAAC_ID,
    //ITE_FILTER_CODECOPUSDE_ID,
    //ITE_FILTER_CODECOPUSEN_ID,
    ITE_FILTER_VORBISGRAB_ID,
    ITE_FILTER_OPUSGRAB_ID,
    ITE_FILTER_CODECOPUS_ID,
    ITE_FILTER_DEM4A_ID,
    ITE_FILTER_FFMPEGGRAB_ID,
    ITE_FILTER_QLAYE_ID,
    ITE_FILTER_CODECMGR_ID,
    ITE_FILTER_STRETCH_ID,
    ITE_FILTER_FADE_ID,
    #if CFG_RESERVE_FILTER
    ITE_FILTER_MULTIMIX_ID,
    ITE_FILTER_FDGRAB_ID,
    #endif
} IteFilterId;

/**
 * Method fucntion ID
 */
typedef enum _IteMethodId {
    ITE_FILTER_A_Method = 1,
    ITE_FILTER_B_Method,
    ITE_FILTER_C_Method,
    ITE_FILTER_D_Method,
    ITE_FILTER_E_Method,
    ITE_FILTER_F_Method,
    /*video*/
	ITE_FILTER_CAP_GETFRAMERATE,
	ITE_FILTER_CAP_GETINTERLANCED,
	ITE_FILTER_CAP_GETERROR,
	ITE_FILTER_CAP_SETSENSORINIT,
	ITE_FILTER_CAP_GETSENSORSTABLE,
	ITE_FILTER_JPEG_SNAPSHOT,
	ITE_FILTER_REC_AVI_OPEN,
	ITE_FILTER_REC_AVI_OPEN_PARTIAL,
	ITE_FILTER_REC_AVI_CLOSE,
	ITE_FILTER_NV12_PUSH_PKG,
	ITE_FILTER_MJPEG_REC_AVI_OPEN,
	ITE_FILTER_MJPEG_REC_AVI_OPEN_PARTIAL,
	ITE_FILTER_MJPEG_REC_AVI_CLOSE,
	ITE_FILTER_UVC_SETTING,
	ITE_FILTER_UVC_GETOPENED,
    ITE_FILTER_UVC_GETCONNECTED,
	ITE_FILTER_DISPLAY_SET_INPUT_WINDOWS,
    /*audio*/
    //ITE_FILTER_PLAYEROPEN,
    ITE_FILTER_SET_FILEPATH,
    ITE_FILTER_LOOPPLAY,
    ITE_FILTER_FILEOPEN,
    ITE_FILTER_FILECLOSE,
    ITE_FILTER_SEEK_POS,
    ITE_FILTER_SHIFT_POS,
    ITE_FILTER_PAUSE,
    ITE_FILTER_SETRATE,
    ITE_FILTER_GETRATE,
    ITE_FILTER_SET_CB,
    ITE_FILTER_SET_VALUE,
    ITE_FILTER_GET_VALUE,
    ITE_FILTER_SET_BYPASS,
    ITE_FILTER_INPUT_RATE,
    ITE_FILTER_OUTPUT_RATE,
    ITE_FILTER_SET_NCHANNELS,
    ITE_FILTER_GET_NCHANNELS,
    ITE_FILTER_GET_STATUS,
    ITE_FILTER_SET_GAIN,
    ITE_FILTER_SET_PARAM,
    ITE_FILTER_GET_PARAM,
    /*udp*/
    ITE_FILTER_UDP_SEND_SET_PARA,
    ITE_FILTER_UDP_SEND_GET_SOCKET,
    ITE_FILTER_UDP_RECV_SET_PARA,
    ITE_FILTER_UDP_RECV_GET_SOCKET,
	/*tcp*/
    ITE_FILTER_TCP_SEND_SET_PARA,
    ITE_FILTER_TCP_SEND_GET_SOCKET,
    ITE_FILTER_TCP_RECV_SET_PARA,
    ITE_FILTER_TCP_RECV_GET_SOCKET,
    /*end*/
}IteMethodId;

/**
 * Fiter Status
 */
typedef enum _IteFilterStatus {
    FILTER_ERR = 0,
    FILTER_NONE,
    FILTER_RESUME,
    FILTER_PAUSE,
    FILTER_FLUSH
}IteFilterStatus;

typedef enum _IteAudioCase {
    AudioIdle=0,
    AudioPlayFile,
    AudioRecFile,
    AudioNetCom,
    AnalogNetCom,
    AnalogLocalCom,
    AnalogLocalBroadcast,
    AudioBroadcastSend,
    AudioBroadcastRecv,
    AResamplePlayFile,
    StreamPlay,
	AudioQuickPlay,
    //AudioMicMixer,
	AudioMixerFile,
	AudioMixerPacket,
    AudioAVPlay,
    AudioFFmpegPlay,
    AudioNetM3u8,
    AudioNetCurl,
    AudioMixer,
    AudioRelay,
}IteAudioCase;

/**
 * Method fucntion description
 */
struct _IteMethodDes {
    IteMethodId id;
    MethodFunc method_func;
};
typedef struct _IteMethodDes IteMethodDes;

/**
 * Filter function description
 */
struct _IteFilterDes {
    IteFilterId id;
    FilterFunc init;
    FilterFunc uninit;
    FilterFunc process;
    IteMethodDes *method;
};
typedef struct _IteFilterDes IteFilterDes;

/**
 * Filter's input & output data structure
 */
struct _IteFilterParm {
    QueueHandle_t Qhandle;
    sem_t semHandle;
};
typedef struct _IteFilterParm IteFilterParm;

/**
 * Filter definition
 */
struct _IteFilter {
    IteFilterParm input[2];
    IteFilterParm output[2];
    IteFilterDes filterDes;
    pthread_t tID;
    pthread_mutex_t tMutex;
    pthread_cond_t tCond;
	size_t pthread_stacksize;
    bool run;
    IteFilterStatus status;
    bool inputSemBind;
    void *data;
    void *srcFlow;
};

/**
 * Filter chain list
 */
struct _IteFChain {
    IteFilter *filter;
    struct _IteFChain *nextPtr;
};
typedef struct _IteFChain IteFChain;

/**
 * Filter chain's configure
 */
struct _IteFcConf {
    char *name;
    bool Semaphore;
    int QSize;
};
typedef struct _IteFcConf IteFcConf;

/**
 * Flow list definition
 */
struct _IteFlowerList {
    IteFChain *fc;
    IteFcConf config;
};
typedef struct _IteFlowerList IteFlowerList;

typedef struct _IteAudioFlower {
    IteFilter *Fsndwrite;
    IteFilter *Fsndread;
    IteFilter *Fsource;
    IteFilter *Fdestinat;
	IteFilter *Frec_avi;
    IteFilter *Fmix;
    IteFilter *Faec;
    IteFilter *Fdecoder;
    IteFilter *Fencoder;
    IteFilter *Fasr;
    IteFilter *Fudpsend;
    IteFilter *Fudprecv;
	IteFilter *Ftcpsend;
	IteFilter *Ftcprecv;
	IteFilter *Ftee;
	IteFilter *Fsrc; //for resample
    IteFChain fc;
    IteFChain fcc;
	IteFChain fccc;
}IteAudioFlower;

typedef struct _IteVideoFlower {
	IteFilter *Fcap;
    IteFilter *Fsource;
    IteFilter *Fdestinat;
	IteFilter *Fh264dec;
	IteFilter *Fdisplay;
	IteFilter *Fdisplaycam;
	IteFilter *Fjpegwriter;
	IteFilter *Ffilewriter;
	IteFilter *Frec_avi;
	IteFilter *Fipcam;
	IteFilter *Fudprecv;
	IteFilter *Ftcprecv;
    IteFilter *Fuvc;
    IteFilter *Fjpegdec;
    IteFChain fc;
}IteVideoFlower;

typedef struct _DataInfo{
    int sr;
    int nch;
    int bitsize;
    int codectype;
    int bytes20ms;
    unsigned int duration;
	unsigned int currentms;
    int sw_dB;
    #if CFG_CHANNEL_SELECT
    int ch_select;
    #endif
    bool init;
}DataInfo;

typedef struct _dBctrl{
    float curdB;
    float tardB;
    float predB;
    float gap;
    float alpha;
    int   delayms;
}dBctrl;

#if CFG_RESERVE_FILTER
typedef struct _fQueueData{
    pthread_mutex_t mutex;
    mblkq dataQ;
    int startcount;
    bool flagEof;
}fQueueData;

typedef struct _SoundPoolParm{
    FILE *fd;
    int id;
    DataInfo info;
    unsigned int priotiry;
    unsigned int data;
    void *datap;
}SoundPoolParm;
#endif

typedef struct _IteFlower{
    IteAudioFlower *audiostream;
    IteAudioFlower *asrstream;
    IteVideoFlower *videostream;
	IteVideoFlower *recordstream;
    #if CFG_RESERVE_FILTER
    fQueueData fQ;//Q
    void *userp;
    #endif
    IteAudioCase audiocase;
    DataInfo dInfo;
    DataInfo mixInfo;
    void (*cb_func)(int,void*);
    void (*cb_link)(void *);
}IteFlower;

typedef struct
{
    bool call_ready;
    bool call_start;
    bool call_end;
    char call_ip[16];

}Call_info;

typedef enum _VideoStreamDir {
    VideoStreamSendRecv,
    VideoStreamSendOnly,
    VideoStreamRecvOnly
} VideoStreamDir;
typedef enum _PlaySoundCase{
    /*play mode*/
    Normal = -1,
    Repeat,
    Scilent,
    /*flow queue flag*/
    Eofsound,
    Eofmixsound,
    Eofclose,
    /*ASR*/
    Asrevent,
    ASR_FAIL,
    ASR_SUCCESS_ARG,
    #if CFG_FADE_FILTER
    /*FADE*/
    FADEIN,
    FADEOUT,
    #endif
    /*Quick play file*/
    MIX_EOF_FILE,
	PLAY_EOF_FILE=Eofclose,
}PlaySoundCase;

typedef enum _AudioCodecType{
    PCM = 1,
    ALAW,
    ULAW
}AudioCodecType;

/*asr tag*/
typedef struct _asrStruct{
    int index;
    int rs;
    char* text;
    float score;
}asrStruct;

typedef struct _VideoMemoInfo
{
	char videomemo_file[256];
	int video_width;
	int video_height;
	int video_fps;
}VideoMemoInfo;


typedef struct _VideoInputWindowsInfo
{
	int x_offset;
	int y_offset;
	int width;
	int height;
	void* nextptr;     // pointer next windows information
}VideoInputWindowsInfo;


//=============================================================================
//                              Global Data Definition
//=============================================================================

extern IteFilterDes *gFilterDesSet[];


//=============================================================================
//                              Public Function Definition
//=============================================================================
typedef void (*FfilewriterCallback)(void *arg);

/**
 * Malloc a new filter.
 *
 * @param id The new filter ID
 * @return Filter
 */
IteFilter *ite_filter_new(IteFilterId id);

/**
 * Free a filter
 *
 * @param f The filter you want to free
 */
void ite_filter_delete(IteFilter *f);

/**
 * Call filter's method function
 *
 * @param f The filter
 * @param MId The method function ID
 * @param arg The method function's argument
 */
void ite_filter_call_method(IteFilter *f, uint32_t MId, void *arg);

/**
 * Sets when all input of filter use the same semaphore
 *
 * @param f The filter
 */
void ite_filter_set_semBind(IteFilter *f);

/**
 * Sets the status of filter
 *
 * @param f The filter
 * @param status The status of filter
 */
void ite_filter_set_status(IteFilter *f, IteFilterStatus status);

/**
 * Sets filter chain configure
 *
 * @param fc The filter chain
 * @param param_size The number of configure parameter
 * @param param The configure parameter
 */
void ite_filterChain_setConfig(IteFChain *fc, int param_size, char **param);

/**
 * Build the filter chain
 *
 * @param helper The filter chain
 * @param name The filter chain name
 */
void ite_filterChain_build(IteFChain *helper, char *name);

/**
 * Print all filter in the filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_print(IteFChain *helper);

/**
 * Stop all threads of filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_stop(IteFChain *helper);

/**
 * Delete the filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_delete(IteFChain *helper);


/**
 * Link filter to filter chain
 *
 * @param bee The inf filter's thread run after filter chain run all filter threads.
 * @param helper the filter chain
 * @param outf The filter that want to send output data to @inf
 * @param preFoutPin The output Pin number of filter in filter chain
 * @param inf The filter that want to receive input data from @outf
 * @param inputPin The input Pin number of filter you want to add to link
 */
void ite_filterChain_A_link_B(bool bee, IteFChain *helper, IteFilter *outf, int preFoutPin, IteFilter *inf, int inputPin);

/**
 * Link filter to filter chain
 *
 * @param helper the filter chain
 * @param preFoutPin The output Pin number of filter in filter chain
 * @param f The filter you want to link
 * @param inputPin The input Pin number of filter you want to add to link
 */
void ite_filterChain_link(IteFChain *helper, int preFoutPin, IteFilter *f, int inputPin);

/**
 * Unlink filter from filter chain
 *
 * @param bee The inf filter stop thread before filter chain stop all filter threads
 * @param helper the filter chain
 * @param outf the filter that output data to @inf
 * @param preFoutPin The output Pin number of filter
 * @param inf The filter that input data from @outf
 * @param inputPin The input Pin number of filter you want to unlink
 */
void ite_filterChain_A_unlink_B(bool bee, IteFChain *helper, IteFilter *outf, int preFoutPin, IteFilter *inf, int inputPin);

/**
 * Unlink filter from filter chain
 *
 * @param helper the filter chain
 * @param preFoutPin The output Pin number of filter
 * @param f The filter you want to unlink
 * @param inputPin The input Pin number of filter you want to unlink
 */
void ite_filterChain_unlink(IteFChain *helper, int preFoutPin, IteFilter *f, int inputPin);

/**
 * Create all threads in filter chain to run
 *
 * @param helper The filter chain
 */
void ite_filterChain_run(IteFChain *helper);

/**
 * Set the status of all threads in filter chain
 *
 * @param helper The filter chain
 * @param status The status you want to set
 */
void ite_filterChain_setStatus(IteFChain *helper, IteFilterStatus status);

/**
 * Get the status of all threads in filter chain
 *
 * @param helper The filter chain
 * @return The status of filter chain
 */
IteFilterStatus ite_filterChain_getStatus(IteFChain *helper);

/**
 * Init flower list
 *
 */
void ite_flower_init(void);

/**
 * Reset flower list
 *
 */
void ite_flower_reset(void);


/**
 * Add a filter chain in flower list
 *
 * @param fc The filter chain
 * @param name The filter chain name
 */
void ite_flower_add(IteFChain *fc, char *name);

/**
 * Get queue size of filter chain
 *
 * @param fc The filter chain
 * @return Queue size
 */
int ite_flower_findFChainQSize(IteFChain *fc);

/**
 * Get semaphore option of filter chain
 *
 * @param fc The filter chain
 * @return if use semaphore or not
 */
bool ite_flower_findFChainUseSem(IteFChain *fc);

/**
 * Get configure of filter chain
 *
 * @param fc The filter chain
 * @return The filter chain configure
 */
IteFcConf *ite_flower_findFChainConfig(IteFChain *fc);

/**
 * Print all filter chains in flower list
 *
 * @param f The filter
 */
void ite_flower_print(void);

/**
 * Remove a filter chain from the flower list
 *
 * @param fc The filter chain
 */
void ite_flower_delete(IteFChain *fc);

/**
 * Deinit flower list
 *
 */
void ite_flower_deinit(void);
#endif

/*flower_stream*/
#include "audio_def.h"

#ifndef _WIN32
int flow_start_udp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir);
int flow_start_tcp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir);
int flow_start_udp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port);
int flow_start_tcp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port);
int flow_stop_udp_videostream(IteFlower *f, VideoStreamDir dir);
int flow_stop_tcp_videostream(IteFlower *f, VideoStreamDir dir);
int flow_stop_udp_audiostream(IteFlower *f);
int flow_stop_tcp_audiostream(IteFlower *f);
int flow_start_recv_ipcamstream(IteFlower *f);
int flow_stop_recv_ipcamstream(IteFlower *f);
int flow_start_camera_show(IteFlower *f, int ch, VideoInputWindowsInfo* w_info);
void flow_stop_camera_show(IteFlower *f);
int flow_start_camera_with_osd(IteFlower *f, int ch);
void flow_stop_camera_with_osd(IteFlower *f);
int flow_start_camera_with_osd_record(IteFlower *f, char* file_path, int width, int height, int fps);
int flow_start_camera_with_osd_record_split(IteFlower *f, char* file_path, int width, int height, int fps);
void flow_stop_camera_with_osd_record(IteFlower *f);
bool flow_polling_camera_status(IteFlower *f);


#ifdef CFG_UVC_ENABLE
int flow_start_uvc_yuv(IteFlower *f, int width, int height, int fps);
int flow_stop_uvc_yuv(IteFlower *f);
int flow_start_uvc_mjpeg(IteFlower *f, int width, int height, int fps);
int flow_stop_uvc_mjpeg(IteFlower *f);
int flow_start_uvc_mjpeg_record(IteFlower *f, char* file_path, int width, int height, int fps);
void flow_stop_uvc_mjpeg_record(IteFlower *f);
int flow_start_uvc_h264(IteFlower *f, int width, int height, int fps);
int flow_stop_uvc_h264(IteFlower *f);
int flow_start_uvc_h264_record(IteFlower *f, char* file_path, int width, int height, int fps);
void flow_stop_uvc_h264_record(IteFlower *f);
bool flow_polling_uvc_opened(IteFlower *f);
bool flow_polling_uvc_connected(IteFlower *f);
#endif
#endif

/**/
#ifdef __cplusplus
}
#endif

#endif /* ITE_STREAMER_H */
