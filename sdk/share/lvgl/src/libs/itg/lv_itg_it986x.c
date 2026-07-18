/**
 * @file lv_itg_it986x.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../draw/lv_image_decoder_private.h"
#include "../../../lvgl.h"
#if LV_USE_ITG && !LV_USE_ITG_SW && (CFG_CHIP_FAMILY == 9860)

#include "../../core/lv_global.h"
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

#define DECODER_NAME                    "ITG"
#define image_cache_draw_buf_handlers   &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)
#define TJPGD_WORKBUFF_SIZE             4096    //Recommended by TJPGD libray

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    itg_header_t header;
    bool ref_alpha_buf;
} lv_itg_t;

typedef struct {
    uint8_t * data;
    size_t data_size;
    size_t next_read_pos;
} io_source_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * src, lv_image_header_t * header);
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void decoder_close(lv_image_decoder_t * dec, lv_image_decoder_dsc_t * dsc);
static uint8_t * itg_load_file(lv_image_decoder_dsc_t * dsc);
static lv_draw_buf_t * decode_itg_data(const uint8_t * itg_data, lv_image_decoder_dsc_t * dsc);
/**********************
 *  GLOBAL VARIABLES
 **********************/
itg_alpha_buffer_t itg_alpha_buffer;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Register the ITG decoder functions in LVGL
 */
void lv_itg_init(void)
{
    lv_image_decoder_t * dec = lv_image_decoder_create();
    lv_image_decoder_set_info_cb(dec, decoder_info);
    lv_image_decoder_set_open_cb(dec, decoder_open);
    lv_image_decoder_set_close_cb(dec, decoder_close);

    dec->name = DECODER_NAME;
}

void lv_itg_deinit(void)
{
    lv_image_decoder_t * dec = NULL;
    while((dec = lv_image_decoder_get_next(dec)) != NULL) {
        if(dec->info_cb == decoder_info) {
            lv_image_decoder_delete(dec);
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Get info about a ITG image
 * @param decoder   pointer to the decoder where this function belongs
 * @param dsc       image descriptor containing the source and type of the image and other info.
 * @param header    image information is set in header parameter
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't get the info
 */
static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header)
{
    LV_UNUSED(decoder); /*Unused*/

    lv_image_src_t src_type = dsc->src_type;          /*Get the source type*/

    if(src_type == LV_IMAGE_SRC_FILE || src_type == LV_IMAGE_SRC_VARIABLE) {
        uint16_t * size;
        static const uint8_t magic[] = {'I', 'T', 'G', '\0'};
        uint8_t buf[8];

        /*If it's a ITG file...*/
        if(src_type == LV_IMAGE_SRC_FILE) {
            /* Read the width and height from the file. They have a constant location:
             * [4..5]: width
             * [6..7]: height
            */
            uint32_t rn;
            lv_fs_read(&dsc->file, buf, sizeof(buf), &rn);

            if(rn != sizeof(buf)) return LV_RESULT_INVALID;

            if(lv_memcmp(buf, magic, sizeof(magic)) != 0) return LV_RESULT_INVALID;

            size = (uint16_t *)&buf[4];
        }
        /*If it's a ITG file in a  C array...*/
        else {
            const lv_image_dsc_t * img_dsc = dsc->src;
            const uint32_t data_size = img_dsc->data_size;
            size = (uint16_t *)((uint8_t *)img_dsc->data + 4);

            if(data_size < sizeof(magic)) return LV_RESULT_INVALID;
            if(lv_memcmp(img_dsc->data, magic, sizeof(magic)) != 0) return LV_RESULT_INVALID;
        }

        /*Save the data in the header*/
        header->cf = LV_COLOR_FORMAT_ARGB8888;
        header->w = size[0];
        header->h = size[1];
        header->stride = header->w * 4;

        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;         /*If didn't succeeded earlier then it's an error*/
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
}

/**
 * Open a ITG image and decode it into dsc.decoded
 * @param decoder   pointer to the decoder where this function belongs
 * @param dsc       decoded image descriptor
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);
    LV_PROFILER_DECODER_BEGIN_TAG("lv_itg_decoder_open");

    lv_itg_t * itg = (lv_itg_t *) dsc->user_data;
    if(itg == NULL) {
        itg = lv_zalloc(sizeof(lv_itg_t));
        LV_ASSERT_MALLOC(itg);
        dsc->user_data = itg;
    }

    uint8_t * itg_data = NULL;
    if(dsc->src_type == LV_IMAGE_SRC_FILE) {
        /*Load the file*/
        itg_data = itg_load_file(dsc);
        if(itg_data == NULL) {
            LV_LOG_WARN("load %s failed\n", (char *)dsc->src);
            LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
            return LV_RESULT_INVALID;
        }
    }
    else if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = dsc->src;
        lv_memcpy(&itg->header, img_dsc->data, sizeof(itg_header_t));
        itg_data = (uint8_t *)img_dsc->data + sizeof(itg_header_t);
    }
    else {
        LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_t * decoded = decode_itg_data(itg_data, dsc);

    if(dsc->src_type == LV_IMAGE_SRC_FILE) lv_free((void *)itg_data);

    if(!decoded) {
        LV_LOG_WARN("Error decoding ITG");
        LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
        return LV_RESULT_INVALID;
    }

    dsc->decoded = decoded;

    if(dsc->args.no_cache) {
        LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
        return LV_RESULT_OK;
    }

    /*If the image cache is disabled, just return the decoded image*/
    if(!lv_image_cache_is_enabled()) {
        LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
        return LV_RESULT_OK;
    }

    /*Add the decoded image to the cache*/
    lv_image_cache_data_t search_key;
    search_key.src_type = dsc->src_type;
    search_key.src = dsc->src;
    search_key.slot.size = decoded->data_size;

    lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);

    if(entry == NULL) {
        LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
        return LV_RESULT_INVALID;
    }
    dsc->cache_entry = entry;

    LV_PROFILER_DECODER_END_TAG("lv_itg_decoder_open");
    return LV_RESULT_OK;
}

/**
 * Close PNG image and free data
 * @param decoder   pointer to the decoder where this function belongs
 * @param dsc       decoded image descriptor
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);

    if(dsc->args.no_cache ||
       !lv_image_cache_is_enabled()) lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);

    lv_itg_t * itg = (lv_itg_t *) dsc->user_data;
    if(!itg) return;

    if(itg->ref_alpha_buf) {
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
    lv_free(itg);
    dsc->user_data = NULL;
}

static uint8_t * itg_load_file(lv_image_decoder_dsc_t * dsc)
{
    lv_itg_t * itg = (lv_itg_t *) dsc->user_data;
    LV_ASSERT(itg);
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, dsc->src, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) return NULL;

    uint32_t br;
    res = lv_fs_read(&f, &itg->header, sizeof(itg_header_t), &br);
    if((res != LV_FS_RES_OK) || (br != sizeof(itg_header_t))) {
        lv_fs_close(&f);
        return LV_RES_INV;
    }

    uint8_t * out = lv_malloc(itg->header.size + 7);
    if(!out) {
        lv_fs_close(&f);
        return NULL;
    }

    res = lv_fs_read(&f, out, itg->header.size + 7, &br);
    lv_fs_close(&f);

    if(res != LV_FS_RES_OK) return NULL;
    if(br < itg->header.size) return NULL;

    return out;
}

static lv_draw_buf_t * decode_itg_data(const uint8_t * itg_data, lv_image_decoder_dsc_t * dsc)
{
    lv_draw_buf_t * decoded = NULL;
    lv_itg_t * itg = (lv_itg_t *) dsc->user_data;
    LV_ASSERT(itg);
    int ret;

    if(itg->header.flags & ITG_CRC32) {
        uint8_t * crc_data = (uint8_t *)LV_ROUND_UP((uint32_t)itg_data + itg->header.size, 4);
        uint32_t crc = 0;
        ret = ite_itdpu_crc32((const uint8_t *)&itg->header, sizeof(itg_header_t), &crc);
        ret |= ite_itdpu_crc32(itg_data, LV_ROUND_UP(itg->header.size, 4), &crc);
        if((ret < 0) || lv_memcmp(&crc, crc_data, 4)) {
            LV_LOG_WARN("CRC verify fail");
            return NULL;
        }
    }

    decoded = lv_zalloc(sizeof(lv_draw_buf_t));
    LV_ASSERT_MALLOC(decoded);

    decoded->header.w = itg->header.width;
    decoded->header.h = itg->header.height;
    decoded->header.cf = (itg->header.pixel_format == ITG_RGB565) ? LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_ARGB8888;

    if((itg->header.itg_type >= ITG_JPEG_RAW) && (itg->header.itg_type <= ITG_JPEG_SPEEDY)) {
        int width_align = (itg->header.flags & ITG_JPEG_YCBCR444) ? 8 : 16;
        decoded->header.stride = LV_ROUND_UP(itg->header.width,
                                             width_align) * ((itg->header.pixel_format == ITG_RGB565) ? 2 : 4);
        decoded->data_size = decoded->header.stride * LV_ROUND_UP(itg->header.height, 16);
    }
    else {
        decoded->header.stride = itg->header.width * ((itg->header.pixel_format == ITG_RGB565) ? 2 : 4);
        decoded->data_size = decoded->header.stride * itg->header.height;
    }

    if(itg->header.itg_type == ITG_RAW) {
        if(dsc->src_type == LV_IMAGE_SRC_FILE) {
            uint8_t * data = lv_draw_ite_it2d_alloc_buffer(decoded->data_size);
            if(data == NULL) {
                lv_free(decoded);
                return NULL;
            }
            lv_memcpy(data, itg_data, decoded->data_size);
            lv_sys_cache_data_flush_range(data, decoded->data_size);

            decoded->data = decoded->unaligned_data = data;
        }
        else {
            decoded->data = decoded->unaligned_data = (uint8_t *)itg_data;
        }
    }
    else if((itg->header.itg_type == ITG_UCL) || (itg->header.itg_type == ITG_SPEEDY)) {
        uint8_t * data = lv_draw_ite_it2d_alloc_buffer(decoded->data_size);
        if(data == NULL) {
            lv_free(decoded);
            return NULL;
        }

        ite_itdcps_dsc_t dcps_dsc = { 0 };
        dcps_dsc.dst_size = decoded->data_size;
        dcps_dsc.dst = data;
        dcps_dsc.src_size = itg->header.size;
        dcps_dsc.src = itg_data;
        dcps_dsc.flags.mode = (itg->header.itg_type == ITG_UCL) ? ITE_ITDCPS_MODE_UCL_DEC : ITE_ITDCPS_MODE_SPEEDY_DEC;

        ret = ite_itdcps_decompress(&dcps_dsc);
        if(ret < 0) {
            lv_draw_ite_it2d_free_buffer(data);
            lv_free(decoded);
            return NULL;
        }

        decoded->data = decoded->unaligned_data = data;
    }
    else if((itg->header.itg_type >= ITG_JPEG_RAW) && (itg->header.itg_type <= ITG_JPEG_SPEEDY)) {
        itg_jpeg_t * jpeg = (itg_jpeg_t *) itg_data;
        uint8_t * jpeg_data = (uint8_t *)itg_data + sizeof(itg_jpeg_t);
        uint8_t * alpha_buf;
        uint32_t buf_size;

        if(itg->header.itg_type == ITG_JPEG_RAW) {
            alpha_buf = jpeg_data;
        }
        else {
            LV_ASSERT(jpeg->alpha_size > 0);
            int width_align = (itg->header.flags & ITG_JPEG_YCBCR444) ? 8 : 16;
            uint32_t buf_size = LV_ROUND_UP(itg->header.width, width_align) * LV_ROUND_UP(itg->header.height, 16);
            if(itg_alpha_buffer.size < buf_size) {
                if(itg_alpha_buffer.buf) {
                    ite_it2d_wait_idle();
                    lv_draw_ite_it2d_free_buffer(itg_alpha_buffer.buf);
                    itg_alpha_buffer.buf = NULL;
                    itg_alpha_buffer.size = 0;
                }
                itg_alpha_buffer.buf = lv_draw_ite_it2d_alloc_buffer(buf_size);
                if(! itg_alpha_buffer.buf) {
                    lv_free(decoded);
                    return NULL;
                }
                itg_alpha_buffer.size = buf_size;
            }
            alpha_buf = itg_alpha_buffer.buf;

            ite_itdcps_dsc_t dcps_dsc = { 0 };
            dcps_dsc.dst_size = buf_size;
            dcps_dsc.dst = itg_alpha_buffer.buf;
            dcps_dsc.src_size = jpeg->alpha_size;
            dcps_dsc.src = jpeg_data;
            dcps_dsc.flags.mode = (itg->header.itg_type == ITG_JPEG_UCL) ? ITE_ITDCPS_MODE_UCL_DEC : ITE_ITDCPS_MODE_SPEEDY_DEC;

            ret = ite_itdcps_decompress(&dcps_dsc);
            if(ret < 0) {
                lv_free(decoded);
                return NULL;
            }
        }

        jpeg_data += jpeg->alpha_size;

        uint8_t * workb_temp = lv_malloc(TJPGD_WORKBUFF_SIZE);
        if(! workb_temp) {
            lv_free(decoded);
            return NULL;
        }
        io_source_t io;
        io.data = jpeg_data;
        io.data_size = jpeg->jpeg_size;
        io.next_read_pos = 0;

        JDEC jd;
        JRESULT rc = jd_prepare(&jd, input_func, workb_temp, (size_t)TJPGD_WORKBUFF_SIZE, &io);
        if(rc != JDR_OK) {
            lv_free(workb_temp);
            lv_free(decoded);
            return NULL;
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
            lv_free(decoded);
            return NULL;
        }

        ctx.uv_pitch = (itg->header.flags & ITG_JPEG_YCBCR444) ? ctx.pitch : ctx.pitch / 2;
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
            lv_draw_ite_it2d_free_buffer(outbuf);
            lv_free(decoded);
            return NULL;
        }

        uint8_t * unaligned_data = lv_draw_ite_it2d_alloc_buffer(decoded->data_size + 15);
        if(unaligned_data == NULL) {
            lv_draw_ite_it2d_free_buffer(outbuf);
            lv_free(decoded);
            return NULL;
        }
        decoded->unaligned_data = unaligned_data;
        decoded->data = (uint8_t *)LV_ROUND_UP((uint32_t)unaligned_data, 16);

        ite_itvp_context_t itvp_ctx = { 0 };

        itvp_ctx.input.width = ctx.width;
        itvp_ctx.input.height = ctx.height;
        itvp_ctx.input.pitch = ctx.pitch;
        itvp_ctx.input.uv_pitch = ctx.uv_pitch;
        itvp_ctx.input.format = (itg->header.flags & ITG_JPEG_YCBCR444) ? ITE_ITVP_PIXEL_FORMAT_YUV444 :
                                ITE_ITVP_PIXEL_FORMAT_YUV422;
        itvp_ctx.input.data[0] = ctx.outbuf[0];
        itvp_ctx.input.data[1] = ctx.outbuf[1];
        itvp_ctx.input.data[2] = ctx.outbuf[2];
        itvp_ctx.output.width = ctx.width;
        itvp_ctx.output.height = ctx.height;
        itvp_ctx.output.pitch = ctx.width * ((itg->header.pixel_format == ITG_ARGB8888) ? 4 : 2);
        itvp_ctx.output.format = (itg->header.pixel_format == ITG_ARGB8888) ? ITE_ITVP_PIXEL_FORMAT_RGB888 :
                                 ITE_ITVP_PIXEL_FORMAT_DITHER565;
        itvp_ctx.output.data[0][0] = decoded->data;

        ret = ite_itvp_ctx_init(&itvp_ctx);
        if(ret < 0) {
            lv_draw_ite_it2d_free_buffer(outbuf);
            lv_draw_ite_it2d_free_buffer(unaligned_data);
            lv_free(decoded);
            return NULL;
        }

        lv_sys_cache_data_invd_range(decoded->data, decoded->data_size);

        ret = ite_itvp_process(&itvp_ctx);
        lv_draw_ite_it2d_free_buffer(outbuf);
        if(ret < 0) {
            lv_draw_ite_it2d_free_buffer(unaligned_data);
            lv_free(decoded);
            return NULL;
        }

        if(itg->header.pixel_format == ITG_ARGB8888) {
            int i;

            if(jpeg->alpha_size > 0) {
                int j = 0;
                for(i = 3; i < decoded->data_size; i += 4) {
                    decoded->data[i] = alpha_buf[j++];
                }
            }
            else {
                for(i = 3; i < decoded->data_size; i += 4) {
                    decoded->data[i] = 0xFF;
                }
            }
            lv_sys_cache_data_flush_range(decoded->data, decoded->data_size);
        }

        if(alpha_buf == itg_alpha_buffer.buf) {
            itg->ref_alpha_buf = true;
            itg_alpha_buffer.refcount++;
        }
    }
    else {
        LV_LOG_WARN("unsupported itg type: %d", itg->header.itg_type);
        lv_free(decoded);
        return NULL;
    }
    decoded->header.flags = LV_IMAGE_FLAGS_MODIFIABLE | LV_IMAGE_FLAGS_ALLOCATED | LV_DRAW_ITE_IT2D_READY;
    decoded->header.magic = LV_IMAGE_HEADER_MAGIC;
    decoded->handlers = image_cache_draw_buf_handlers;
    return decoded;
}

#endif /*LV_USE_ITG*/


