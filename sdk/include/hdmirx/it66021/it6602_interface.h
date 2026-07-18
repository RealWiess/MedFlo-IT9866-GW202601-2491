#ifndef __IT6602_INTERFACE_H__
#define __IT6602_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOOL
#define BOOL unsigned char
#endif
//=============================================================================
//                              Constant Definition
//=============================================================================
#define MMP_HDMIRX_DETIMING IT6602_DETIMING

//=============================================================================
//                              Constant Definition
//=============================================================================

typedef enum IT6602_PROPERTY_TAG
{
    IT6602_IS_DVI_MODE,
    IT6602_CHECK_ENGINE_IDLE,
    IT6602_ASPECT_RATIO,
    IT6602_OUTPUT_VIDEO_MODE,
    IT6602_PCLK,
    IT6602_HEIGHT,
    IT6602_WIDTH,
    IT6602_HTOTAL,
    IT6602_VTOTAL,
    IT6602_COLOR_DEPTH,
    IT6602_IS_INTERLACE,
    IT6602_IS_VID_STABLE,
    IT6602_IS_AUDIO_ON,
    IT6602_IS_AUDIO_RESET,
    IT6602_AUDIO_SAMPLERATE,
    IT6602_AUDIO_CHANNEL_NUMBER,
    IT6602_AUDIO_MODE,
    IT6602_CHECK_PLG5V_PWR,
    IT6602_COLOR_FULLRANGE,

} IT6602_PROPERTY;


typedef enum IT6602_AUDIO_MODES_TAG
{
    IT6602_AUDIO_OFF,
    IT6602_AUDIO_HBR,
    IT6602_AUDIO_DSD,
    IT6602_AUDIO_NLPCM,
    IT6602_AUDIO_LPCM,

} IT6602_AUDIO_MODES;

typedef enum IT6602_PAR_TAG
{
    IT6602_PAR_4_3 = 1,
    IT6602_PAR_16_9 = 2,
    IT6602_PAR_14_9 = 3,

} IT6602_PAR;

typedef enum MMP_HDMIRX_PROPERTY_TAG
{
    HDMIRX_IS_DVI_MODE              = IT6602_IS_DVI_MODE            ,
    HDMIRX_CHECK_ENGINE_IDLE        = IT6602_CHECK_ENGINE_IDLE      ,
    HDMIRX_ASPECT_RATIO             = IT6602_ASPECT_RATIO           ,
    HDMIRX_OUTPUT_VIDEO_MODE        = IT6602_OUTPUT_VIDEO_MODE      ,
    HDMIRX_PCLK                     = IT6602_PCLK                   ,
    HDMIRX_HEIGHT                   = IT6602_HEIGHT                 ,
    HDMIRX_WIDTH                    = IT6602_WIDTH                  ,
    HDMIRX_HTOTAL                   = IT6602_HTOTAL                 ,
    HDMIRX_VTOTAL                   = IT6602_VTOTAL                 ,
    HDMIRX_COLOR_DEPTH              = IT6602_COLOR_DEPTH            ,
    HDMIRX_IS_INTERLACE             = IT6602_IS_INTERLACE           ,
    HDMIRX_IS_VID_STABLE            = IT6602_IS_VID_STABLE          ,
    HDMIRX_IS_AUDIO_ON              = IT6602_IS_AUDIO_ON            ,
    HDMIRX_IS_AUDIO_RESET           = IT6602_IS_AUDIO_RESET         ,
    HDMIRX_AUDIO_SAMPLERATE         = IT6602_AUDIO_SAMPLERATE       ,
    HDMIRX_AUDIO_CHANNEL_NUMBER     = IT6602_AUDIO_CHANNEL_NUMBER   ,
    HDMIRX_AUDIO_MODE               = IT6602_AUDIO_MODE             ,
    //HDMIRX_CHECK_PLG5V_PWR          = IT6602_CHECK_PLG5V_PWR        ,
    HDMIRX_COLOR_FULLRANGE          = IT6602_COLOR_FULLRANGE        ,

} MMP_HDMIRX_PROPERTY;

typedef enum MMP_HDMIRX_AUDIO_MODE_TAG
{
    HDMIRX_AUDIO_OFF                = IT6602_AUDIO_OFF              ,
    HDMIRX_AUDIO_HBR                = IT6602_AUDIO_HBR              ,
    HDMIRX_AUDIO_DSD                = IT6602_AUDIO_DSD              ,
    HDMIRX_AUDIO_NLPCM              = IT6602_AUDIO_NLPCM            ,
    HDMIRX_AUDIO_LPCM               = IT6602_AUDIO_LPCM             ,

} MMP_HDMIRX_AUDIO_MODE;

typedef enum MMP_HDMIRX_PAR_TAG
{
    HDMIRX_PAR_4_3                  = IT6602_PAR_4_3                ,
    HDMIRX_PAR_16_9                 = IT6602_PAR_16_9               ,
    HDMIRX_PAR_14_9                 = IT6602_PAR_14_9               ,
} MMP_HDMIRX_PAR;


//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================


//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct IT6602_DETIMING_TAG
{
    unsigned int        HDES; //H DATA ENABLE START 
    unsigned int        HDEE; //H DATA ENABLE END  
    unsigned int        VDES; //V DATA ENABLE START 
    unsigned int        VDEE; //V DATA ENABLE END 
} IT6602_DETIMING;

//=============================================================================
//                Global Data Definition
//=============================================================================


//=============================================================================
//                Private Function Definition
//=============================================================================


//=============================================================================
//                Public Function Definition
//=============================================================================

//=============================================================================
/**
* Device Initialize
*/
//=============================================================================
void IT6602Initialize(void);
//=============================================================================
/**
 * Device Output Pin Tri-State.
 */
//=============================================================================
void IT6602OutputPinTriState(BOOL flag);
//=============================================================================
/**
* Device Signal State
*/
//=============================================================================
BOOL IT6602IsSignalStable(void);
//=============================================================================
/**
* Device property.
*/
//=============================================================================
unsigned int IT6602GetProperty(MMP_HDMIRX_PROPERTY property);
//=============================================================================
/**
* * Device set property.
*/
//=============================================================================
void IT6602SetProperty(MMP_HDMIRX_PROPERTY property, unsigned int value);
//=============================================================================
/**
* * Check HDCP.
*/
//=============================================================================
unsigned int IT6602IsHDCPOn(void);
void IT6602DisableHDCP(BOOL flag);
//=============================================================================
/**
* * Get DE Timing.
*/
//=============================================================================
void IT6602GetDETiming(MMP_HDMIRX_DETIMING *pDETiming);
//=============================================================================
/**
* Device power down
*/
//=============================================================================
BOOL IT6602PowerDown(BOOL enable);


#ifdef __cplusplus
}
#endif

#endif


