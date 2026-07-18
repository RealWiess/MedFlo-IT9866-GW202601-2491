/***************************************************************************
 *
 *            Copyright (c) 2003-2018 by HCC Embedded
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
 * Vaci Ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _API_MDRIVER_MST_H_
#define _API_MDRIVER_MST_H_

#include "api_mdriver.h"

#include "../version/ver_mdriver_mst.h"
#if VER_MDRIVER_MST_MAJOR != 1 || VER_MDRIVER_MST_MINOR != 3
 #error Incompatible MDRIVER_MST version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

F_DRIVER * mst_initfunc ( unsigned long driver_param );


enum
{
  MST_NO_ERROR
  , MST_ERROR = 101
};

#ifdef __cplusplus
}
#endif

#endif /* _MST_H_ */

