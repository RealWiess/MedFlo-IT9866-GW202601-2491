/***************************************************************************
 *
 *            Copyright (c) 2006-2020 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _USBD_CDCACM_CFG_H_
#define _USBD_CDCACM_CFG_H_

#include "../api/api_usbd_cdcacm.h"

#include "../version/ver_usbd_cdcacm.h"
#if VER_USBD_CDCACM_MAJOR != 5 || VER_USBD_CDCACM_MINOR != 6
 #error "Invalid USBD_CDCACM version."
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Set this to one if notifications shall be sent over the interrupt channel.
   This is the way defined by the standard.
   The built in windows XP driver ignores notifications. */
#define USBD_CDCACM_NOTIFY_ON_IT_EP 1


/* The number of serial lines to implement. USB configuration must match
   this value. */
#define USBD_CDCACM_N_LINES         1

/* Default settings for serial lines. */
#define USBD_CDCACM_DEFAULT_BPS     9600
#define USBD_CDCACM_DEFAULT_STOP    LCT_STOP_1
#define USBD_CDCACM_DEFAULT_PARITY  LCT_PARITY_NONE
#define USBD_CDCACM_DEFAULT_CLEN    8

/* The default Rx mode after start. Possible settings are: */
/*      - USBD_CDCACM_RXMODE_NORMAL */
/*      - USBD_CDCACM_RXMODE_DIRECT */
/* Set this to USBD_CDCACM_RXMODE_DIRECT if USBD_CDCACM_RX_BUFFER_SIZE iz zero! */
#define USBD_CDCACM_DEFAULT_RXMODE  USBD_CDCACM_RXMODE_NORMAL

/* size of a receive buffer. Please set it to the max. BULK IN endpoint */
/* packet size. Set it to zero if only Direct reception is used. */
#define USBD_CDCACM_RX_BUFFER_SIZE  ( 4 * 1024 )/*512*/ // Irene Lin

#define USBD_CDCACM_TASK_STACK_SIZE 1024

/* Number of Receive buffers on each CDC line */
#define USBD_CDCACM_BUF_NUM_LINE_0  8 // 2 // Irene Lin
#define USBD_CDCACM_BUF_NUM_LINE_1  0
#define USBD_CDCACM_BUF_NUM_LINE_2  0
#define USBD_CDCACM_BUF_NUM_LINE_3  0

#ifdef __cplusplus
}
#endif

#endif /* ifndef _USBD_CDCSER_CFG_H_ */

