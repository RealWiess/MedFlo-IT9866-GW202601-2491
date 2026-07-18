/***************************************************************************
* Copyright (c) 2008 ITE Technology Corp. All Rights Reserved.
*
* @file
* Codecs Code
*
* @author Kuoping Hsu
* @version 1.0
*
***************************************************************************/
#ifndef CODECS_DEFS_H
#define CODECS_DEFS_H

/* Additional region for nand boot */
#if defined(HAVE_NANDBOOT)
    #if defined(DEBUG)
        #define BOOTSTRAP_SIZE 0x8000
    #else
        #define BOOTSTRAP_SIZE 0x2000
    #endif // defined(DEBUG)
#else
    #define BOOTSTRAP_SIZE     0x0
#endif

/* puts the plugin to array */
#define CODEC_ARRAY_SIZE       (180 * 1024)      // the maximun size of each plug-ins

/* the start address of CODEC plugin */
#if (CFG_CHIP_FAMILY == 9860)
#define CODEC_START_ADDR       (0x1000 + BOOTSTRAP_SIZE)
#elif (CFG_CHIP_FAMILY == 9830)
#define CODEC_START_ADDR       (0x0 + BOOTSTRAP_SIZE)
#endif
/* magic for normal codecs */
#define CODEC_MAGIC            0x534D3020       // "SM0 "

/* increase this every time the api struct changes */
#define CODEC_API_VERSION      0x00000002

/* machine target, no used currently */
#define TARGET_ID              0x00000220

/* the maximun size of codec plug-ins */
#if (CFG_CHIP_FAMILY == 9860)
#define RISC1_SIZE             (640 * 1024)
#elif (CFG_CHIP_FAMILY == 9830)
#define RISC1_SIZE             (320 * 1024)
#endif
#endif // __CODECS_DEFS_H__