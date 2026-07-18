/***************************************************************************
 *
 *            Copyright (c) 2011-2023 by Tuxera
 *
 * This software is copyrighted by and is the sole property of
 * Tuxera.  All rights, title, ownership, or other interests
 * in the software remain the property of Tuxera.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of Tuxera.
 *
 * Tuxera reserves the right to modify this software without notice.
 *
 * Tuxera Inc
 * Westendintie 1
 * 02160 Espoo
 * Finland
 *
 * Tel:  +358 20 764 1720
 * http: www.tuxera.com
 * email: info@tuxera.com
 *
 ***************************************************************************/
#ifndef PSP_TYPES_H_
#define PSP_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#include "../../version/ver_psp_types.h"
#if VER_PSP_TYPES_MAJOR != 1 || VER_PSP_TYPES_MINOR != 5
 #error Incompatible PSP_TYPES version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT8_MAX
 #define UINT8_MAX  ( 0xffu )
#endif
#ifndef UINT16_MAX
 #define UINT16_MAX ( 0xffffu )
#endif
#ifndef UINT32_MAX
 #define UINT32_MAX ( 0xffffffffu )
#endif
#ifndef UINT64_MAX
 #define UINT64_MAX ( (uint64_t)0xffffffffffffffffu )
#endif

typedef char  char_t;
typedef uint16_t  wchar16_t;    /* UTF-16 character */

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

/* Macro for Unicode (UTF-16) string literals */
/* _MSC_VER is defined if Visual Studio is used */
#if ( !defined( _MSC_VER ) && ( __STDC_VERSION__ <= 199901L ) )
/* Compiler version is ISO C99 or older */
/* UTF-16 string literals are L"my string" */
#define HCC_UTF( str )   L##str
#else
/* Compiler version is ISO C11 or newer */
/* UTF-16 string literals are u"my string" */
#define HCC_UTF( str )   u##str
#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef PSP_TYPES_H_ */

