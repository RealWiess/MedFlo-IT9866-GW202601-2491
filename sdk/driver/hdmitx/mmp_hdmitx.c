/*
 * Copyright (c) 2010 ITE. All Rights Reserved.
 */
/** @file
 * Configurations.
 *
 * @author Odin He
 * @version 1.0
 */
// #include "ite/mmp_types.h"
#include "hdmitx/typedef.h"
#include "hdmitx/hdmitx_drv.h"
#include "hdmitx/hdmitx_sys.h"
#include "hdmitx/mmp_hdmitx.h"

// =============================================================================
//                              Public Function Definition
// =============================================================================

static BOOL     gbDeviceID_66121    = TRUE;
pthread_mutex_t ghdmitx_mutex       = PTHREAD_MUTEX_INITIALIZER;

// =============================================================================

/**
 * HDMI TX initialization.
 */

// =============================================================================
void
ithHDMITXInitialize (
    MMP_HDMITX_INPUT_DEVICE inputDevice)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    InitHDMITX_Instance_66121(inputDevice);

    if (!ithHDMITXIsChipEmpty())
    {
        (void)printf("hdmi tx online \n");
    }

    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================

/**
 * HDMI TX Loop Process.
 */

// =============================================================================
void
ithHDMITXDevLoopProc (
    BOOL                    bHDMIRxModeChange,
    unsigned int            AudioSampleRate,
    unsigned int            AudioChannelNum,
    MMP_HDMITX_INPUT_DEVICE inputDevice)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    HDMITX_DevLoopProc_66121(bHDMIRxModeChange, AudioSampleRate, AudioChannelNum, inputDevice);

    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================
/**
 * HDMI TX Loop Process.
 */
// =============================================================================

void
ithHDMITXDisable (
    void)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    HDMITXDisable_66121();

    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================
/**
 * HDMI TX Loop Process.
 */
// =============================================================================

void
ithHDMITXAVMute (
    BOOL isEnable)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    if (isEnable)
    {
        SetAVMute_66121(TRUE);
    }
    else
    {
        SetAVMute_66121(FALSE);
    }
    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================

/**
 * HDMI TX Set Display Option.
 */

// =============================================================================
void
ithHDMITXSetDisplayOption (
    MMP_HDMITX_VIDEO_TYPE       VideoMode,
    MMP_HDMITX_VESA_ID          VesaTiming,
    uint16_t                    EnableHDCP,
    MMP_HDMITX_INPUT_COLOR_MODE InColorMode)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    HDMITX_ChangeDisplayOption_66121(VideoMode, VesaTiming, EnableHDCP, InColorMode);

    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================

/**
 * HDMI TX Set DE Timing.
 */

// =============================================================================
void
ithHDMITXSetDETiming (
    unsigned int    HDES,
    unsigned int    HDEE,
    unsigned int    VDES,
    unsigned int    VDEE)
{
    pthread_mutex_lock(&ghdmitx_mutex);

    timingDE_66121.HDES = HDES;
    timingDE_66121.HDEE = HDEE;
    timingDE_66121.VDES = VDES;
    timingDE_66121.VDEE = VDEE;

    pthread_mutex_unlock(&ghdmitx_mutex);
}

// =============================================================================

/**
 * HDMI TX check IC.
 */

// =============================================================================
BOOL
ithHDMITXIsChipEmpty (
    void)
{
    BYTE    regDeviceID = 0x0;
    BOOL    chipEmpty   = 0;

    regDeviceID = HDMITX_ReadI2C_Byte_66121(REG_TX_DEVICE_ID0);

    (void)printf("regDeviceID:0x%x\n", regDeviceID);
    if (regDeviceID == 0x12)
    {
        gbDeviceID_66121 = TRUE;
    }
    else
    {
        gbDeviceID_66121 = FALSE;
    }

    chipEmpty = HDMITX_IsChipEmpty_66121();

    return chipEmpty;
}

BOOL
ithHDMITXIs66121Chip (
    void)
{
    return gbDeviceID_66121;
}
