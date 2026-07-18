/*
 * Copyright (c) 2007 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE RISC 970 type definition File
 *
 */
//=============================================================================
//                              Include Files
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                              Enumeration Type Definition
//=============================================================================

//=============================================================================
//                              Constant Definition
//=============================================================================
#include "codecs_defs.h"

#define RISC1_IMAGE_MEM_TARGET          0
#define RISC2_IMAGE_MEM_TARGET          1
#define AUDIO_MESSAGE_MEM_TARGET        2
#define SHARE_MEM1_TARGET               3

#define RISC1_CPU                       0
#define RISC2_CPU                       1

#define ARMLITE_CPU                     RISC1_CPU
#define ARMLITE_CPU_IMAGE_MEM_TARGET    RISC1_IMAGE_MEM_TARGET
#define ARMLITE_CPU_SHARE_MEM0_TARGET   AUDIO_MESSAGE_MEM_TARGET
#define ARMLITE_CPU_SHARE_MEM1_TARGET   SHARE_MEM1_TARGET 

#define ALT_CPU                         RISC2_CPU
#define ALT_CPU_IMAGE_MEM_TARGET        RISC2_IMAGE_MEM_TARGET

#define RISC1_IMAGE_SIZE                RISC1_SIZE
#define RISC2_IMAGE_SIZE                (32 * 1024)
#define AUDIO_MESSAGE_SIZE              (16 * 1024)
#define SHARE_MEM1_SIZE                 ( 8 * 1024)