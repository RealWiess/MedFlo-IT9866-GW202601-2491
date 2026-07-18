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
#ifndef _API_HCC_MEM_H
#define _API_HCC_MEM_H

#include "../psp/include/psp_types.h"

#include "../version/ver_hcc_mem.h"
#if ( VER_HCC_MEM_MAJOR != 1 ) || ( VER_HCC_MEM_MINOR != 29 )
 #error Incompatible HCC_MEM version number!
#endif
#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2
 #error Incompatible OAL version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  HCC_MEM_SUCCESS = 0
  , HCC_MEM_ERROR
};

int hcc_mem_init ( void );
int hcc_mem_start ( void );
int hcc_mem_stop ( void );
int hcc_mem_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_HCC_MEM_H */

