///*****************************************
//  Copyright (C) 2009-2018
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
#include <stdio.h>
#include <stdlib.h>
#include "hdmirx/it66021/it660x.h"
#include "hdmirx/it66021/IO.h"
#include "hdmirx/it66021/Utility.h"
#include "hdmirx/it66021/debug.h"
#include "hdmirx/it66021/it6602_interface.h"

// =============================================================================
//                Constant Definition
// =============================================================================
#define ECCTIMEOUT 5
// =============================================================================
//                Macro Definition
// =============================================================================

// =============================================================================
//                Structure Definition
// =============================================================================

// =============================================================================
//                Global Data Definition
// =============================================================================

// =============================================================================
//                Private Function Definition
// =============================================================================
static unsigned int
IT6602_OPTION (
    IT6602_PROPERTY property)
{
    unsigned int value;
    switch (property)
    {
        case IT6602_IS_DVI_MODE:
            value = (IT6602getHDMIMode()? 0: 1);
            break;

        case IT6602_CHECK_ENGINE_IDLE:
            break;

        case IT6602_ASPECT_RATIO:
            switch (IT6602getDAR())
            {
                case M_AVI_FMT_ASRATIO_4_3:
                    value = IT6602_PAR_4_3;

                case M_AVI_FMT_ASRATIO_16_9:
                    value = IT6602_PAR_16_9;

                case M_AVI_FMT_ASRATIO_14_9:
                    value = IT6602_PAR_14_9;
            }
            break;

        case IT6602_OUTPUT_VIDEO_MODE:
            value = IT6602getOutputVideoMode();
            break;

        case IT6602_PCLK:
            value = IT6602getPCLK();
            break;

        case IT6602_HEIGHT:
            value = IT6602getVertActive();
            break;

        case IT6602_WIDTH:
            value = IT6602getHorzActive();
            break;

        case IT6602_HTOTAL:
            value = IT6602getHorzTotal();
            break;

        case IT6602_VTOTAL:
            value = IT6602getVertTotal();
            break;

        case IT6602_COLOR_DEPTH:
            // output color depth
            switch (IT6602getOutputColorDepth())
            {
                case 0x0:
                    value = 24;
                    break;

                case 0x01:
                    value = 30;
                    break;

                case 0x10:
                    value = 36;
                    break;
            }
            break;

        case IT6602_IS_INTERLACE:
            value = IT6602getIsInterlace(); // 1: interlace
            break;

        case IT6602_IS_VID_STABLE:
            value = IT6602getSCDT();        // 1: stable  0:unstable
            break;

        case IT6602_IS_AUDIO_ON:
            value = IT6602getIsAudioOn();
            break;

        case IT6602_IS_AUDIO_RESET:
            value = IT6602getIsAudioReset();
            break;

        case IT6602_AUDIO_SAMPLERATE:
            value = IT6602getAudioSampleFrq();
            switch (value)
            {
                case B_FS_44100:
                    value = 44100;
                    break;

                case B_FS_48000:
                    value = 48000;
                    break;

                case B_FS_32000:
                    value = 32000;
                    break;

                case B_FS_88200:
                    value = 88200;
                    break;

                case B_FS_768000:
                    value = 768000;
                    break;

                case B_FS_96000:
                    value = 96000;
                    break;

                case B_FS_176400:
                    value = 176400;
                    break;

                case B_FS_192000:
                    value = 192000;
                    break;

                case B_FS_NOTID:
                case B_FS_22050:
                case B_FS_24000:
                default:
                    value = 0;
                    break;
            }
            break;

        case IT6602_AUDIO_CHANNEL_NUMBER:
            value = IT6602getAudioCannelNum();
            break;

        case IT6602_AUDIO_MODE:
            switch (IT6602getAudioMode())
            {
                case 1:
                    value = IT6602_AUDIO_HBR;
                    break;

                case 2:
                    value = IT6602_AUDIO_DSD;
                    break;

                case 3:
                    value = IT6602_AUDIO_NLPCM;
                    break;

                case 4:
                    value = IT6602_AUDIO_LPCM;
                    break;

                case 0:
                default:
                    value = IT6602_AUDIO_OFF;
                    break;
            }
            break;

        case IT6602_COLOR_FULLRANGE:
            value = IT6602getOutputColorRange(); // 0: Limited range 1:full range
            break;

        default:
            break;
    }
    return value;
}

// =============================================================================
//                Public Function Definition
// =============================================================================

// =============================================================================

/**
 * * Device Initialize
 */

// =============================================================================
void
IT6602Initialize (
    void)
{
    (void)printf("IT6602 init\n");
    IT6602_fsm_init();
}

// =============================================================================

/**
 * Device Output Pin Tri-State.
 */

// =============================================================================
void
IT6602OutputPinTriState (
    BOOL flag)
{
    if (flag == 1)
    {
        IT6602VideoOutputEnable(0); // off
        IT6602AudioOutputEnable(0);
    }
    else
    {
        IT6602VideoOutputEnable(1); // on
        IT6602AudioOutputEnable(1);
    }
}

// =============================================================================

/**
 * check Device Signal State
 */

// =============================================================================
BOOL
IT6602IsSignalStable (
    void)
{
    static unsigned int ecc_checktimes = 0;
    IT6602_fsm();

    if (IT6602getSCDT())
    {
        if (IsVideoOn() && !IT6602getColorRangeChange() && IT6602getEQEND())
        {
            if (ecc_checktimes++ < ECCTIMEOUT)
            {
                if (IT6602getECC() == 0)
                {
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                return TRUE;
            }
        }
        else
        {
            ecc_checktimes = 0;
            return FALSE;
        }
    }
    else
    {
        ecc_checktimes = 0;
        return FALSE;
    }
}

// =============================================================================

/**
 * * Device get property.
 */

// =============================================================================
unsigned int
IT6602GetProperty (
    MMP_HDMIRX_PROPERTY property)
{
    return IT6602_OPTION(property);
}

// =============================================================================

/**
 * * Device set property.
 */

// =============================================================================
void
IT6602SetProperty (
    MMP_HDMIRX_PROPERTY property,
    unsigned int        value)
{
    switch (property)
    {
        default:
            break;
    }
}

// =============================================================================

/**
 * * Check HDCP.
 */

// =============================================================================
void
IT6602DisableHDCP (
    BOOL flag)
{
    (void)printf("(%s) no supported \n", __FUNCTION__);
}

unsigned int
IT6602IsHDCPOn (
    void)
{
    return IT6602getHDCPStatus();
}

// =============================================================================

/**
 * * Get DataEnable Timing.
 */

// =============================================================================
void
IT6602GetDETiming (
    MMP_HDMIRX_DETIMING * pDETiming)
{
    unsigned int    HSync       = IT6602getHorzSyncWidth();
    unsigned int    HBackPorch  = IT6602getHorzBackPorch();
    unsigned int    HActive     = IT6602getHorzActive();
    unsigned int    VSync       = IT6602getVertSyncWidth();
    unsigned int    VBackPorch  = IT6602getVertBackPorch();
    unsigned int    VActive     = IT6602getVertActive();

    pDETiming->HDES = HSync + HBackPorch - 2;
    pDETiming->HDEE = HSync + HBackPorch - 2 + HActive;

    pDETiming->VDES = VSync + VBackPorch - 1;
    pDETiming->VDEE = VSync + VBackPorch - 1 + VActive;

    // (void)printf("---HDES = %d, HDEE = %d, VDES = %d, VDEE = %d---\n", pDETiming->HDES, pDETiming->HDEE, pDETiming->VDES, pDETiming->VDEE);
}

// =============================================================================

/**
 * Device power down
 */

// =============================================================================
BOOL
IT6602PowerDown (
    BOOL enable)
{
    return FALSE;
}
