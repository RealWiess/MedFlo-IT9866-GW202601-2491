/**
 * @file lv_draw_ite_it2d_surf_cache.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_surf_cache.h"
#if LV_USE_DRAW_ITE_IT2D
#include "../../misc/lv_lru.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

static void draw_cache_free_value(void *);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_lru_t * lv_draw_ite_it2d_surf_cache;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void  lv_draw_ite_it2d_surf_cache_init(void)
{
    lv_draw_ite_it2d_surf_cache = lv_lru_create(LV_ITE_IT2D_LRU_SIZE, 65536,
                                                draw_cache_free_value, NULL);
}

void  lv_draw_ite_it2d_surf_cache_deinit(void)
{
    lv_lru_delete(lv_draw_ite_it2d_surf_cache);
}

ite_it2d_surface_t  * lv_draw_ite_it2d_surf_cache_get(const void * key, size_t key_length)
{
    ite_it2d_surface_t * value = NULL;
    lv_lru_get(lv_draw_ite_it2d_surf_cache, key, key_length, (void **) &value);
    return value;
}

void lv_draw_ite_it2d_surf_cache_put(const void * key, size_t key_length,
                                     ite_it2d_surface_t * surf)
{
    LV_LOG_INFO("cache surface %p, %d*%d@%dbpp", surf, surf->width, surf->height,
                ite_it2d_bits_per_pixel(surf->flags.format));

    size_t size = sizeof(ite_it2d_surface_t) + (surf->flags.static_buf ? 0 : surf->pitch * surf->height);

    lv_lru_set(lv_draw_ite_it2d_surf_cache, key, key_length, surf, size);
}

bool lv_draw_ite_it2d_create_surface(ite_it2d_surface_t * surf)
{
    for(;;) {
        if(ite_it2d_create_surface(surf) == 0) {
            if(surf->flags.static_buf) {
                break;
            }
            else {
                uint8_t * buf = lv_malloc(surf->pitch * surf->height);
                if(buf != NULL) {
                    if(surf->buf) {
                        lv_memcpy(buf, surf->buf, surf->pitch * surf->height);
                        lv_sys_cache_data_flush_range(buf, surf->pitch * surf->height);
                    }
                    else {
                        lv_sys_cache_data_invd_range(buf, surf->pitch * surf->height);
                    }
                    surf->buf = buf;
                    break;
                }
            }
        }

        size_t free_mem = lv_draw_ite_it2d_surf_cache->free_memory;
        lv_lru_remove_lru_item(lv_draw_ite_it2d_surf_cache);
        if(free_mem == lv_draw_ite_it2d_surf_cache->free_memory) {
            LV_LOG_WARN("out of surface memory");
            LV_ASSERT_MALLOC(false);
            return false;
        }
    }
    return true;
}

void lv_draw_ite_it2d_destroy_surface(ite_it2d_surface_t * surf)
{
    ite_it2d_wait_surface_finish(surf);
    if(!surf->flags.static_buf) {
        lv_free(surf->buf - surf->flags.buf_offset);
    }
    surf->buf = NULL;
}

void * lv_draw_ite_it2d_alloc_buffer(uint32_t size)
{
    uint8_t * buf = NULL;
    for(;;) {
        buf = lv_malloc(size);
        if(buf) {
            break;
        }
        size_t free_mem = lv_draw_ite_it2d_surf_cache->free_memory;
        lv_lru_remove_lru_item(lv_draw_ite_it2d_surf_cache);
        if(free_mem == lv_draw_ite_it2d_surf_cache->free_memory) {
            LV_LOG_WARN("out of buffer memory");
            LV_ASSERT_MALLOC(false);
            return NULL;
        }
    }
    return buf;
}

void lv_draw_ite_it2d_free_buffer(void * buf)
{
    lv_free(buf);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void draw_cache_free_value(void * value)
{
    ite_it2d_surface_t * surf = (ite_it2d_surface_t *)value;
    LV_LOG_INFO("destroy surface %p", surf);
    ite_it2d_wait_surface_finish(surf);
    if(!surf->flags.static_buf) {
        lv_free(surf->buf - surf->flags.buf_offset);
    }
    surf->buf = NULL;
    lv_free(surf);
}

#endif /*LV_USE_DRAW_ITE_IT2D*/

