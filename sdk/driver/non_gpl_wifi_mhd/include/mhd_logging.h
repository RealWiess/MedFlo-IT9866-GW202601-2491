/*
 * Copyright 2018, Broadcom Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Inc.
 */

#ifndef INCLUDED_MHD_LOGGING_H_
#define INCLUDED_MHD_LOGGING_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
#define MHD_LOGGING_UART_ENABLE
*/
/*
#define MHD_LOGGING_BUFFER_ENABLE
*/

#if defined( MHD_LOGGING_UART_ENABLE )

#include <stdio.h>

#define MHD_LOG( x ) {printf x; }

#elif defined( MHD_LOGGING_BUFFER_ENABLE )


extern int mhd_logging_printf(const char *format, ...);

#define MHD_LOG( x ) {mhd_logging_printf x; }


#else /* if defined( MHD_LOGGING_BUFFER_ENABLE ) */

#define MHD_LOG( x )	//do {MY_PRINTF x; MY_PRINTF("\r");} while(0==1)

#endif /* if defined( MHD_LOGGING_BUFFER_ENABLE ) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_MHD_LOGGING_H_ */
