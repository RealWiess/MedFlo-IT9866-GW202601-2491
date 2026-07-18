/**
 * @file lv_itg_private.h
 *
 */

#ifndef LV_ITG_PRIVATE_H
#define LV_ITG_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lv_itg.h"

#if LV_USE_ITG

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    ITG_RAW,
    ITG_UCL,
    ITG_SPEEDY,
    ITG_JPEG_RAW,
    ITG_JPEG_UCL,
    ITG_JPEG_SPEEDY,
    ITG_ANIM
} itg_type_t;

typedef enum {
    ITG_RGB565,
    ITG_ARGB8888
} itg_pixel_format_t;

#define ITG_JPEG_YCBCR444 0x1
#define ITG_CRC32         0x2

typedef struct {
    uint8_t magic[4];
    int16_t width;
    int16_t height;
    uint8_t itg_type;
    uint8_t pixel_format;
    uint16_t flags;
    uint32_t size;
} itg_header_t;

typedef struct {
    uint32_t alpha_size;
    uint32_t jpeg_size;
} itg_jpeg_t;

typedef struct {
    uint32_t refcount;
    uint32_t size;
    uint8_t * buf;
} itg_alpha_buffer_t;

extern const lv_obj_class_t lv_itg_anim_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

extern itg_alpha_buffer_t itg_alpha_buffer;

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ITG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_ITG_PRIVATE_H*/
