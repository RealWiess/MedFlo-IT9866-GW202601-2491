/**
 * @file lv_itg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../draw/lv_image_decoder_private.h"
#include "../../../lvgl.h"
#if LV_USE_ITG_SW

#include "../../core/lv_global.h"
#include "lv_itg_private.h"
#include "../tjpgd/tjpgd.h"
#include "ucl/ucl.h"
#include "speedy_comp.h"

/*********************
 *      DEFINES
 *********************/

#define DECODER_NAME                    "ITG"
#define image_cache_draw_buf_handlers   &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)
#define TJPGD_WORKBUFF_SIZE             4096    //Recommended by TJPGD libray
#define ITDPU_DEVICE                    DEVICE_DT_NAME(DT_NODELABEL(dpu))

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    itg_header_t header;
} lv_itg_t;

typedef struct {
    uint8_t * data;
    size_t data_size;
    size_t next_read_pos;
    lv_draw_buf_t * decoded;
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

static int img_data_cb(JDEC * jd, void * bitmap, JRECT * rect)
{
    io_source_t * io               = jd->device;
    lv_draw_buf_t * decoded        = io->decoded;
    uint8_t   *   dst              = decoded->data;
    uint8_t   *   src              = bitmap;


    if(decoded->header.cf == LV_COLOR_FORMAT_RGB565) {
        for(int y = rect->top; y <= rect->bottom; y++) {
            for(int x = rect->left; x <= rect->right; x++) {
                int offset = y * decoded->header.stride + x * 2;
                int i = ((y - rect->top) * (rect->right - rect->left + 1) + (x - rect->left)) * 3;
                uint8_t r = src[i + 0];
                uint8_t g = src[i + 1];
                uint8_t b = src[i + 2];
                uint16_t pixel = ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3);
                dst[offset] = pixel & 0xFF;
                dst[offset + 1] = (pixel >> 8) & 0xFF;
            }
        }

    }
    else {
        for(int y = rect->top; y <= rect->bottom; y++) {
            for(int x = rect->left; x <= rect->right; x++) {
                int offset = y * decoded->header.stride + x * 4;
                int i = ((y - rect->top) * (rect->right - rect->left + 1) + (x - rect->left)) * 3;
                uint8_t r = src[i + 0];
                uint8_t g = src[i + 1];
                uint8_t b = src[i + 2];
                dst[offset] = r;
                dst[offset + 1] = g;
                dst[offset + 2] = b;
            }
        }

    }

    return 1;
}

static uint32_t crc32_ieee_update(uint32_t crc, const uint8_t * data, size_t len)
{
    /* crc table generated from polynomial 0xedb88320 */
    static const uint32_t table[16] = {
        0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
        0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
        0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
        0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
    };

    crc = ~crc;

    for(size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        crc          = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
        crc          = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
    }

    return (~crc);
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
        uint32_t crc = crc32_ieee_update(0x0, (const uint8_t *)&itg->header, sizeof(itg_header_t));
        crc = crc32_ieee_update(crc, itg_data, LV_ROUND_UP(itg->header.size, 4));
        if(lv_memcmp(&crc, crc_data, 4)) {
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
            uint8_t * data = lv_malloc(decoded->data_size);
            if(data == NULL) {
                lv_free(decoded);
                return NULL;
            }
            lv_memcpy(data, itg_data, decoded->data_size);

            decoded->data = decoded->unaligned_data = data;
        }
        else {
            decoded->data = decoded->unaligned_data = (uint8_t *)itg_data;
        }
    }
    else if((itg->header.itg_type == ITG_UCL) || (itg->header.itg_type == ITG_SPEEDY)) {
        uint8_t * data = lv_malloc(decoded->data_size);
        if(data == NULL) {
            lv_free(decoded);
            return NULL;
        }

        if(itg->header.itg_type == ITG_UCL) {
            if(ucl_init() != UCL_E_OK) {
                lv_free(data);
                lv_free(decoded);
                return NULL;
            }

            uint8_t * src = (uint8_t *)itg_data + 18;
            uint8_t * dst = data;
            uint32_t  out_size, in_size;

            out_size = in_size = 0;

            while((out_size < decoded->data_size) && (in_size < itg->header.size)) {
                uint32_t out_len, in_len;

                out_len = ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) |
                           (src[3]));
                src += 4;
                in_len = ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) |
                          (src[3]));
                src += 4;

                if(out_len == in_len) {
                    lv_memcpy(dst, src, out_len);
                }
                else {
                    ret = out_len;
                    if((ucl_nrv2e_decompress_8(src, in_len, dst, &ret, NULL) !=
                        UCL_E_OK) ||
                       (ret != out_len)) {
                        LV_LOG_WARN("ucl_nrv2e_decompress_8 fail");
                        lv_free(data);
                        lv_free(decoded);
                        return NULL;
                    }
                }

                dst += out_len;
                src += in_len;
                out_size += out_len;
                in_size += 8 + in_len;
            };
        }
        else {
            myLZParaType para = {4, 11, 4096, 6};
            ret = myLZ_depack((const void *)((uint8_t *)itg_data + 14), (void *)data, decoded->data_size, 4096, &para);
            if(ret != decoded->data_size) {
                LV_LOG_WARN("ucl_nrv2e_decompress_8 fail");
                lv_free(data);
                lv_free(decoded);
                return NULL;
            }
        }

        decoded->data = decoded->unaligned_data = data;
    }
    else if((itg->header.itg_type >= ITG_JPEG_RAW) && (itg->header.itg_type <= ITG_JPEG_SPEEDY)) {
        itg_jpeg_t * jpeg = (itg_jpeg_t *) itg_data;
        uint8_t * jpeg_data = (uint8_t *)itg_data + sizeof(itg_jpeg_t);
        uint8_t * alpha_buf;

        if(itg->header.itg_type == ITG_JPEG_RAW) {
            alpha_buf = jpeg_data;
        }
        else {
            LV_ASSERT(jpeg->alpha_size > 0);
            int width_align = (itg->header.flags & ITG_JPEG_YCBCR444) ? 8 : 16;
            uint32_t buf_size = LV_ROUND_UP(itg->header.width, width_align) * LV_ROUND_UP(itg->header.height, 16);
            alpha_buf = lv_malloc(buf_size);

            if(itg->header.itg_type == ITG_JPEG_UCL) {
                if(ucl_init() != UCL_E_OK) {
                    lv_free(alpha_buf);
                    lv_free(decoded);
                    return NULL;
                }

                uint8_t * src = jpeg_data + 18;
                uint8_t * dst = alpha_buf;
                uint32_t  out_size, in_size;

                out_size = in_size = 0;

                while((out_size < buf_size) && (in_size < jpeg->alpha_size)) {
                    uint32_t out_len, in_len;

                    out_len = ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) |
                               (src[3]));
                    src += 4;
                    in_len = ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) |
                              (src[3]));
                    src += 4;

                    if(out_len == in_len) {
                        lv_memcpy(dst, src, out_len);
                    }
                    else {
                        ret = out_len;
                        if((ucl_nrv2e_decompress_8(src, in_len, dst, &ret,
                                                   NULL) != UCL_E_OK) ||
                           (ret != out_len)) {
                            LV_LOG_WARN("ucl_nrv2e_decompress_8 fail");
                            lv_free(alpha_buf);
                            lv_free(decoded);
                            return NULL;
                        }
                    }

                    dst += out_len;
                    src += in_len;
                    out_size += out_len;
                    in_size += 8 + in_len;
                };
            }
            else {
                myLZParaType para = {4, 11, 4096, 6};
                ret = myLZ_depack((const void *)jpeg_data, (void *)alpha_buf, decoded->data_size, 4096, &para);
                if(ret != decoded->data_size) {
                    LV_LOG_WARN("ucl_nrv2e_decompress_8 fail");
                    lv_free(alpha_buf);
                    lv_free(decoded);
                    return NULL;
                }
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

        uint8_t * unaligned_data = lv_malloc(decoded->data_size + 15);
        if(unaligned_data == NULL) {
            lv_free(workb_temp);
            lv_free(decoded);
            return NULL;
        }
        decoded->unaligned_data = unaligned_data;
        decoded->data = (uint8_t *)LV_ROUND_UP((uint32_t)unaligned_data, 16);

        io.decoded = decoded;

        rc = jd_decomp(&jd, img_data_cb, 0); // 0 = full image decode
        if(rc != JDR_OK) {
            lv_free(workb_temp);
            lv_free(decoded);
            lv_free(unaligned_data);
            return NULL;
        }

        if(itg->header.pixel_format == ITG_ARGB8888) {
            uint32_t i;

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
        }

        if(itg->header.itg_type != ITG_JPEG_RAW) {
            lv_free(alpha_buf);
        }
    }
    else {
        LV_LOG_WARN("unsupported itg type: %d", itg->header.itg_type);
        lv_free(decoded);
        return NULL;
    }
    decoded->header.flags = LV_IMAGE_FLAGS_MODIFIABLE | LV_IMAGE_FLAGS_ALLOCATED;
    decoded->header.magic = LV_IMAGE_HEADER_MAGIC;
    decoded->handlers = image_cache_draw_buf_handlers;
    return decoded;
}

#endif /*LV_USE_ITG*/


