/**
 * @file lv_ffmpeg.h
 *
 */
#ifndef LV_FFMPEG_H
#define LV_FFMPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#if LV_USE_FFMPEG != 0
#include "../../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct ffmpeg_context_s;

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_ffmpeg_player_class;
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_ffmpeg_raw_class;

typedef enum {
    LV_FFMPEG_PLAYER_CMD_START,
    LV_FFMPEG_PLAYER_CMD_STOP,
    LV_FFMPEG_PLAYER_CMD_PAUSE,
    LV_FFMPEG_PLAYER_CMD_RESUME,
    LV_FFMPEG_PLAYER_CMD_LAST
} lv_ffmpeg_player_cmd_t;

typedef enum {
    LV_FFMPEG_RAW_CMD_START,
    LV_FFMPEG_RAW_CMD_STOP,
    LV_FFMPEG_RAW_CMD_PAUSE,
    LV_FFMPEG_RAW_CMD_RESUME,
    LV_FFMPEG_RAW_CMD_LAST
} lv_ffmpeg_raw_cmd_t;
/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create ffmpeg_player object
 * @param parent pointer to an object, it will be the parent of the new player
 * @return pointer to the created ffmpeg_player
 */
lv_obj_t * lv_ffmpeg_player_create(lv_obj_t * parent);

/**
 * Set the path of the file to be played.
 * @param obj pointer to a ffmpeg_player object
 * @param path video file path
 * @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't get the info.
 */
lv_result_t lv_ffmpeg_player_set_src(lv_obj_t * obj, const char * path, int dst_width, int dst_height);

/**
 * Set command control video player
 * @param obj pointer to a ffmpeg_player object
 * @param cmd control commands
 */
void lv_ffmpeg_player_set_cmd(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd);

/**
 * Set the video to automatically replay
 * @param obj pointer to a ffmpeg_player object
 * @param en true: enable the auto restart
 */
void lv_ffmpeg_player_set_auto_restart(lv_obj_t * obj, bool en);
/**
 * Create ffmpeg_player object
 * @param parent pointer to an object, it will be the parent of the new player
 * @return pointer to the created ffmpeg_player
 */
lv_obj_t * lv_ffmpeg_raw_create(lv_obj_t * parent);

/**
 * Set the path of the file to be played
 * @param obj pointer to a ffmpeg_player object
 * @param path video file path
 * @param dst_width video window's width
 * @param dst_height video window's height
 * @param src_width video source's width
 * @param src_height video source's height
 * @param period video frame display period
* @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't get the info.
 */
lv_result_t lv_ffmpeg_raw_set_src(lv_obj_t * obj, const char * path, int dst_width, int dst_height, int src_width, int src_height, int period);

/**
 * Set command control video player
 * @param obj pointer to a ffmpeg_player object
 * @param cmd control commands
 */
void lv_ffmpeg_raw_set_cmd(lv_obj_t * obj, lv_ffmpeg_raw_cmd_t cmd);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_FFMPEG*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_FFMPEG_H*/
