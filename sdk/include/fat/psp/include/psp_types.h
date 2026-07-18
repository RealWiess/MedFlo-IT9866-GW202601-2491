/***************************************************************************
 *
 *            Copyright (c) 2011-2019 by HCC Embedded
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
#ifndef PSP_TYPES_H_
#define PSP_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#include "../../version/ver_psp_types.h"
#if VER_PSP_TYPES_MAJOR != 1 || VER_PSP_TYPES_MINOR != 2
 #error Incompatible PSP_TYPES version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT8_MAX
 #define UINT8_MAX  0xffu
#endif
#ifndef UINT16_MAX
 #define UINT16_MAX 0xffffu
#endif
#ifndef UINT32_MAX
 #define UINT32_MAX 0xffffffffu
#endif

typedef char  char_t;

#ifndef FALSE
 #define FALSE 0u
#endif
#ifndef TRUE
 #define TRUE  1u
#endif

/* 340 S : MISRA-C:2004 19.7, MISRA-C:2012/AMD1/TC1 D.4.9: Use of function like macro. */
/* 78 S : MISRA-C:2004 19.10, MISRA-C:2012/AMD1/TC1 R.20.7: Macro parameter not in brackets. */
/*LDRA_INSPECTED 340 S*/
/*LDRA_INSPECTED 78 S*/
#define HCC_UNUSED_ARG( arg ) (void)( arg )

#ifdef __cplusplus
}
#endif

#endif /* ifndef PSP_TYPES_H_ */

