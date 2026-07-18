/**
 * @file lv_itg_anim_it986x.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../widgets/image/lv_image_private.h"
#include "../../misc/lv_timer_private.h"
#include "../../core/lv_obj_class_private.h"
#include "../../../lvgl.h"
#if LV_USE_ITG && !defined(_WIN32) && (CFG_CHIP_FAMILY == 9860)

#include "lv_itg_private.h"
#include "../../draw/ite_it2d/lv_draw_ite_it2d.h"
#include "../tjpgd/tjpgd.h"
#include "ite_itdcps.h"
#include "ite_itdpu.h"
#include "ite_itjpeg.h"
#include "ite_itvp.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS            (&lv_itg_anim_class)
#define TJPGD_WORKBUFF_SIZE 4096    //Recommended by TJPGD libray
#define ITDPU_DEVICE        DEVICE_DT_NAME(DT_NODELABEL(dpu))

/* dispose_op flags from inside fcTL */
#define PNG_DISPOSE_OP_NONE        0x00
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#define PNG_DISPOSE_OP_PREVIOUS    0x02

/* blend_op flags from inside fcTL */
#define PNG_BLEND_OP_SOURCE        0x00
#define PNG_BLEND_OP_OVER          0x01

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint16_t frame_count;
    uint16_t play_count;
} itg_anim_t;

typedef struct {
    uint8_t itg_type;
    struct {
        uint8_t dop : 4;
        uint8_t bop : 4;
    } flags;
    uint16_t delay;
    uint16_t w0;
    uint16_t h0;
    uint16_t x0;
    uint16_t y0;
    uint32_t size_or_alpha_offset;
    uint32_t data_offset;
} itg_frame_t;

typedef struct {
    lv_image_t img;
    itg_anim_t * anim;
    itg_frame_t * frames;
    ite_it2d_surface_t surf;
    uint8_t * surf_buf;
    ite_it2d_surface_t src_surf;
    ite_it2d_surface_t prev_surf;
    uint8_t * src_surf_buf;
    lv_timer_t * timer;
    lv_img_dsc_t imgdsc;
    uint32_t last_call;
    uint16_t frame;
    uint16_t play;
    uint16_t delay;
    uint16_t x0;
    uint16_t y0;
    uint8_t dop;
    bool img_src_file;
    bool ref_alpha_buf;
    bool end_play;
} lv_itg_anim_t;

typedef struct {
    uint8_t * data;
    uint32_t data_size;
    uint32_t next_read_pos;
} io_source_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_itg_anim_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_itg_anim_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void next_frame_task_cb(lv_timer_t * t);
static int itg_anim_render_frame(lv_itg_anim_t * itg_anim);
static void itg_anim_close(lv_itg_anim_t * itg_anim);
static size_t input_func(JDEC * jd, uint8_t * buff, size_t ndata);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_itg_anim_class = {
    .constructor_cb = lv_itg_anim_constructor,
    .destructor_cb = lv_itg_anim_destructor,
    .instance_size = sizeof(lv_itg_anim_t),
    .base_class = &lv_image_class,
    .name = "itg_anim",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_itg_anim_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_itg_anim_set_src(lv_obj_t * obj, const void * src)
{
    lv_itg_anim_t * itg_anim = (lv_itg_anim_t *) obj;

    /*Close previous anim if any*/
    if(itg_anim->anim) {
        itg_anim_close(itg_anim);
        itg_anim->imgdsc.data = NULL;
    }

    uint8_t * img_data = NULL;
    itg_header_t itg_header;

    lv_memzero(&itg_anim->surf, sizeof(ite_it2d_surface_t));
    lv_memzero(&itg_anim->prev_surf, sizeof(ite_it2d_surface_t));
    lv_memzero(&itg_anim->src_surf, sizeof(ite_it2d_surface_t));

    itg_anim->img_src_file = false;
    itg_anim->ref_alpha_buf = false;
    itg_anim->frame = 0;
    itg_anim->play = 0;
    itg_anim->delay = 0;
    itg_anim->dop = 0;

    if(lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        const lv_img_dsc_t * img_dsc = src;

        lv_memcpy(&itg_header, img_dsc->data, sizeof(itg_header));

        if(itg_header.itg_type != ITG_ANIM) {
            LV_LOG_WARN("not a animation itg data");
            return;
        }

        img_data = (uint8_t *) img_dsc->data + sizeof(itg_header);
    }
    else if(lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        uint32_t br;

        lv_fs_file_t lv_file;
        lv_fs_res_t res = lv_fs_open(&lv_file, src, LV_FS_MODE_RD);
        if(res != LV_FS_RES_OK) {
            LV_LOG_WARN("open itg file fail");
            return;
        }

        res = lv_fs_read(&lv_file, &itg_header, sizeof(itg_header), &br);
        if((res != LV_FS_RES_OK) || (br != sizeof(itg_header))) {
            lv_fs_close(&lv_file);
            return;
        }

        if(itg_header.itg_type != ITG_ANIM) {
            LV_LOG_WARN("not a animation itg file");
            lv_fs_close(&lv_file);
            return;
        }

        uint32_t buf_size = itg_header.size + 7;
        img_data = (uint8_t *) lv_draw_ite_it2d_alloc_buffer(buf_size);
        if(!img_data) {
            LV_LOG_WARN("out of 2D memory");
            lv_fs_close(&lv_file);
            return;
        }

        res = lv_fs_read(&lv_file, img_data, buf_size, &br);
        if((res != LV_FS_RES_OK) || (br < itg_header.size)) {
            LV_LOG_WARN("read itg file fail");
            lv_draw_ite_it2d_free_buffer(img_data);
            lv_fs_close(&lv_file);
            return;
        }

        lv_fs_close(&lv_file);
        itg_anim->img_src_file = true;
    }

    if(itg_header.flags & ITG_CRC32) {
        uint8_t * crc_data = (uint8_t *)LV_ROUND_UP((uint32_t)(img_data + itg_header.size), 4);
        uint32_t crc = 0;
        int ret = ite_itdpu_crc32((const uint8_t *)&itg_header, sizeof(itg_header), &crc);
        ret |= ite_itdpu_crc32(img_data, LV_ROUND_UP(itg_header.size, 4), &crc);
        if((ret < 0) || memcmp(&crc, crc_data, 4)) {
            LV_LOG_WARN("CRC verify fail");
            if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
            return;
        }
    }

    itg_anim_t * anim = (itg_anim_t *) img_data;
    itg_frame_t * frames = (itg_frame_t *)(anim + 1);
    bool need_src_surf = false;
    bool need_prev_surf = false;

    for(int i = 1; i < anim->frame_count; i++) {
        itg_frame_t * frame = &frames[i];

        if(itg_anim->src_surf.width < frame->w0) {
            itg_anim->src_surf.width = frame->w0;
        }
        if(itg_anim->src_surf.height < frame->h0) {
            itg_anim->src_surf.height = frame->h0;
        }

        if((frame->flags.bop == PNG_BLEND_OP_OVER) ||
           (frame->w0 != itg_header.width) ||
           (frame->h0 != itg_header.height)) {
            need_src_surf = true;
        }

        if(frame->flags.dop == PNG_DISPOSE_OP_PREVIOUS) {
            need_prev_surf = true;
        }
    }

    if((frames->itg_type >= ITG_RAW) && (frames->itg_type <= ITG_SPEEDY)) {
        if(itg_header.pixel_format == ITG_RGB565) {
            itg_anim->surf.pitch = itg_header.width * 2;
            itg_anim->surf.flags.format = ITE_IT2D_PIXEL_FORMAT_RGB_565;
            itg_anim->src_surf.pitch = itg_anim->src_surf.width * 2;
        }
        else {
            itg_anim->surf.pitch = itg_header.width * 4;
            itg_anim->surf.flags.format = ITE_IT2D_PIXEL_FORMAT_ARGB_8888;
            itg_anim->src_surf.pitch = itg_anim->src_surf.width * 4;
        }
    }
    else if((frames->itg_type >= ITG_JPEG_RAW) && (frames->itg_type <= ITG_JPEG_SPEEDY)) {
        int width_align = (itg_header.flags & ITG_JPEG_YCBCR444) ? 8 : 16;

        if(itg_header.pixel_format == ITG_RGB565) {
            itg_anim->surf.pitch = LV_ROUND_UP(itg_header.width, width_align) * 2;
            itg_anim->surf.flags.format = ITE_IT2D_PIXEL_FORMAT_RGB_565;
            itg_anim->src_surf.pitch = LV_ROUND_UP(itg_anim->src_surf.width, width_align) * 2;
        }
        else {
            itg_anim->surf.pitch = LV_ROUND_UP(itg_header.width, width_align) * 4;
            itg_anim->surf.flags.format = ITE_IT2D_PIXEL_FORMAT_ARGB_8888;
            itg_anim->src_surf.pitch = LV_ROUND_UP(itg_anim->src_surf.width, width_align) * 4;
        }

        if(itg_header.itg_type != ITG_JPEG_RAW) {
            uint32_t buf_size = LV_ROUND_UP(itg_header.width, 16) * LV_ROUND_UP(itg_header.height, 16);
            if(itg_alpha_buffer.size < buf_size) {
                if(itg_alpha_buffer.buf) {
                    ite_it2d_wait_idle();
                    lv_draw_ite_it2d_free_buffer(itg_alpha_buffer.buf);
                    itg_alpha_buffer.buf = NULL;
                    itg_alpha_buffer.size = 0;
                }
                itg_alpha_buffer.buf = lv_draw_ite_it2d_alloc_buffer(buf_size);
                if(! itg_alpha_buffer.buf) {
                    LV_LOG_WARN("out of 2D memory");
                    if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
                    return;
                }
                itg_alpha_buffer.size = buf_size;
            }

            itg_anim->ref_alpha_buf = true;
            itg_alpha_buffer.refcount++;
        }
    }
    else {
        LV_LOG_WARN("unsupport itg type");
        if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
        return;
    }

    itg_anim->surf_buf = lv_draw_ite_it2d_alloc_buffer(itg_anim->surf.pitch * LV_ROUND_UP(itg_header.height, 16) + 15);
    if(! itg_anim->surf_buf) {
        LV_LOG_WARN("out of 2D memory");
        if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
        return;
    }
    itg_anim->surf.buf = itg_anim->surf_buf;

    if(need_src_surf) {
        itg_anim->src_surf_buf = lv_draw_ite_it2d_alloc_buffer(itg_anim->src_surf.pitch * LV_ROUND_UP(itg_anim->src_surf.height,
                                                                                                   16) + 15);
        if(! itg_anim->src_surf_buf) {
            LV_LOG_WARN("out of 2D memory");
            lv_draw_ite_it2d_free_buffer(itg_anim->surf_buf);
            itg_anim->surf_buf = NULL;
            if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
            return;
        }
        itg_anim->src_surf.flags.format = itg_anim->surf.flags.format;
        itg_anim->src_surf.flags.static_buf = true;
        itg_anim->src_surf.buf = itg_anim->src_surf_buf;
        itg_anim->src_surf.id = ite_it2d_get_current_id();
    }

    if(need_prev_surf) {
        itg_anim->prev_surf.buf = lv_draw_ite_it2d_alloc_buffer(itg_anim->src_surf.pitch * itg_anim->src_surf.height);
        if(! itg_anim->prev_surf.buf) {
            LV_LOG_WARN("out of 2D memory");
            lv_draw_ite_it2d_free_buffer(itg_anim->src_surf_buf);
            itg_anim->src_surf_buf = NULL;
            lv_draw_ite_it2d_free_buffer(itg_anim->surf_buf);
            itg_anim->surf_buf = NULL;
            if(itg_anim->img_src_file) lv_draw_ite_it2d_free_buffer(img_data);
            return;
        }

        itg_anim->prev_surf.flags.format = itg_anim->surf.flags.format;
        itg_anim->prev_surf.flags.static_buf = true;
        itg_anim->prev_surf.id = ite_it2d_get_current_id();
    }

    if((frames->itg_type >= ITG_JPEG_RAW) && (frames->itg_type <= ITG_JPEG_SPEEDY)) {
        itg_anim->surf.flags.buf_offset = LV_ROUND_UP((uint32_t)itg_anim->surf_buf, 16) - (uint32_t)(itg_anim->surf_buf);
        itg_anim->surf.buf = itg_anim->surf_buf + itg_anim->surf.flags.buf_offset;
        itg_anim->src_surf.flags.buf_offset = LV_ROUND_UP((uint32_t)itg_anim->src_surf.buf,
                                                          16) - (uint32_t)(itg_anim->src_surf.buf);
        itg_anim->src_surf.buf = itg_anim->src_surf_buf + itg_anim->src_surf.flags.buf_offset;
    }

    itg_anim->anim = (itg_anim_t *) img_data;
    itg_anim->frames = (itg_frame_t *)(itg_anim->anim + 1);

    itg_anim->surf.width = itg_header.width;
    itg_anim->surf.height = itg_header.height;
    itg_anim->surf.flags.static_buf = true;
    itg_anim->surf.id = ite_it2d_get_current_id();

    itg_anim->imgdsc.data = itg_anim->surf.buf;
    itg_anim->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    itg_anim->imgdsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE | LV_DRAW_ITE_IT2D_READY;
    itg_anim->imgdsc.header.cf = (itg_anim->surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? LV_COLOR_FORMAT_RGB565 :
                                 LV_COLOR_FORMAT_ARGB8888;
    itg_anim->imgdsc.header.h = itg_anim->surf.height;
    itg_anim->imgdsc.header.w = itg_anim->surf.width;
    itg_anim->imgdsc.header.stride = itg_anim->surf.pitch;
    itg_anim->imgdsc.data_size = itg_anim->surf.pitch * itg_anim->surf.height;

    itg_anim->last_call = lv_tick_get();

    lv_img_set_src(obj, &itg_anim->imgdsc);

    lv_timer_resume(itg_anim->timer);
    lv_timer_reset(itg_anim->timer);

    next_frame_task_cb(itg_anim->timer);
}

void lv_itg_anim_restart(lv_obj_t * obj)
{
    lv_itg_anim_t * itg_anim = (lv_itg_anim_t *) obj;

    itg_anim->frame = 0;
    itg_anim->play = 0;
    itg_anim->delay = 0;
    itg_anim->dop = 0;

    lv_timer_resume(itg_anim->timer);
    lv_timer_reset(itg_anim->timer);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_itg_anim_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_itg_anim_t * itg_anim = (lv_itg_anim_t *) obj;

    itg_anim->anim = NULL;
    itg_anim->timer = lv_timer_create(next_frame_task_cb, 10, obj);
    lv_timer_pause(itg_anim->timer);
}

static void lv_itg_anim_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_itg_anim_t * itg_anim = (lv_itg_anim_t *) obj;
    if(itg_anim->anim)
        itg_anim_close(itg_anim);
    lv_timer_del(itg_anim->timer);
}

static void next_frame_task_cb(lv_timer_t * t)
{
    lv_obj_t * obj = t->user_data;
    lv_itg_anim_t * itg_anim = (lv_itg_anim_t *) obj;
    uint32_t elaps = lv_tick_elaps(itg_anim->last_call);
    if(elaps < itg_anim->delay) return;

    itg_anim->last_call = lv_tick_get();

    int has_next = itg_anim_render_frame(itg_anim);
    if(has_next == 0) {
        /*It was the last repeat*/
        lv_timer_pause(t);
        lv_obj_send_event(obj, LV_EVENT_READY, NULL);
    }

    lv_obj_invalidate(obj);
}

static int itg_anim_render_frame(lv_itg_anim_t * itg_anim)
{
    itg_frame_t * frame = &itg_anim->frames[itg_anim->frame];
    ite_it2d_context_t it2d_ctx;
    int ret;

    if(itg_anim->dop != PNG_DISPOSE_OP_NONE) {
        ite_it2d_ctx_init(&it2d_ctx);
        ite_it2d_set_dst_surface(&it2d_ctx, &itg_anim->surf);
        ite_it2d_set_dst_pos(&it2d_ctx, itg_anim->x0, itg_anim->y0);

        if(itg_anim->dop == PNG_DISPOSE_OP_BACKGROUND) {
            ite_it2d_set_fg_color(&it2d_ctx, ite_it2d_color_make(0, 0, 0, 0));
            ret = ite_it2d_fill(&it2d_ctx);
        }
        else {
            LV_ASSERT(itg_anim->dop == PNG_DISPOSE_OP_PREVIOUS);
            ite_it2d_set_src_surface(&it2d_ctx, &itg_anim->prev_surf);
            ret = ite_it2d_copy(&it2d_ctx);
        }

        if(ret < 0) {
            LV_LOG_WARN("dispose op fail");
        }

        itg_anim->dop = PNG_DISPOSE_OP_NONE;
    }

    if(itg_anim->end_play) {
        itg_anim->end_play = false;
        return 0;
    }

    if(frame->flags.dop == PNG_DISPOSE_OP_PREVIOUS) {
        LV_ASSERT(itg_anim->prev_surf.buf);
        itg_anim->prev_surf.width = frame->w0;
        itg_anim->prev_surf.height = frame->h0;
        itg_anim->prev_surf.pitch = frame->w0 * ((itg_anim->prev_surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? 2 : 4);
        itg_anim->src_surf.pitch = frame->w0 * ((itg_anim->src_surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? 2 : 4);

        ite_it2d_ctx_init(&it2d_ctx);
        ite_it2d_set_dst_surface(&it2d_ctx, &itg_anim->prev_surf);
        ite_it2d_set_src_surface(&it2d_ctx, &itg_anim->surf);
        ite_it2d_set_src_pos(&it2d_ctx, frame->x0, frame->y0);

        ret = ite_it2d_copy(&it2d_ctx);
        if(ret < 0) {
            LV_LOG_WARN("copy surface fail");
        }
        itg_anim->x0 = frame->x0;
        itg_anim->y0 = frame->y0;
    }

    bool to_src_surf = itg_anim->src_surf_buf && (itg_anim->frame > 0);

    if(frame->itg_type == ITG_RAW) {
        if(!itg_anim->src_surf_buf && !itg_anim->prev_surf.buf) {
            itg_anim->surf.buf = (uint8_t *)itg_anim->anim + frame->data_offset;
            to_src_surf = false;
        }
        else {
            itg_anim->src_surf.buf = (uint8_t *)itg_anim->anim + frame->data_offset;
            to_src_surf = true;
        }
        itg_anim->src_surf.pitch = frame->w0 * ((itg_anim->src_surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? 2 : 4);
    }
    else if((frame->itg_type == ITG_UCL) || (frame->itg_type == ITG_SPEEDY)) {
        ite_itdcps_dsc_t dcps_dsc = { 0 };
        uint32_t pitch = frame->w0 * ((itg_anim->surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? 2 : 4);
        ite_it2d_surface_t * surf = to_src_surf ? &itg_anim->src_surf : &itg_anim->surf;
        dcps_dsc.dst_size = pitch * frame->h0;
        dcps_dsc.dst = surf->buf;
        dcps_dsc.src_size = frame->size_or_alpha_offset;
        dcps_dsc.src = (uint8_t *)itg_anim->anim + frame->data_offset;
        dcps_dsc.flags.mode = (frame->itg_type == ITG_UCL) ? ITE_ITDCPS_MODE_UCL_DEC : ITE_ITDCPS_MODE_SPEEDY_DEC;

        ret = ite_itdcps_decompress(&dcps_dsc);
        if(ret < 0) {
            LV_LOG_WARN("decompress surface data fail");
            goto end;
        }

        itg_anim->src_surf.pitch = pitch;
        surf->id = ite_it2d_get_current_id();
    }
    else if((frame->itg_type >= ITG_JPEG_RAW) && (frame->itg_type <= ITG_JPEG_SPEEDY)) {
        itg_jpeg_t * jpeg = (itg_jpeg_t *) &itg_anim->frames[itg_anim->anim->frame_count];
        uint8_t * alpha_buf;
        uint32_t buf_size;

        jpeg += itg_anim->frame;

        if(frame->itg_type == ITG_JPEG_RAW) {
            alpha_buf = (uint8_t *)itg_anim->anim + frame->size_or_alpha_offset;
            if((uint32_t)alpha_buf % 8) {
                ite_it2d_wait_idle();
                lv_memcpy(itg_alpha_buffer.buf, alpha_buf, jpeg->alpha_size);
                alpha_buf = itg_alpha_buffer.buf;
            }
        }
        else {
            LV_ASSERT(jpeg->alpha_size > 0);
            buf_size = LV_ROUND_UP(frame->w0, 16) * LV_ROUND_UP(frame->h0, 16);
            ite_itdcps_dsc_t dcps_dsc = { 0 };
            dcps_dsc.dst_size = buf_size;
            dcps_dsc.dst = itg_alpha_buffer.buf;
            dcps_dsc.src_size = jpeg->alpha_size;
            dcps_dsc.src = (uint8_t *)itg_anim->anim + frame->size_or_alpha_offset;
            dcps_dsc.flags.mode = (frame->itg_type == ITG_JPEG_UCL) ? ITE_ITDCPS_MODE_UCL_DEC : ITE_ITDCPS_MODE_SPEEDY_DEC;

            ret = ite_itdcps_decompress(&dcps_dsc);
            if(ret < 0) {
                LV_LOG_WARN("decompress alpha data fail");
                goto end;
            }

            alpha_buf = itg_alpha_buffer.buf;
        }

        uint8_t * jpeg_data = (uint8_t *)itg_anim->anim + frame->data_offset;

        uint8_t * workb_temp = lv_malloc(TJPGD_WORKBUFF_SIZE);
        if(! workb_temp) {
            LV_LOG_WARN("out of memory");
            goto end;
        }
        io_source_t io;
        io.data = jpeg_data;
        io.data_size = jpeg->jpeg_size;
        io.next_read_pos = 0;

        JDEC jd;
        JRESULT rc = jd_prepare(&jd, input_func, workb_temp, (size_t)TJPGD_WORKBUFF_SIZE, &io);
        if(rc != JDR_OK) {
            lv_free(workb_temp);
            LV_LOG_WARN("decode itg JPEG fail");
            goto end;
        }

        ite_itjpeg_context_t ctx;
        ite_itjpeg_ctx_init(&ctx);

        ctx.width = LV_ROUND_UP(jd.width, jd.msx * 8);
        ctx.height = LV_ROUND_UP(jd.height, jd.msy * 8);
        ctx.pitch = ctx.width;

        buf_size = ctx.pitch * ctx.height * 2 + 15;
        uint8_t * outbuf = lv_draw_ite_it2d_alloc_buffer(buf_size);
        if(! outbuf) {
            lv_free(workb_temp);
            LV_LOG_WARN("out of memory");
            goto end;
        }

        bool yuv444 = false;
        if(jd.samp[0][0] == 0x1 && jd.samp[0][1] == 0x2 &&
           jd.samp[1][0] == 0x1 && jd.samp[1][1] == 0x2 &&
           jd.samp[2][0] == 0x1 && jd.samp[2][1] == 0x2) {
            yuv444 = true;
        }

        ctx.uv_pitch = yuv444 ? ctx.pitch : ctx.pitch / 2;
        ctx.inbuf = io.data + jd.ofs;
        ctx.inbuf_size = io.data_size - jd.ofs;
        ctx.outbuf[0] = (uint8_t *)LV_ROUND_UP((uint32_t)outbuf, 16);
        ctx.outbuf[1] = ctx.outbuf[0] + (ctx.pitch * ctx.height);
        ctx.outbuf[2] = ctx.outbuf[1] + (ctx.uv_pitch * ctx.height);
        ctx.op.decode.msx = jd.msx;
        ctx.op.decode.msy = jd.msy;
        lv_memcpy(ctx.op.decode.qtid, jd.qtid, sizeof(ctx.op.decode.qtid));
        ctx.op.decode.ncomp = jd.ncomp;
        ctx.op.decode.nrst = jd.nrst;
        lv_memcpy(ctx.op.decode.huffbits, jd.huffbits, sizeof(ctx.op.decode.huffbits));
        lv_memcpy(ctx.op.decode.huffdata, jd.huffdata, sizeof(ctx.op.decode.huffdata));
        lv_memcpy(ctx.op.decode.qtdata, jd.qtdata, sizeof(ctx.op.decode.qtdata));
        lv_memcpy(ctx.op.decode.samp, jd.samp, sizeof(ctx.op.decode.samp));

        ret = ite_itjpeg_decode(&ctx);
        lv_free(workb_temp);
        if(ret < 0) {
            LV_LOG_WARN("decode itg JPEG fail");
            lv_draw_ite_it2d_free_buffer(outbuf);
            goto end;
        }

        ite_it2d_surface_t * surf;

        if(to_src_surf) {
            itg_anim->src_surf.width = frame->w0;
            itg_anim->src_surf.height = frame->h0;
            itg_anim->src_surf.pitch = ctx.width * (itg_anim->src_surf.flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565 ? 2 : 4);
            surf = &itg_anim->src_surf;
        }
        else {
            surf = &itg_anim->surf;
        }

        ite_itvp_context_t itvp_ctx = { 0 };

        itvp_ctx.input.width = ctx.width;
        itvp_ctx.input.height = ctx.height;
        itvp_ctx.input.pitch = ctx.pitch;
        itvp_ctx.input.uv_pitch = ctx.uv_pitch;
        itvp_ctx.input.format = yuv444 ? ITE_ITVP_PIXEL_FORMAT_YUV444 : ITE_ITVP_PIXEL_FORMAT_YUV422;
        itvp_ctx.input.data[0] = ctx.outbuf[0];
        itvp_ctx.input.data[1] = ctx.outbuf[1];
        itvp_ctx.input.data[2] = ctx.outbuf[2];
        itvp_ctx.output.width = ctx.width;
        itvp_ctx.output.height = ctx.height;
        itvp_ctx.output.pitch = surf->pitch;
        itvp_ctx.output.format = (surf->flags.format == ITE_IT2D_PIXEL_FORMAT_RGB_565) ? ITE_ITVP_PIXEL_FORMAT_DITHER565 :
                                 ITE_ITVP_PIXEL_FORMAT_RGB888;
        itvp_ctx.output.data[0][0] = surf->buf;

        ret = ite_itvp_ctx_init(&itvp_ctx);
        if(ret < 0) {
            lv_draw_ite_it2d_free_buffer(outbuf);
            LV_LOG_WARN("ite_itvp_ctx_init fail");
            goto end;
        }

        ite_it2d_wait_surface_finish(surf);

        buf_size = surf->pitch * surf->height;
        lv_sys_cache_data_invd_range(surf->buf, buf_size);

        ret = ite_itvp_process(&itvp_ctx);
        lv_draw_ite_it2d_free_buffer(outbuf);
        if(ret < 0) {
            LV_LOG_WARN("ite_itvp_process fail");
            goto end;
        }

        if(surf->flags.format == ITE_IT2D_PIXEL_FORMAT_ARGB_8888) {
            int i;

            if(jpeg->alpha_size > 0) {
                int j = 0;
                for(i = 3; i < buf_size; i += 4) {
                    surf->buf[i] = alpha_buf[j++];
                }
            }
            else {
                for(i = 3; i < buf_size; i += 4) {
                    surf->buf[i] = 0xFF;
                }
            }
            lv_sys_cache_data_flush_range(surf->buf, buf_size);
        }
    }
    else {
        LV_LOG_WARN("invalid itg frame");
        goto end;
    }

    if(to_src_surf) {
        itg_anim->src_surf.width = frame->w0;
        itg_anim->src_surf.height = frame->h0;

        ite_it2d_ctx_init(&it2d_ctx);
        ite_it2d_set_dst_surface(&it2d_ctx, &itg_anim->surf);
        ite_it2d_set_dst_pos(&it2d_ctx, frame->x0, frame->y0);
        ite_it2d_set_src_surface(&it2d_ctx, &itg_anim->src_surf);

        if(frame->flags.bop == PNG_BLEND_OP_OVER) {
            ite_it2d_enable_blending(&it2d_ctx, true);
        }

        ret = ite_it2d_copy(&it2d_ctx);
        if(ret < 0) {
            LV_LOG_WARN("copy surface fail");
        }
    }

end:
    itg_anim->delay = frame->delay;
    itg_anim->dop = frame->flags.dop;

    itg_anim->frame++;
    if(itg_anim->frame >= itg_anim->anim->frame_count) {
        itg_anim->frame = 0;

        if(itg_anim->anim->play_count > 0) {
            itg_anim->play++;
            if(itg_anim->play >= itg_anim->anim->play_count) {
                itg_anim->play = 0;
                itg_anim->end_play = true;
            }
        }
        else {
            itg_anim->end_play = true;
        }
    }

    return 1;
}

static void itg_anim_close(lv_itg_anim_t * itg_anim)
{
    if(itg_anim->prev_surf.buf) {
        ite_it2d_wait_surface_finish(&itg_anim->prev_surf);
        lv_draw_ite_it2d_free_buffer(itg_anim->prev_surf.buf);
        itg_anim->prev_surf.buf = NULL;
    }
    if(itg_anim->src_surf_buf) {
        lv_draw_ite_it2d_destroy_surface(&itg_anim->src_surf);
        lv_draw_ite_it2d_free_buffer(itg_anim->src_surf_buf);
        itg_anim->src_surf_buf = NULL;
    }
    lv_draw_ite_it2d_destroy_surface(&itg_anim->surf);
    if(itg_anim->surf_buf) {
        lv_draw_ite_it2d_free_buffer(itg_anim->surf_buf);
        itg_anim->surf_buf = NULL;
    }
    if(itg_anim->anim && itg_anim->img_src_file) {
        lv_draw_ite_it2d_free_buffer(itg_anim->anim);
        itg_anim->anim = NULL;
    }
    if(itg_anim->ref_alpha_buf) {
        LV_ASSERT(itg_alpha_buffer.refcount > 0);
        if(--itg_alpha_buffer.refcount == 0) {
            if(itg_alpha_buffer.buf) {
                ite_it2d_wait_idle();
                lv_draw_ite_it2d_free_buffer(itg_alpha_buffer.buf);
                itg_alpha_buffer.buf = NULL;
            }
            itg_alpha_buffer.size = 0;
        }
    }
}

static size_t input_func(JDEC * jd, uint8_t * buff, size_t ndata)
{
    io_source_t * io = jd->device;

    if(!io) return 0;

    const uint32_t bytes_left = io->data_size - io->next_read_pos;
    const uint32_t to_read = ndata <= bytes_left ? (uint32_t)ndata : bytes_left;
    if(to_read == 0)
        return 0;
    if(buff) {
        lv_memcpy(buff, io->data + io->next_read_pos, to_read);
    }
    io->next_read_pos += to_read;
    return to_read;

    return 0;
}

#endif /*LV_USE_ITG*/


