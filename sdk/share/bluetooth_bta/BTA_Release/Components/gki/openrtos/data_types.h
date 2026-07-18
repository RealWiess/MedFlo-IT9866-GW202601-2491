/****************************************************************************
**
**  Name        data_types.h
**  $Header: /Bluetooth/gki/data_types.h 7     9/13/00 11:01a Jjose $
**
**  Function    this file contains common data type definitions used
**              throughout the Widcomm Bluetooth code
**
**  Date       Modification
**  -----------------------
**  3/12/99    Create
**  07/27/00    Added nettohs macro for Little Endian
**                                                                         
**  Copyright (c) 1999- 2002, Widcomm Inc., All Rights Reserved.
**  Proprietary and confidential.
**
*****************************************************************************/

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#ifndef NULL
#define NULL     0
#endif

#ifndef FALSE
#define FALSE  0
#endif

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned int   UINT32;
typedef unsigned long long UINT64;
typedef signed   int   INT32;
typedef signed   char   INT8;
typedef signed   short  INT16;
typedef unsigned char   BOOLEAN;

typedef UINT32          UINTPTR;

#ifndef TIMER_PARAM_TYPE
#define TIMER_PARAM_TYPE UINT32
#endif

typedef UINT32          TIME_STAMP;

#ifndef TRUE
#define TRUE   1
#endif

#ifndef MIN
#define MIN(x, y)   (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y)   (((x) > (y)) ? (x) : (y))
#endif

typedef unsigned char   UBYTE;

#ifdef __arm
#define PACKED  __packed
#define INLINE  __inline
#else
#define PACKED
#define INLINE
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN FALSE
#endif

#define UINT16_LOW_BYTE(x)      ((x) & 0xff)
#define UINT16_HI_BYTE(x)       ((x) >> 8)

/* MACRO definitions for safe string functions */
/* Replace standard string functions with safe functions if available */
#define BCM_STRCAT_S(x1,x2,x3)      strcat((x1),(x3))
#define BCM_STRNCAT_S(x1,x2,x3,x4)  strncat((x1),(x3),(x4))
#define BCM_STRCPY_S(x1,x2,x3)      strcpy((x1),(x3))
#define BCM_STRNCPY_S(x1,x2,x3,x4)  strncpy((x1),(x3),(x4))
#define BCM_SPRINTF_S(x1,x2,x3,x4)  sprintf((x1),(x3),(x4))
#define BCM_VSPRINTF_S(x1,x2,x3,x4) vsprintf((x1),(x3),(x4))

#define BT_MEMCPY(a,b,c)            memcpy( (a), (b), (c) )
#define BT_MEMSET(a,b,c)            memset( (a), (b), (c) )
#define BT_STRLEN(a)                strlen( (a) )
#define BT_MEMCMP(a,b,c)            memcmp( (a), (b), (c) )
#define BT_MEMCMP_OK                0                       // memcmp success

#include "bt_types.h"

#endif

