/**
 ****************************************************************************************
 *
 * @file co_bool.h
 *
 * @brief This file replaces the need to include stdint or stdbool typical headers,
 *        which may not be available in all toolchains, and adds new types
 *
 * Copyright (C) RivieraWaves 2011-2019
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

#ifndef _CO_BOOL_H_
#define _CO_BOOL_H_


/**
 ****************************************************************************************
 * @addtogroup CO_BOOL
 * @ingroup COMMON
 * @brief Common boolean standard types (removes use of stdbool).
 *
 * @{
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */


//STDBOOL---------------------------------------------------------------------------------
#if defined(CFG_RWTL) || defined(CFG_RWX1)
///Boolean type
typedef unsigned char bool;
///True value
#define true    1
///False value
#define false   0
#else
#ifdef PLATFORM_ITE_FREERTOS
#ifndef BOOL
	typedef unsigned char           BOOL;
#endif
#ifndef bool
	typedef unsigned char    bool;
#endif
#ifndef FALSE		
    #define FALSE   0
#endif

#ifndef false
	#define false 0
#endif

#ifndef TRUE
    #define TRUE    (!FALSE)
#endif

#ifndef true
	#define true (!false)
#endif

#endif
#endif
/// @} CO_BOOL
#endif // _CO_BOOL_H_
