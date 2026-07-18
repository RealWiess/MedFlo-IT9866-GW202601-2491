/***************************************************************************
 *
 *            Copyright (c) 2010-2023 by Tuxera
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
#ifndef PSP_STRING_H_
#define PSP_STRING_H_

#include <string.h>
#include "psp_types.h"

#include "../../version/ver_psp_string.h"
#if VER_PSP_STRING_MAJOR != 1 || VER_PSP_STRING_MINOR != 10
 #error Incompatible PSP_STRING version number!
#endif

#define PSP_STRING_STRNLEN_SUPPORT  1
#define PSP_STRING_STRNICMP_SUPPORT 1
#define PSP_STRING_STRNRCHR_SUPPORT 1

/* 35 S : MISRA-C:2012/AMD1/TC1 R.2.1: Static procedure is not explicitly called in code analysed. */
/* 77 S : MISRA-C:2004 19.4: Macro replacement list needs parentheses. */
/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memcpy( d, s, l )   memcpy( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memmove( d, s, l )  memmove( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memset( d, c, l )   memset( ( d ), ( c ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memcmp( s1, s2, l ) memcmp( ( s1 ), ( s2 ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#if ( PSP_STRING_STRNLEN_SUPPORT != 0 )
 #define psp_strnlen( s, l ) strnlen( ( s ), (size_t)( l ) )
#else
 #define psp_strnlen( s, l ) strlen( s )
#endif

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncat( d, s, l )   strncat( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncpy( d, s, l )   strncpy( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncmp( s1, s2, l ) strncmp( ( s1 ), ( s2 ), (size_t)( l ) )

#if ( PSP_STRING_STRNICMP_SUPPORT != 0 )
 /*LDRA_INSPECTED 35 S*/
 /*LDRA_INSPECTED 77 S*/
 #define psp_strnicmp( s1, s2, l ) strnicmp( ( s1 ), ( s2 ), (size_t)( l ) )
#endif

#if ( PSP_STRING_STRNRCHR_SUPPORT != 0 )
 /*LDRA_INSPECTED 35 S*/
 /*LDRA_INSPECTED 77 S*/
 #define psp_strnrchr( s, c ) strnrchr( ( s ), ( c ) )
#endif

uint16_t psp_w16csnlen ( wchar16_t const * const p_src, uint16_t length );
wchar16_t * psp_w16csncat ( wchar16_t * const p_dst, wchar16_t const * const p_src, uint16_t length );
wchar16_t * psp_w16csncpy ( wchar16_t * const p_dst, wchar16_t const * const p_src, uint16_t length );
int psp_w16csncmp ( wchar16_t const * const p_src1, const wchar16_t * const p_src2, uint16_t length );
const wchar16_t * psp_w16csnchr ( wchar16_t const * const p_src, uint16_t length, wchar16_t ch );
#if ( PSP_STRING_STRNICMP_SUPPORT == 0 )
int psp_strnicmp ( const char * s1, const char * s2, uint16_t length );
#endif
#if ( PSP_STRING_STRNRCHR_SUPPORT == 0 )
char * psp_strnrchr (const char * s, int c, uint16_t length );
#endif


#endif /* ifndef PSP_STRING_H_ */

