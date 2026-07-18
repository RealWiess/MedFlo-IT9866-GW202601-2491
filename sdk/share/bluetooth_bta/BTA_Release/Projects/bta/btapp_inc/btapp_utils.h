/*****************************************************************************
**
**  Name:             btapp_utils.h
**
**  Description:     This file contains btapp utility funciton definitions
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_UTILS_H
#define BTAPP_UTILS_H


#include "btapp.h"
#include "btapp_int.h"

/* Structure used for streaming data */

typedef struct
{
    UINT16              size;       /* Size of the buffer */
    UINT8 *             p_buffer;   /* Pointer to buffer */
    UINT8 *             p_next;     /* Pointer to next byte to use in buffer */

}tBTAPP_UTILS_MEM_STREAM;

typedef struct
{
#define BTAPP_UTILS_STRM_TYPE_MEMORY        0
#define BTAPP_UTILS_STRM_TYPE_FILE          1

    UINT8               type;

#define BTAPP_UTILS_STRM_SUCCESS            0
#define BTAPP_UTILS_STRM_ERROR_OVERFLOW     1
#define BTAPP_UTILS_STRM_ERROR_FILE         2

    UINT8               status;

    union
    {
        tBTAPP_UTILS_MEM_STREAM  mem;
    } u;

} tBTAPP_UTILS_STREAM;


extern void *btapp_utils_stream_alloc(tBTAPP_UTILS_STREAM * p_stream, UINT16 buf_size);
extern  UINT16 btapp_utils_stream_used_size(tBTAPP_UTILS_STREAM * p_stream);
extern  UINT16 btapp_utils_stream_unused_size(tBTAPP_UTILS_STREAM * p_stream);
extern BOOLEAN btapp_utils_stream_ok(tBTAPP_UTILS_STREAM * p_stream);
extern BOOLEAN btapp_utils_stream_str(tBTAPP_UTILS_STREAM * p_stream, const char * p_str);
extern BOOLEAN btapp_utils_stream_u8(tBTAPP_UTILS_STREAM * p_stream, UINT8 val);
extern BOOLEAN btapp_utils_stream_u16(tBTAPP_UTILS_STREAM * p_stream, UINT16 val);
extern BOOLEAN btapp_utils_stream_u32(tBTAPP_UTILS_STREAM * p_stream, UINT32 val);
extern BOOLEAN btapp_utils_stream_u64(tBTAPP_UTILS_STREAM * p_stream, UINT64 val);
extern BOOLEAN btapp_utils_stream_arr(tBTAPP_UTILS_STREAM * p_stream, UINT8 *p_arr, UINT16 arr_size);
extern BOOLEAN btapp_utils_init_mem_stream(tBTAPP_UTILS_STREAM * p_stream,
                                        UINT8 * p_buffer,
                                        UINT16 size);
extern BOOLEAN btapp_utils_get_u8_val(tBTAPP_UTILS_STREAM *p_stream, UINT8 *p_val);
extern BOOLEAN btapp_utils_get_u16_val(tBTAPP_UTILS_STREAM *p_stream, UINT16 *p_val);
extern BOOLEAN btapp_utils_get_u32_val(tBTAPP_UTILS_STREAM *p_stream, UINT32 *p_val);
extern BOOLEAN btapp_utils_get_arr(tBTAPP_UTILS_STREAM *p_stream, UINT16 val_len, UINT8 *p_val);
extern UINT16 btapp_utils_scru_translate_and_copy_string (char *p_src, char *p_dst);
extern BOOLEAN btapp_utils_atobd(char *p_ascii, UINT8 *p_bd);
extern BOOLEAN btapp_utils_read_string(FILE *p_f, char * tag, char * name);
extern BOOLEAN btapp_utils_read_int(FILE *p_f, char * tag, int * p_val);
extern BOOLEAN btapp_utils_read_uint32(FILE *p_f, char * tag, UINT32 * p_val);
extern BOOLEAN btapp_utils_read_boolean(FILE *p_f, char * tag, BOOLEAN *val);
extern void * btapp_utils_alloc(UINT16 cb);
extern void btapp_utils_free(void **p);

#endif
