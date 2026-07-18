/**
 * @file lv_ffmpeg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <pthread.h>
#include "lv_ffmpeg_private.h"
#if LV_USE_FFMPEG != 0
#include "../../draw/lv_image_decoder_private.h"
#include "../../draw/lv_draw_buf_private.h"
#include "../../core/lv_obj_class_private.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
//#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include "ith/ith_video.h"
#include "isp/mmp_isp.h"

/*********************
 *      DEFINES
 *********************/

#define DECODER_NAME    "FFMPEG"

#if LV_COLOR_DEPTH == 8
    #define AV_PIX_FMT_TRUE_COLOR PIX_FMT_RGB8
#elif LV_COLOR_DEPTH == 16
    #define AV_PIX_FMT_TRUE_COLOR PIX_FMT_RGB565LE
#elif LV_COLOR_DEPTH == 32
    #define AV_PIX_FMT_TRUE_COLOR PIX_FMT_BGR0
#else
    #error Unsupported  LV_COLOR_DEPTH
#endif

#define MY_CLASS (&lv_ffmpeg_raw_class)

#define FRAME_DEF_REFR_PERIOD   33  /*[ms]*/

#define DECODER_BUFFER_SIZE (8 * 1024)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct PacketQueue {
    AVPacketList    *first_pkt, *last_pkt;
    int             nb_packets;
    int             size;
    int             abort_request;
    int64_t         lastPts;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} PacketQueue;

struct ffmpeg_raw_context_s {
	uint8_t * stream_data;
	int stream_size;
	int read_bytes;
	int parse_count;
	
    AVCodecContext * video_dec_ctx;
    AVStream * video_stream;
    //uint8_t * video_src_data[4];
    uint8_t * video_dst_data[4];
	int video_dst_width;
	int video_dst_height;
    //struct SwsContext * sws_ctx;
    AVFrame * frame;
    AVPacket pkt;
    int video_stream_idx;
    bool has_alpha;  
    lv_draw_buf_t draw_buf;
    lv_draw_buf_handlers_t draw_buf_handlers;

	pthread_t video_tid;
	PacketQueue videoq;
};

#pragma pack(1)

struct _lv_image_pixel_color_s {
    lv_color_t c;
    uint8_t alpha;
};

#pragma pack()

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int packet_queue_put(PacketQueue *q, AVPacket *pkt);
static void packet_queue_init(PacketQueue *q);
static void packet_queue_flush(PacketQueue *q);
static void packet_queue_end(PacketQueue *q);
static void packet_queue_abort(PacketQueue *q);
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);

static struct ffmpeg_raw_context_s * ffmpeg_open_file(const char * path);
static void ffmpeg_close(struct ffmpeg_raw_context_s * ffmpeg_ctx);
static void ffmpeg_close_src_ctx(struct ffmpeg_raw_context_s * ffmpeg_ctx);
static void ffmpeg_close_dst_ctx(struct ffmpeg_raw_context_s * ffmpeg_ctx);
static int ffmpeg_image_allocate(struct ffmpeg_raw_context_s * ffmpeg_ctx, int width, int height);
static uint8_t * ffmpeg_get_image_data(struct ffmpeg_raw_context_s * ffmpeg_ctx);
static int ffmpeg_update_next_frame(struct ffmpeg_raw_context_s * ffmpeg_ctx);

static void lv_ffmpeg_raw_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ffmpeg_raw_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_ffmpeg_raw_class = {
    .constructor_cb = lv_ffmpeg_raw_constructor,
    .destructor_cb = lv_ffmpeg_raw_destructor,
    .instance_size = sizeof(lv_ffmpeg_raw_t),
    .base_class = &lv_image_class,
    .name = "lv_ffmpeg_raw",
};

static ISP_DEVICE gIspDev = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_ffmpeg_raw_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_result_t lv_ffmpeg_raw_set_src(lv_obj_t * obj, const char * path, int dst_width, int dst_height, int src_width, int src_height, int period)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_result_t res = LV_RESULT_INVALID;

    lv_ffmpeg_raw_t * player = (lv_ffmpeg_raw_t *)obj;

    if(player->ffmpeg_ctx) {
        ffmpeg_close(player->ffmpeg_ctx);
        player->ffmpeg_ctx = NULL;
    }

    lv_timer_pause(player->timer);

	//init isp
	uint32_t result = 0;
	MMP_ISP_CORE_INFO   ISPCOREINFO = { 0 };
	result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
	if (result)
        printf("mmpIspInitialize() error (0x%x) !!\n", result);
	ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
	result = mmpIspSetCore(gIspDev, &ISPCOREINFO);
    if (result)
        printf("mmpIspSetCore() error (0x%x) !!\n", result);

    result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_PLAY_VIDEO);
    if (result)
        printf("mmpIspSetMode() error (0x%x) !! \n", result);

	//open file
    player->ffmpeg_ctx = ffmpeg_open_file(path);
    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg file open failed: %s", path);
        goto failed;
    }

	player->ffmpeg_ctx->video_dec_ctx->width = src_width;
	player->ffmpeg_ctx->video_dec_ctx->height = src_height;
	player->ffmpeg_ctx->has_alpha = false;

    if(ffmpeg_image_allocate(player->ffmpeg_ctx, dst_width, dst_height) < 0) {
        LV_LOG_ERROR("ffmpeg image allocate failed");
        ffmpeg_close(player->ffmpeg_ctx);
        player->ffmpeg_ctx = NULL;
        goto failed;
    }

    bool has_alpha = player->ffmpeg_ctx->has_alpha;
    uint8_t * data = ffmpeg_get_image_data(player->ffmpeg_ctx);
    lv_color_format_t cf = has_alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_NATIVE;
    uint32_t stride = dst_width * lv_color_format_get_size(cf);
    uint32_t data_size = stride * dst_height;
    lv_memzero(data, data_size);

    player->imgdsc.header.w = dst_width;
    player->imgdsc.header.h = dst_height;
    player->imgdsc.data_size = data_size;
    player->imgdsc.header.cf = cf;
    player->imgdsc.header.stride = stride;
    player->imgdsc.data = data;

    lv_image_set_src(&player->img.obj, &(player->imgdsc));

    if(period > 0) {
        LV_LOG_INFO("frame refresh period = %d ms, rate = %d fps",
                    period, 1000 / period);
        lv_timer_set_period(player->timer, period);
    }
    else {
        LV_LOG_WARN("unable to get frame refresh period");
    }

    res = LV_RESULT_OK;

failed:
    return res;
}

void lv_ffmpeg_raw_set_cmd(lv_obj_t * obj, lv_ffmpeg_raw_cmd_t cmd)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_raw_t * player = (lv_ffmpeg_raw_t *)obj;

    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg_ctx is NULL");
        return;
    }

    lv_timer_t * timer = player->timer;

    switch(cmd) {
        case LV_FFMPEG_RAW_CMD_START:
            lv_timer_resume(timer);
            LV_LOG_INFO("ffmpeg player start");
            break;
        case LV_FFMPEG_RAW_CMD_STOP:
            lv_timer_pause(timer);
            LV_LOG_INFO("ffmpeg player stop");
            break;
        case LV_FFMPEG_RAW_CMD_PAUSE:
            lv_timer_pause(timer);
            LV_LOG_INFO("ffmpeg player pause");
            break;
        case LV_FFMPEG_RAW_CMD_RESUME:
            lv_timer_resume(timer);
            LV_LOG_INFO("ffmpeg player resume");
            break;
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            break;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;

    /* duplicate the packet */
    if (q->mutex)
    {
        if (av_dup_packet(pkt) < 0)
            return -1;

        pkt1       = av_malloc(sizeof(AVPacketList));
        if (!pkt1)
            return -1;
        pkt1->pkt  = *pkt;
        pkt1->next = NULL;

        pthread_mutex_lock(&q->mutex);

        if (!q->last_pkt)
            q->first_pkt = pkt1;
        else
            q->last_pkt->next = pkt1;
        q->last_pkt = pkt1;
        q->nb_packets++;
        q->size    += pkt1->pkt.size + sizeof(*pkt1);
        q->lastPts  = pkt->pts;
        /* XXX: should duplicate packet data in DV case */
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    else
    {
        return -1;
    }
}

/* packet queue handling */
static void packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    //packet_queue_put(q, &flush_pkt);
}

static void packet_queue_flush(PacketQueue *q)
{
    AVPacketList *pkt, *pkt1;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
        {
            pkt1 = pkt->next;
            av_free_packet(&pkt->pkt);
            av_freep(&pkt);
        }
        q->last_pkt   = NULL;
        q->first_pkt  = NULL;
        q->nb_packets = 0;
        q->size       = 0;
        pthread_mutex_unlock(&q->mutex);
    }
}

static void packet_queue_end(PacketQueue *q)
{
    if (q->mutex)
    {
        packet_queue_flush(q);
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->cond);
        memset(q, 0, sizeof(PacketQueue));
    }
}

static void packet_queue_abort(PacketQueue *q)
{
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        q->abort_request = 1;
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
    }
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet received. */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int          ret;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);

        for (;;)
        {
            if (q->abort_request)
            {
                ret = -1;
                break;
            }

            pkt1 = q->first_pkt;
            if (pkt1)
            {
                q->first_pkt = pkt1->next;
                if (!q->first_pkt)
                    q->last_pkt = NULL;
                q->nb_packets--;
                q->size -= pkt1->pkt.size + sizeof(*pkt1);
                *pkt     = pkt1->pkt;
                av_free(pkt1);
                ret      = 1;
                break;
            }
            else if (!block)
            {
                ret = 0;
                break;
            }
            else
            {
                av_log(NULL, AV_LOG_DEBUG, "queue empty, condition wait %lld\n", av_gettime());
                pthread_cond_wait(&q->cond, &q->mutex);
                av_log(NULL, AV_LOG_DEBUG, "leave condition wait %lld\n", av_gettime());
            }
        }
        pthread_mutex_unlock(&q->mutex);
        return ret;
    }
    else
    {
        return -1;
    }
}

static void render_frame(AVCodecContext *avctx, AVFrame *picture, struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
	uint32_t            result = 0;
	MMP_ISP_OUTPUT_INFO out_info  = {0};
    MMP_ISP_SHARE       isp_share = {0};

	if (picture->width != avctx->width && avctx->width != 0)
	{
        picture->width = avctx->width;
	}
    if (picture->height != avctx->height && avctx->height != 0)
    {
        picture->height = avctx->height;
    }

	if (picture->width % 4 != 0)
    {
        picture->width = picture->width - (picture->width % 4);
    }
    if (picture->height % 4 != 0)
    {
        picture->height = picture->height - (picture->height % 4);
    }

	isp_share.addrY    = (uint32_t)picture->data[0];
    isp_share.addrU    = (uint32_t)picture->data[1];
    isp_share.addrV    = (uint32_t)picture->data[2];
    isp_share.width    = picture->width;
    isp_share.height   = picture->height;
    isp_share.pitchY   = picture->linesize[0];
    isp_share.pitchUv  = picture->linesize[1];
    isp_share.format = MMP_ISP_IN_NV12;

	out_info.addrRGB   = (uint32_t)ffmpeg_ctx->video_dst_data[0];
	out_info.width     = ffmpeg_ctx->video_dst_width;
    out_info.height    = ffmpeg_ctx->video_dst_height;
    out_info.pitchRGB  = out_info.width * 2;
	out_info.format    = MMP_ISP_OUT_DITHER565A;

	mmpIspSetOutputWindow(gIspDev, &out_info);
	mmpIspSetVideoWindow(gIspDev, 0, 0, out_info.width, out_info.height);

	result = mmpIspPlayImageProcess(gIspDev, &isp_share);
		if (result)
			printf("mmpIspPlayImageProcess() error (0x%x) !!\n", result);

	result = mmpIspWaitEngineIdle(gIspDev);
	if (result)
		printf("mmpIspWaitEngineIdle() error (0x%x) !!\n", result);
}

static void *video_thread(void *arg)
{
    int            ret;
	struct ffmpeg_raw_context_s * ffmpeg_ctx = (struct ffmpeg_raw_context_s *) arg;

    for (;;)
    {
        AVPacket pkt;
        if (ffmpeg_ctx->videoq.abort_request)
        {
            goto the_end;
        }
        ret = packet_queue_get(&ffmpeg_ctx->videoq, &pkt, 0);
        if (ret < 0)
        {
            goto the_end;
        }
        else if (ret > 0)
        {
            int     got_picture = 0, rev = -1;

            rev = avcodec_decode_video2(ffmpeg_ctx->video_dec_ctx, ffmpeg_ctx->frame, &got_picture, &pkt);
            if (rev < 0)
                av_log(NULL, AV_LOG_ERROR, "video decode error\n");

            av_free_packet(&pkt);
            if (got_picture)
            {
            	//printf("got a video frame, y_pitch = %d, uv_pitch = %d, height = %d, data = %x\n", ffmpeg_ctx->frame->linesize[0], ffmpeg_ctx->frame->linesize[1], ffmpeg_ctx->frame->height, ffmpeg_ctx->frame->data[0]);
				render_frame(ffmpeg_ctx->video_dec_ctx, ffmpeg_ctx->frame, ffmpeg_ctx);
            }
        }
        else
            usleep(5000);
    }

the_end:
    if (ffmpeg_ctx->video_dec_ctx)
    {
        avcodec_flush_buffers(ffmpeg_ctx->video_dec_ctx);
    }
    pthread_exit(NULL);
    return 0;
}

static uint8_t * ffmpeg_get_image_data(struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
    uint8_t * img_data = ffmpeg_ctx->video_dst_data[0];

    if(img_data == NULL) {
        LV_LOG_ERROR("ffmpeg video dst data is NULL");
    }

    return img_data;
}

static int ffmpeg_update_next_frame(struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
    int ret = 0;
	int frame_size = 0;

    while(1)
	{
		if (ffmpeg_ctx->read_bytes == ffmpeg_ctx->stream_size )
		{
			ret = -1;
			break;
		}
	
		if (ffmpeg_ctx->parse_count == ffmpeg_ctx->stream_size)
		{
			uint8_t *pH264_packet = NULL;
			frame_size = ffmpeg_ctx->parse_count - ffmpeg_ctx->read_bytes + 1;

			pH264_packet = (uint8_t*)malloc(frame_size + 4);
			memcpy(pH264_packet, &ffmpeg_ctx->stream_data[ffmpeg_ctx->read_bytes], frame_size);
			ffmpeg_ctx->pkt.data = pH264_packet;
			ffmpeg_ctx->pkt.size = frame_size;
			packet_queue_put(&ffmpeg_ctx->videoq, &(ffmpeg_ctx->pkt));
			ffmpeg_ctx->read_bytes = ffmpeg_ctx->parse_count;

			break;
		}
		else if (ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count] == 0 && ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count+1] == 0 && ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count+2] == 0 && ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count+3] == 1)
		{
			uint8_t *pH264_packet = NULL;
			frame_size = ffmpeg_ctx->parse_count - ffmpeg_ctx->read_bytes;
			
			pH264_packet = (uint8_t*)malloc(frame_size + 4);
			memcpy(pH264_packet, &ffmpeg_ctx->stream_data[ffmpeg_ctx->read_bytes], frame_size);
			ffmpeg_ctx->pkt.data = pH264_packet;
			ffmpeg_ctx->pkt.size = frame_size;
			packet_queue_put(&ffmpeg_ctx->videoq, &(ffmpeg_ctx->pkt));
			
			ffmpeg_ctx->read_bytes = ffmpeg_ctx->parse_count;
			ffmpeg_ctx->parse_count = ffmpeg_ctx->parse_count + 4;
			break;
		}
		else if (ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count] == 0 && ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count+1] == 0 && ffmpeg_ctx->stream_data[ffmpeg_ctx->parse_count+2] == 1)
		{
			uint8_t *pH264_packet = NULL;
			frame_size = ffmpeg_ctx->parse_count - ffmpeg_ctx->read_bytes;
			
			pH264_packet = (uint8_t*)malloc(frame_size + 4);
			memcpy(pH264_packet, &ffmpeg_ctx->stream_data[ffmpeg_ctx->read_bytes], frame_size);
			ffmpeg_ctx->pkt.data = pH264_packet;
			ffmpeg_ctx->pkt.size = frame_size;
			packet_queue_put(&ffmpeg_ctx->videoq, &(ffmpeg_ctx->pkt));
			
			ffmpeg_ctx->read_bytes = ffmpeg_ctx->parse_count;
			ffmpeg_ctx->parse_count = ffmpeg_ctx->parse_count + 3;
			break;
		}
		else
			ffmpeg_ctx->parse_count++;	
    }

    return ret;

}

static struct ffmpeg_raw_context_s * ffmpeg_open_file(const char * path)
{
	FILE          *fp;
    unsigned int  file_size = 0;
	AVCodec * videoCodec;
	AVCodecContext * video_context = NULL;
	
    if(path == NULL || lv_strlen(path) == 0) {
        LV_LOG_ERROR("file path is empty");
        return NULL;
    }

    struct ffmpeg_raw_context_s * ffmpeg_ctx = lv_malloc_zeroed(sizeof(struct ffmpeg_raw_context_s));
    LV_ASSERT_MALLOC(ffmpeg_ctx);
    if(ffmpeg_ctx == NULL) {
        LV_LOG_ERROR("ffmpeg_ctx malloc failed");
        goto failed;
    }
	
    //init video hardware
	ithVideoInit(NULL);

    av_register_all();

    videoCodec = avcodec_find_decoder(CODEC_ID_H264);
	
	video_context              = avcodec_alloc_context();
    video_context->codec_id    = CODEC_ID_H264;
    video_context->codec_type  = AVMEDIA_TYPE_VIDEO;

	if(avcodec_open(video_context, videoCodec) >= 0)
	{
		pthread_attr_t attr;
		packet_queue_init(&ffmpeg_ctx->videoq);
		pthread_attr_init(&attr);
	    pthread_attr_setstacksize(&attr, 16*1024);
		pthread_create(&ffmpeg_ctx->video_tid, &attr, video_thread, (void *)ffmpeg_ctx);
	}

	ffmpeg_ctx->video_dec_ctx = video_context;

	//open file
	fp = fopen(path, "rb");
	fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

	ffmpeg_ctx->stream_size = file_size;
	ffmpeg_ctx->stream_data = malloc(ffmpeg_ctx->stream_size);
	fread(ffmpeg_ctx->stream_data, 1, ffmpeg_ctx->stream_size, fp);
	fclose(fp);
	
    return ffmpeg_ctx;

failed:
    ffmpeg_close(ffmpeg_ctx);
    return NULL;
}

static int ffmpeg_image_allocate(struct ffmpeg_raw_context_s * ffmpeg_ctx, int width, int height)
{
    int ret;

	ffmpeg_ctx->video_dst_width = width;
	ffmpeg_ctx->video_dst_height = height;
	ffmpeg_ctx->video_dst_data[0] = lv_malloc(width * height * 2);
    if(ffmpeg_ctx->video_dst_data[0] == NULL) {
        LV_LOG_ERROR("Could not allocate dst raw video buffer");
        return -1;
    }

    ffmpeg_ctx->frame = avcodec_alloc_frame();

    if(ffmpeg_ctx->frame == NULL) {
        LV_LOG_ERROR("Could not allocate frame");
        return -1;
    }

    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&ffmpeg_ctx->pkt);
    ffmpeg_ctx->pkt.data = NULL;
    ffmpeg_ctx->pkt.size = 0;

    return 0;
}

static void ffmpeg_close_src_ctx(struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
    if(ffmpeg_ctx->video_dec_ctx)
    {
    	avcodec_close(ffmpeg_ctx->video_dec_ctx);
		av_free(ffmpeg_ctx->video_dec_ctx);
    }
    av_free(ffmpeg_ctx->frame);
}

static void ffmpeg_close_dst_ctx(struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
    if(ffmpeg_ctx->video_dst_data[0] != NULL) {
        lv_free(ffmpeg_ctx->video_dst_data[0]);
        ffmpeg_ctx->video_dst_data[0] = NULL;
    }
}

static void ffmpeg_close(struct ffmpeg_raw_context_s * ffmpeg_ctx)
{
    if(ffmpeg_ctx == NULL) {
        LV_LOG_WARN("ffmpeg_ctx is NULL");
        return;
    }

	if (ffmpeg_ctx->video_tid)
	{
		packet_queue_abort(&ffmpeg_ctx->videoq);
        pthread_join(ffmpeg_ctx->video_tid, NULL);
        packet_queue_end(&ffmpeg_ctx->videoq);
		ffmpeg_ctx->video_tid = 0;
	}

    //sws_freeContext(ffmpeg_ctx->sws_ctx);
    ffmpeg_close_src_ctx(ffmpeg_ctx);
    ffmpeg_close_dst_ctx(ffmpeg_ctx);
    lv_free(ffmpeg_ctx);
	//release isp
	mmpIspTerminate(&gIspDev);

	//release video hardware
	ithVideoExit();
	
    LV_LOG_INFO("ffmpeg_ctx closed");
}

static void lv_ffmpeg_raw_frame_update_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    lv_ffmpeg_raw_t * player = (lv_ffmpeg_raw_t *)obj;

    if(!player->ffmpeg_ctx) {
        return;
    }

    int has_next = ffmpeg_update_next_frame(player->ffmpeg_ctx);

    if(has_next < 0) {
        lv_ffmpeg_raw_set_cmd(obj, LV_FFMPEG_RAW_CMD_STOP);
        return;
    }

    lv_image_cache_drop(lv_image_get_src(obj));

    lv_obj_invalidate(obj);
}

static void lv_ffmpeg_raw_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_raw_t * player = (lv_ffmpeg_raw_t *)obj;

    player->auto_restart = false;
    player->ffmpeg_ctx = NULL;
    player->timer = lv_timer_create(lv_ffmpeg_raw_frame_update_cb,
                                    FRAME_DEF_REFR_PERIOD, obj);
    lv_timer_pause(player->timer);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_ffmpeg_raw_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_raw_t * player = (lv_ffmpeg_raw_t *)obj;

    if(player->timer) {
        lv_timer_delete(player->timer);
        player->timer = NULL;
    }

    lv_image_cache_drop(lv_image_get_src(obj));

    ffmpeg_close(player->ffmpeg_ctx);
    player->ffmpeg_ctx = NULL;

    LV_TRACE_OBJ_CREATE("finished");
}

#endif /*LV_USE_FFMPEG*/
