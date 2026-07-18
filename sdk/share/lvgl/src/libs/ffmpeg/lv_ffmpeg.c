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

#define MY_CLASS (&lv_ffmpeg_player_class)

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

struct ffmpeg_context_s {
    AVIOContext * io_ctx;
    lv_fs_file_t lv_file;
    AVFormatContext * fmt_ctx;
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
    //int video_src_linesize[4];
    //int video_dst_linesize[4];
    //enum AVPixelFormat video_dst_pix_fmt;
	enum PixelFormat video_dst_pix_fmt;
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

static int ffmpeg_lvfs_read(void * ptr, uint8_t * buf, int buf_size);
static int64_t ffmpeg_lvfs_seek(void * ptr, int64_t pos, int whence);
static AVIOContext * ffmpeg_open_io_context(lv_fs_file_t * file);
static struct ffmpeg_context_s * ffmpeg_open_file(const char * path, bool is_lv_fs_path);
static void ffmpeg_close(struct ffmpeg_context_s * ffmpeg_ctx);
static void ffmpeg_close_src_ctx(struct ffmpeg_context_s * ffmpeg_ctx);
static void ffmpeg_close_dst_ctx(struct ffmpeg_context_s * ffmpeg_ctx);
static int ffmpeg_image_allocate(struct ffmpeg_context_s * ffmpeg_ctx, int width, int height);
static int ffmpeg_get_image_header(lv_image_decoder_dsc_t * dsc, lv_image_header_t * header);
static int ffmpeg_get_frame_refr_period(struct ffmpeg_context_s * ffmpeg_ctx);
static uint8_t * ffmpeg_get_image_data(struct ffmpeg_context_s * ffmpeg_ctx);
static int ffmpeg_update_next_frame(struct ffmpeg_context_s * ffmpeg_ctx);
static bool ffmpeg_pix_fmt_has_alpha(enum PixelFormat pix_fmt);
static bool ffmpeg_pix_fmt_is_yuv(enum PixelFormat pix_fmt);

static void lv_ffmpeg_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ffmpeg_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_ffmpeg_player_class = {
    .constructor_cb = lv_ffmpeg_player_constructor,
    .destructor_cb = lv_ffmpeg_player_destructor,
    .instance_size = sizeof(lv_ffmpeg_player_t),
    .base_class = &lv_image_class,
    .name = "lv_ffmpeg_player",
};

static ISP_DEVICE gIspDev = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_ffmpeg_player_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_result_t lv_ffmpeg_player_set_src(lv_obj_t * obj, const char * path, int dst_width, int dst_height)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_result_t res = LV_RESULT_INVALID;

    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

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
    player->ffmpeg_ctx = ffmpeg_open_file(path, LV_FFMPEG_PLAYER_USE_LV_FS);
    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg file open failed: %s", path);
        goto failed;
    }

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

    int period = ffmpeg_get_frame_refr_period(player->ffmpeg_ctx);

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

void lv_ffmpeg_player_set_cmd(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg_ctx is NULL");
        return;
    }

    lv_timer_t * timer = player->timer;

    switch(cmd) {
        case LV_FFMPEG_PLAYER_CMD_START:
            av_seek_frame(player->ffmpeg_ctx->fmt_ctx,
                          0, 0, AVSEEK_FLAG_BACKWARD);
            lv_timer_resume(timer);
            LV_LOG_INFO("ffmpeg player start");
            break;
        case LV_FFMPEG_PLAYER_CMD_STOP:
            av_seek_frame(player->ffmpeg_ctx->fmt_ctx,
                          0, 0, AVSEEK_FLAG_BACKWARD);
            lv_timer_pause(timer);
            LV_LOG_INFO("ffmpeg player stop");
            break;
        case LV_FFMPEG_PLAYER_CMD_PAUSE:
            lv_timer_pause(timer);
            LV_LOG_INFO("ffmpeg player pause");
            break;
        case LV_FFMPEG_PLAYER_CMD_RESUME:
            lv_timer_resume(timer);
            LV_LOG_INFO("ffmpeg player resume");
            break;
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            break;
    }
}

void lv_ffmpeg_player_set_auto_restart(lv_obj_t * obj, bool en)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;
    player->auto_restart = en;
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

static void render_frame(AVCodecContext *avctx, AVFrame *picture, struct ffmpeg_context_s * ffmpeg_ctx)
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
	if (avctx->codec_id == CODEC_ID_MJPEG)
	{
	    isp_share.format = MMP_ISP_IN_YUV422;
	}
	else
	{
	    isp_share.format = MMP_ISP_IN_NV12;
	}

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
	struct ffmpeg_context_s * ffmpeg_ctx = (struct ffmpeg_context_s *) arg;

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

static uint8_t * ffmpeg_get_image_data(struct ffmpeg_context_s * ffmpeg_ctx)
{
    uint8_t * img_data = ffmpeg_ctx->video_dst_data[0];

    if(img_data == NULL) {
        LV_LOG_ERROR("ffmpeg video dst data is NULL");
    }

    return img_data;
}

static bool ffmpeg_pix_fmt_has_alpha(enum PixelFormat pix_fmt)
{
    //const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    const AVPixFmtDescriptor * desc = &av_pix_fmt_descriptors[pix_fmt];

    if(desc == NULL) {
        return false;
    }

    if(pix_fmt == PIX_FMT_PAL8) {
        return true;
    }

    return (desc->flags & PIX_FMT_ALPHA) ? true : false;
}

static bool ffmpeg_pix_fmt_is_yuv(enum PixelFormat pix_fmt)
{
    //const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    const AVPixFmtDescriptor * desc = &av_pix_fmt_descriptors[pix_fmt];

    if(desc == NULL) {
        return false;
    }

    return !(desc->flags & PIX_FMT_RGB) && desc->nb_components >= 2;
}

static int ffmpeg_open_codec_context(int * stream_idx,
                                     AVCodecContext ** dec_ctx, AVFormatContext * fmt_ctx,
                                     enum AVMediaType type)
{
    int ret;
    int stream_index;
    AVStream * st;
    AVCodec * dec = NULL;
    AVDictionary * opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if(ret < 0) {
        LV_LOG_ERROR("Could not find %s stream in input file",
                     av_get_media_type_string(type));
        return ret;
    }
    else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
		*dec_ctx = st->codec;

        /* find decoder for the stream */
        //dec = avcodec_find_decoder(st->codecpar->codec_id);
        dec = avcodec_find_decoder(st->codec->codec_id);
        if(dec == NULL) {
            LV_LOG_ERROR("Failed to find %s codec",
                         av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Init the decoders */
        if((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            LV_LOG_ERROR("Failed to open %s codec",
                         av_get_media_type_string(type));
            return ret;
        }

        *stream_idx = stream_index;
    }

    return 0;
}

static int ffmpeg_get_image_header(lv_image_decoder_dsc_t * dsc,
                                   lv_image_header_t * header)
{
    int ret = -1;

    AVFormatContext * fmt_ctx = NULL;
    AVCodecContext * video_dec_ctx = NULL;
    AVIOContext * io_ctx = NULL;
    int video_stream_idx;

    io_ctx = ffmpeg_open_io_context(&dsc->file);
    if(io_ctx == NULL) {
        LV_LOG_ERROR("io_ctx malloc failed");
        return ret;
    }

    fmt_ctx = avformat_alloc_context();
    if(fmt_ctx == NULL) {
        LV_LOG_ERROR("fmt_ctx malloc failed");
        goto failed;
    }
    fmt_ctx->pb = io_ctx;
    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    /* open input file, and allocate format context */
    if(avformat_open_input(&fmt_ctx, (const char *)dsc->src, NULL, NULL) < 0) {
        LV_LOG_ERROR("Could not open source file %s", (const char *)dsc->src);
        goto failed;
    }

    /* retrieve stream information */
    if(avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LV_LOG_ERROR("Could not find stream information");
        goto failed;
    }

    if(ffmpeg_open_codec_context(&video_stream_idx, &video_dec_ctx,
                                 fmt_ctx, AVMEDIA_TYPE_VIDEO)
       >= 0) {
        bool has_alpha = ffmpeg_pix_fmt_has_alpha(video_dec_ctx->pix_fmt);

        /* allocate image where the decoded image will be put */
        header->w = video_dec_ctx->width;
        header->h = video_dec_ctx->height;
        header->cf = has_alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_NATIVE;
        header->stride = header->w * lv_color_format_get_size(header->cf);

        ret = 0;
    }

failed:
    //avcodec_free_context(&video_dec_ctx);
    if(video_dec_ctx)
    	avcodec_close(video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if(io_ctx != NULL) {
        av_free(io_ctx->buffer);
        av_free(io_ctx);
    }
    return ret;
}

static int ffmpeg_get_frame_refr_period(struct ffmpeg_context_s * ffmpeg_ctx)
{
    int avg_frame_rate_num = ffmpeg_ctx->video_stream->avg_frame_rate.num;
    if(avg_frame_rate_num > 0) {
        int period = 1000 * (int64_t)ffmpeg_ctx->video_stream->avg_frame_rate.den
                     / avg_frame_rate_num;
        return period;
    }

    return -1;
}

static int ffmpeg_update_next_frame(struct ffmpeg_context_s * ffmpeg_ctx)
{
    int ret = 0;

    while(1) {

        /* read frames from the file */
        if(av_read_frame(ffmpeg_ctx->fmt_ctx, &(ffmpeg_ctx->pkt)) >= 0) {
            bool is_image = false;

            /* check if the packet belongs to a stream we are interested in,
             * otherwise skip it
             */
            if(ffmpeg_ctx->pkt.stream_index == ffmpeg_ctx->video_stream_idx)
            {
				#if 0
                ret = ffmpeg_decode_packet(ffmpeg_ctx->video_dec_ctx,
                                           &(ffmpeg_ctx->pkt), ffmpeg_ctx);
				#else
				packet_queue_put(&ffmpeg_ctx->videoq, &(ffmpeg_ctx->pkt));
				#endif
                is_image = true;
            }
			else
				av_free_packet(&(ffmpeg_ctx->pkt));
            //av_packet_unref(&(ffmpeg_ctx->pkt));
            #if 0
            av_free_packet(&(ffmpeg_ctx->pkt));
            if(ret < 0) {
                LV_LOG_WARN("video frame is empty %d", ret);
                break;
            }
			#endif
            /* Used to filter data that is not an image */
            if(is_image) {
                break;
            }
        }
        else {
            ret = -1;
            break;
        }
    }

    return ret;
}

static int ffmpeg_lvfs_read(void * ptr, uint8_t * buf, int buf_size)
{
    lv_fs_file_t * file = ptr;
    uint32_t bytesRead = 0;
    lv_fs_res_t res = lv_fs_read(file, buf, buf_size, &bytesRead);
    if(bytesRead == 0)
        return AVERROR_EOF;  /* Let FFmpeg know that we have reached eof */
    if(res != LV_FS_RES_OK)
        return AVERROR_EOF;
    return bytesRead;
}

static int64_t ffmpeg_lvfs_seek(void * ptr, int64_t pos, int whence)
{
    lv_fs_file_t * file = ptr;
    if(whence == SEEK_SET && lv_fs_seek(file, pos, SEEK_SET) == LV_FS_RES_OK) {
        return pos;
    }
    return -1;
}

static AVIOContext * ffmpeg_open_io_context(lv_fs_file_t * file)
{
    uint8_t * iBuffer = av_malloc(DECODER_BUFFER_SIZE);
    if(iBuffer == NULL) {
        LV_LOG_ERROR("iBuffer malloc failed");
        return NULL;
    }
    AVIOContext * pIOCtx = avio_alloc_context(iBuffer, DECODER_BUFFER_SIZE,   /* internal Buffer and its size */
                                              0,                                   /* bWriteable (1=true,0=false) */
                                              file,                                /* user data ; will be passed to our callback functions */
                                              ffmpeg_lvfs_read,                    /* Read callback function */
                                              0,                                   /* Write callback function */
                                              ffmpeg_lvfs_seek);                   /* Seek callback function */
    if(pIOCtx == NULL) {
        av_free(iBuffer);
        return NULL;
    }
    return pIOCtx;
}

static struct ffmpeg_context_s * ffmpeg_open_file(const char * path, bool is_lv_fs_path)
{
    if(path == NULL || lv_strlen(path) == 0) {
        LV_LOG_ERROR("file path is empty");
        return NULL;
    }

    struct ffmpeg_context_s * ffmpeg_ctx = lv_malloc_zeroed(sizeof(struct ffmpeg_context_s));
    LV_ASSERT_MALLOC(ffmpeg_ctx);
    if(ffmpeg_ctx == NULL) {
        LV_LOG_ERROR("ffmpeg_ctx malloc failed");
        goto failed;
    }
	
    if(is_lv_fs_path) {
        const lv_fs_res_t fs_res = lv_fs_open(&(ffmpeg_ctx->lv_file), path, LV_FS_MODE_RD);
        if(fs_res != LV_FS_RES_OK) {
            LV_LOG_WARN("Could not open file: %s, res: %d", path, fs_res);
            lv_free(ffmpeg_ctx);
            return NULL;
        }

        ffmpeg_ctx->io_ctx = ffmpeg_open_io_context(&(ffmpeg_ctx->lv_file));     /* Save the buffer pointer to free it later */

        if(ffmpeg_ctx->io_ctx == NULL) {
            LV_LOG_ERROR("io_ctx malloc failed");
            goto failed;
        }

        ffmpeg_ctx->fmt_ctx = avformat_alloc_context();
        if(ffmpeg_ctx->fmt_ctx == NULL) {
            LV_LOG_ERROR("fmt_ctx malloc failed");
            goto failed;
        }
        ffmpeg_ctx->fmt_ctx->pb = ffmpeg_ctx->io_ctx;
        ffmpeg_ctx->fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
    }

    //init video hardware
	ithVideoInit(NULL);

    av_register_all();

    /* open input file, and allocate format context */
    if(avformat_open_input(&(ffmpeg_ctx->fmt_ctx), path, NULL, NULL) < 0) {
        LV_LOG_ERROR("Could not open source file %s", path);
        goto failed;
    }

    /* retrieve stream information */
	#if 0
    if(avformat_find_stream_info(ffmpeg_ctx->fmt_ctx, NULL) < 0) {
        LV_LOG_ERROR("Could not find stream information");
        goto failed;
    }
	#endif
    if(ffmpeg_open_codec_context(
           &(ffmpeg_ctx->video_stream_idx),
           &(ffmpeg_ctx->video_dec_ctx),
           ffmpeg_ctx->fmt_ctx, AVMEDIA_TYPE_VIDEO)
       >= 0) {
        ffmpeg_ctx->video_stream = ffmpeg_ctx->fmt_ctx->streams[ffmpeg_ctx->video_stream_idx];

        ffmpeg_ctx->has_alpha = ffmpeg_pix_fmt_has_alpha(ffmpeg_ctx->video_dec_ctx->pix_fmt);

        ffmpeg_ctx->video_dst_pix_fmt = (ffmpeg_ctx->has_alpha ? PIX_FMT_BGRA : AV_PIX_FMT_TRUE_COLOR);

		pthread_attr_t attr;
		packet_queue_init(&ffmpeg_ctx->videoq);
		pthread_attr_init(&attr);
	    pthread_attr_setstacksize(&attr, 16*1024);
		pthread_create(&ffmpeg_ctx->video_tid, &attr, video_thread, (void *)ffmpeg_ctx);
    }

#if LV_FFMPEG_DUMP_FORMAT
    /* dump input information to stderr */
    av_dump_format(ffmpeg_ctx->fmt_ctx, 0, path, 0);
#endif

    if(ffmpeg_ctx->video_stream == NULL) {
        LV_LOG_ERROR("Could not find video stream in the input, aborting");
        goto failed;
    }

    return ffmpeg_ctx;

failed:
    ffmpeg_close(ffmpeg_ctx);
    return NULL;
}

static int ffmpeg_image_allocate(struct ffmpeg_context_s * ffmpeg_ctx, int width, int height)
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

static void ffmpeg_close_src_ctx(struct ffmpeg_context_s * ffmpeg_ctx)
{
    if(ffmpeg_ctx->video_dec_ctx)
    {
    	avcodec_close(ffmpeg_ctx->video_dec_ctx);
    }
    avformat_close_input(&(ffmpeg_ctx->fmt_ctx));
    av_free(ffmpeg_ctx->frame);
}

static void ffmpeg_close_dst_ctx(struct ffmpeg_context_s * ffmpeg_ctx)
{
    if(ffmpeg_ctx->video_dst_data[0] != NULL) {
        lv_free(ffmpeg_ctx->video_dst_data[0]);
        ffmpeg_ctx->video_dst_data[0] = NULL;
    }
}

static void ffmpeg_close(struct ffmpeg_context_s * ffmpeg_ctx)
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
    if(ffmpeg_ctx->io_ctx != NULL) {
        av_free(ffmpeg_ctx->io_ctx->buffer);
        av_free(ffmpeg_ctx->io_ctx);
        lv_fs_close(&(ffmpeg_ctx->lv_file));
    }
    lv_free(ffmpeg_ctx);
	//release isp
	mmpIspTerminate(&gIspDev);

	//release video hardware
	ithVideoExit();
	
    LV_LOG_INFO("ffmpeg_ctx closed");
}

static void lv_ffmpeg_player_frame_update_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

    if(!player->ffmpeg_ctx) {
        return;
    }

    int has_next = ffmpeg_update_next_frame(player->ffmpeg_ctx);

    if(has_next < 0) {
        lv_ffmpeg_player_set_cmd(obj, player->auto_restart ? LV_FFMPEG_PLAYER_CMD_START : LV_FFMPEG_PLAYER_CMD_STOP);
        if(!player->auto_restart) {
            lv_obj_send_event((lv_obj_t *)player, LV_EVENT_READY, NULL);
        }
        return;
    }

    lv_image_cache_drop(lv_image_get_src(obj));

    lv_obj_invalidate(obj);
}

static void lv_ffmpeg_player_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{

    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

    player->auto_restart = false;
    player->ffmpeg_ctx = NULL;
    player->timer = lv_timer_create(lv_ffmpeg_player_frame_update_cb,
                                    FRAME_DEF_REFR_PERIOD, obj);
    lv_timer_pause(player->timer);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_ffmpeg_player_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

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
