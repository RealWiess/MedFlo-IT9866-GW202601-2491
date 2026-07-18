typedef void (*Callback_t)(int state,void *arg);
#ifndef _WIN32
typedef void (*cb_sound_t)(PlaySoundCase event_id, void *arg);
typedef void (*cb_link_t)(void *arg);
void sound_playback_event_handler(PlaySoundCase nEventID, void *arg);
void CreateDetachedThread(void *func,void *arg);
void AudioStreamCancel(IteFlower *f);

//void gfSetInitLinkToFFmpeg(IteFlower **arg);
/*audioparsing.c*/
void parsingMp3IsID3v2(FILE *fd,char *inptr,int length,int *tag);
int audiomgrExtensionName(char* filename);
int audiomgrCodecType(char* filename);
int parsing_wav(FILE* fp, DataInfo* di);
int audioGetTotalTime(char* filename, DataInfo* di);
int parsingMp3Stream(char *ptr,int length,DataInfo* di);
int parsingID3V2Stream(char *ptr,int length,int *tag);
int parsing_mp3(FILE* fp, char* filename,char *ptr,int length, DataInfo* di);
void id3tagParsing(IteFilter *f,rbuf_ite *RingBuf);

/*flow_audiostream.c*/
void flow_start_sound_play(IteFlower *f,const char* filepath,PlaySoundCase mode,Callback_t func);
void flow_stop_sound_play(IteFlower *f);
void flow_start_sound_record(IteFlower *f,const char* filepath,int rate,int channel);
void flow_stop_sound_record(IteFlower *f);
#ifdef CFG_BUILD_ASR
void flow_stop_asr(IteFlower *f);
void flow_start_asr(IteFlower *f,Callback_t);
#endif
void flow_start_memory_play(IteFlower *f,uint8_t *data,size_t dlen,AudioCodecType codec);
void flow_stop_memory_play(IteFlower *f);
void flow_start_AVsound_play(IteFlower *f);
void flow_stop_AVsound_play(IteFlower *f);
void *autostop_task(IteFlower *f);


/*flow_utils.c*/
void flowQInit(IteFlower *f);
void flowQUninit(IteFlower *f);
void flowQPut(IteFlower *f,unsigned char *inbuf,int size);
mblk_ite *flowQGet(IteFlower *f);
void flowQEof(IteFlower *f);
int flowQCount(IteFlower *f);
void flowQflush(IteFlower *f);
int checkQcount(IteFlower *f,int count);
IteFlower *ItoFlowInit(int Qflag);
void ItoFlowUninit(IteFlower *f);

/*flow_reservstream.c*/
void link_callback0(void *arg);
void link_callback1(void *arg);
void FlowPlayerCancel(IteFlower *f);
void FlowPlayerFlush(IteFlower *f);
void FlowI2SCancel(IteFlower *f);
void FlowI2SCheckIdel(IteFlower *f);
void autostop_FlowI2SCancel(IteFlower *f);
void autostop_FlowRelayCancel(IteFlower *f);
void FlowI2SSetDucking(IteFlower *f);
void flowstream_start_play(IteFlower *f,const char* filepath,PlaySoundCase mode,Callback_t func);
void flowstream_stop_play(IteFlower *f);
void flowstream_mix_sound(IteFlower *f,const char* filepath, PlaySoundCase mode, Callback_t func);
void flowstream_start_AVplay(IteFlower *f);
void flowstream_stop_AVplay(IteFlower *f);
void flowstream_start_ffmpegplay(IteFlower *f,const char* filepath,Callback_t func);
void flowstream_stop_ffmpegplay(IteFlower *f);
void flowstream_start_memory_play(IteFlower *f,uint8_t *data,size_t dlen,AudioCodecType codec);
void flowstream_stop_memory_play(IteFlower *f);
void flowstream_status_set(IteFlower *f,IteFilterStatus status);
int flowstream_check_unmix(void);


/*i2s init tool*/
void flow_init_AD(void);
void flow_init_DA(void);
void dawrite_deinit_state(void);
void adread_deinit_state(void);
void Castor3snd_deinit_state(int rate,int bitsize,int channel);
int dawrite_reinit_for_diff_rate(int rate,int bitsize,int channel);
int adread_reinit_for_diff_rate(int rate,int bitsize,int channel);
int Castor3snd_reinit_for_diff_rate(int rate,int bitsize,int channel);
void flow_set_dac_level(int level);
void flow_set_adc_level(int level);
void Flow_init_i2s(IteFlower *f);

/*Qnumber controller*/
int IteAudioQueueController(IteFilter *f,int pin,int max,int min);
IteFilter *Flow_filter_new(IteFilterId id ,IteFlower **arg);
void dinfoSetValue(DataInfo *target,DataInfo *source);
void dinfoSetDefault(DataInfo *Info);
int IteFilterController(IteFilter *f);
int IteAudioQueueNum(IteFilter *f,int pin);
bool IteAudioQueueIsEmpty(IteFilter *f,int pin);

/*audiofilterflow.c*/
void resample_parm_alter(int sr,int ch,bool yesno);
void resample_speed_alter(float speed);
void resample_filter_new(IteAudioFlower **s0,IteFlower *f);
void resample_filter_ctrl_link(IteAudioFlower **s0,bool link);
void audio_flower_free(IteAudioFlower *s);

/*amusefilterflow.c*/
IteFilter *select_open_filter_new(const char* file,IteFlower *f);
int PlayFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb);
void PlayFlowStop(IteFlower *f);
void RecFlowStart(IteFlower *f,const char* filename,int rate,int channel);
void RecFlowStop(IteFlower *f);
void SndrwFlowStart(IteFlower *f,int rate,AudioCodecType type);
void SndrwFlowStop(IteFlower *f);
void PlayFlowGetTime(IteFlower *f,uint32_t *timestamp);
IteFilter *select_codec_filter_new(int type,IteFlower *f);
void PlayStreamFlowStart(IteFlower *f,DataInfo *di);
void PlayStreamFlowStop(IteFlower *f);

/*zamusefilterflow.c*/
int PlayerFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb);
void PlayerFlowStop(IteFlower *f);
void PacketFlowStart(IteFlower *f,int in_rate, int in_chan);
void PacketFlowStop(IteFlower *f);
void PlayerFlowSeekPos(IteFlower *f,int sec);
void PlayerFlowShiftPos(IteFlower *f,int sec);
void PlayerFlowGetPos(IteFlower *f,int *timestamp);
int I2SFlowStart(IteFlower *f,cb_sound_t cb);
void I2SFlowStop(IteFlower *f);
IteFilter *select_url_filter_new(const char* url,IteFlower *p,IteFlower *arg);
int CurlFlowStart(IteFlower *f,const char* url,cb_sound_t cb);
void CurlFlowStop(IteFlower *f);
int CurlMp3FlowStart(IteFlower *f,const char* url,cb_sound_t cb);
void CurlMp3FlowStop(IteFlower *f);
int FFmpegFlowStart(IteFlower *f,const char* filepath,cb_sound_t cb);
void FFmpegFlowStop(IteFlower *f);
int M3u8FlowStart(IteFlower *f,const char* url,cb_sound_t cb);
void M3u8FlowStop(IteFlower *f);
int CurlM4aFlowStart(IteFlower *f,const char* url,cb_sound_t cb);
void CurlM4aFlowStop(IteFlower *f);
int Mp3FlowStart(IteFlower *f,const char* url,cb_sound_t cb);
void Mp3FlowStop(IteFlower *f);
void StreamerFlowStart(IteFlower *f,DataInfo *di);
void StreamerFlowStop(IteFlower *f);

#if CFG_RESERVE_FILTER
//Zpoolfilterflow.c
int SoundFlowStart(IteFlower *f,SoundPoolParm *pool,PlaySoundCase mode,cb_sound_t cb);
void SoundFlowStop(IteFlower *f);
int RelayFlowStart(IteFlower *f,cb_sound_t cb);
void RelayFlowStop(IteFlower *f);
//flow_soundpool.c
int SoundPoolCreate(int num);
int SoundPoolPreLoad(int id,const char *path);
void SoundPoolRelease(void);
void flow_soundpool_play(IteFlower *f,int SPid,PlaySoundCase mode,Callback_t func);
void flow_soundpool_mix_play(IteFlower *f,int SPid,PlaySoundCase mode,Callback_t func);
void flow_soundpool_stop(IteFlower *f);
//flow_multistream.c
void RelayFlush(IteFlower *f);
void flowstream_start_quarter_play(IteFlower *f,const char* filepath,PlaySoundCase mode,Callback_t func,int pin);
void flowstream_stop_quarter_play(IteFlower *f,int pin);
void flowstream_start_quarter_streamplay(IteFlower * f, DataInfo * di,int pin);
void flowstream_stop_quarter_streamplay(IteFlower * f, int pin);
#endif
/*asrfilterflow.c*/
void AsrFlowStart(IteFlower *f,cb_sound_t cb);
void AsrFlowStop(IteFlower *f);
int ResmplePlayFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb,int out_rate,int out_channel);
void ResmplePlayFlowStop(IteFlower *f);

/*avfilterflow.c*/
void gfSetInitLinkToFFmpeg(IteFlower **arg);
int AVPlayerFlowStart(IteFlower *f);
void AVPlayerFlowStop(IteFlower *f);

/*adspfilterflow.c*/
int QRecFlowStart(IteFlower *f,int rate,int ch);
void QRecFlowStop(IteFlower *f);
mblk_ite *QWriteFilterGetData(IteFlower *f);
int QWriteFilterGetDataQSize(IteFlower *f);
int QWriteFilterGetDataPtr(IteFlower *f,unsigned char *pscr,int *len);
int DspResampleFlowStart(IteFlower *f,int in_rate,int out_rate);
void DspBTResampleFlowStart(IteFlower *f,int in_rate,int out_rate, int out_chn);
void DspResampleFlowStop(IteFlower *f);
void QReadFilterPutData(IteFlower *f,mblk_ite *m);
void QReadFilterPutData(IteFlower *f,mblk_ite *m);
void QReadFilterPutDataPtr(IteFlower *f,unsigned char *psrc,int len);
int QReadFilterGetDataQSize(IteFlower *f);
#if CFG_FADE_FILTER
void FadeFilterParmSet(IteFlower *f,PlaySoundCase fademode,int sustainms);
#endif
int MicAECFlowStart(IteFlower *f,int rate,int channel);
void MicAECFlowStop(IteFlower *f);
void HFPPacketFlowStart(IteFlower *f,int in_rate, int in_chan);
void HFPPacketFlowStop(IteFlower *f);


/*iteFlowctrl.c*/
IteFilterStatus Flow_status_get(IteFlower *f);
void Flow_status_set(IteFlower *f,IteFilterStatus status);
int FlowQ_swdB_ctrl(IteFlower *fsrc,int dB);
void FlowQ_Get_Link_pin(IteFlower *fdist ,int *array);
int FlowQ_Get_Idle_mixpin(IteFlower *fdist);
bool FlowQ_Check_MixIdle(IteFlower *fdist);
int FlowQ_A_unlink_B(IteFlower *fsrc,IteFlower *fdist);
int FlowQ_A_link_B(IteFlower *fsrc,int pin,IteFlower *fdist,int pout);
int FlowQ_link_precheck(IteFlower *fsrc,IteFlower *fdist,int sec);

/*flow_utils.c*/
void flowQInit(IteFlower *f);
void flowQUninit(IteFlower *f);
void flowQPut(IteFlower *f,unsigned char *inbuf,int size);
mblk_ite *flowQGet(IteFlower *f);
void flowQEof(IteFlower *f);
int flowQCount(IteFlower *f);
void flowQflush(IteFlower *f);
int checkQcount(IteFlower *f,int count);

/*audioqueue.c*/
/*ring buff*/
rbuf_ite *ite_rbuf_init(size_t size);
void ite_rbuf_free(rbuf_ite *m);
unsigned int ite_rbuf_get_avail_size(rbuf_ite *m);
int ite_rbuf_put(rbuf_ite *mp,unsigned char *src,int sample);
int ite_rbuf_get(unsigned char *dst,rbuf_ite *mp,int sample);
void ite_rbuf_flush(rbuf_ite *mp);
void ite_rbuf_rp_shift(rbuf_ite *mp,int shift);
void ite_rbuf_rearrange(rbuf_ite *mp);
void srcQShapePut(mblkq *q,unsigned char *inptr,int length,int resize);
/*link list*/
void mblk_init(mblk_ite *mp);
void mblkqinit(mblkq *q);
mblk_ite *dupmblk(mblk_ite* mp);
void putmblkq(mblkq *q, mblk_ite *mp);
mblk_ite *getmblkq(mblkq *q);
mblk_ite *popmblkq(mblkq *q);
void flushmblkq(mblkq *q);
int getmblkqavail(mblkq *q);
/*shape mblkQ*/
void mblkQShapeInit(mblkq *q,int bufsize);
void mblkQShapeFlush(mblkq *q);
void mblkQShapeUninit(mblkq *q);
void mblkQShapePut(mblkq *q,mblk_ite *m,int resize);
void srcQShapePut(mblkq *q,unsigned char *inptr,int length,int resize);
void mblkvolctrl(mblk_ite *m,float gain);
void ite_queue_put_scilent(IteQueueblk *blk,IteFilter *f,int pin,int size);
void ite_queue_put_from_mblkQ(IteQueueblk *blk,IteFilter *f,int pin,mblkq *tmpQ);
void ite_queue_put_to_mblkQShape(IteQueueblk *blk,IteFilter *f,int pin,mblkq *tmpQ,int resize);

/*streamapi.c*/
void streamQInit(int cotype);
void streamQUninit(void);
void streamQPut(unsigned char *inbuf,int size);
void streamQPutShape(unsigned char *inbuf,int size,int chopsize);
void streamQmblkPut(mblk_ite *im);
mblk_ite *streamQGet(void);
void streamQEof(void);
int streamQCount(void);
void streamQflush(void);
//void streamSetCodec(ITE_AUDIO_ENGINE cotype);
//int streamGetCodec(void);

/*flower_stream.c*/
IteFlower *IteStreamInit(void);
void IteStreamUninit(IteFlower *f);
#endif
