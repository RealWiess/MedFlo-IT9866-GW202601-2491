#ifndef LEAF_FLOWER_H
#define LEAF_FLOWER_H

#define IPADDR_SIZE 64
#define bool _Bool

#include "flower/flower.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _LeafCallDir {
	CallIncoming,  /**< incoming calls*/
	CallOutgoing   /**< outgoing calls*/	
}LeafCallDir;

typedef enum _LeafCallState{
	LeafCallIdle,					/**<Initial call state */
	LeafCallIncomingReceived, /**<This is a new incoming call */
	LeafCallOutgoingInit, /**<An outgoing call is started */
	LeafCallOutgoingProgress, /**<An outgoing call is in progress */
	LeafCallOutgoingRinging, /**<An outgoing call is ringing at remote end */
	LeafCallOutgoingEarlyMedia, /**<An outgoing call is proposed early media */
	LeafCallConnected, /**<Connected, the call is answered */
	LeafCallStreamsRunning, /**<The media streams are established and running*/
	LeafCallPausing, /**<The call is pausing at the initiative of local end */
	LeafCallPaused, /**< The call is paused, remote end has accepted the pause */
	LeafCallResuming, /**<The call is being resumed by local end*/
	LeafCallRefered, /**<The call is being transfered to another party, resulting in a new outgoing call to follow immediately*/
	LeafCallError, /**<The call encountered an error*/
	LeafCallEnd, /**<The call ended normally*/
	LeafCallPausedByRemote, /**<The call is paused by remote end*/
	LeafCallUpdatedByRemote, /**<The call's parameters are updated, used for example when video is asked by remote */
	LeafCallIncomingEarlyMedia, /**<We are proposing early media to an incoming call */
	LeafCallUpdated, /**<The remote accepted the call update initiated by us */
	LeafCallReleased /**< The call object is no more retained by the core */
} LeafCallState;

typedef struct _LeafCall
{
    LeafCallDir dir;
    char localip[IPADDR_SIZE]; /* local ipaddress for this call */
    LeafCallState   state;
    IteFlower *stream_flow;
    int audio_port;
    int video_port;
	bool VideomemoRecording;
}LeafCall;

LeafCall* leaf_init(void);
uint8_t leaf_uninit(LeafCall *call);
uint8_t leaf_init_video_streams (LeafCall *call, unsigned short port);
uint8_t leaf_init_audio_streams (LeafCall *call, unsigned short port);
uint8_t leaf_start_video_stream(LeafCall *call, Call_info *call_list, unsigned short port);
uint8_t leaf_start_audio_stream(LeafCall *call, Call_info *call_list, unsigned short port);
uint8_t leaf_stop_media_streams(LeafCall *call);
uint8_t leaf_take_video_snapshot(LeafCall *call, char *file,FfilewriterCallback func );
bool leaf_video_memo_is_recording(LeafCall *call);
void leaf_start_video_memo_record(LeafCall *call, char *file, int width, int height, int fps);
void leaf_start_video_memo_partial_record(LeafCall *call, char *file, int width, int height, int fps);
void leaf_stop_video_memo_record(LeafCall *call);
void leaf_start_ipcam_stream(LeafCall *call, const char *addr, int port);
void leaf_stop_ipcam_stream(LeafCall *call);

#ifdef __cplusplus
}
#endif

#endif
