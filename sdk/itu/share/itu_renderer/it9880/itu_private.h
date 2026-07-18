#ifndef ITU_PRIVATE_H
#define ITU_PRIVATE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CFG_M2D_HUD_ENABLE
enum
{
    HUD_TABLE_UPDATE_SUCCESS = 0,			
    HUD_TABLE_PATH_FAIL,
    HUD_TABLE_FOPEN_FAIL,
    HUD_BLOCK_SIZE_INVALID 
};
#endif

typedef struct
{
    ITUSurface surf;
#ifdef CFG_M2D_ENABLE
    GFXSurface * m2dSurf;
#endif
} M2dSurface;

#ifdef CFG_M2D_HUD_ENABLE
typedef struct
{
    ITUSurface surf;
    #ifdef CFG_M2D_ENABLE
    GFXSurface * m2dSurf;
    #endif
    GFXSurface * lcdDstSurf;
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    uint8_t * pUiBuffer[3];
    #else
    uint8_t * pUiBuffer[2];
    #endif
} HudSurface;

    #define LcdSurface HudSurface
#else
    #define LcdSurface M2dSurface
#endif

#ifdef __cplusplus
}
#endif

#endif // ITU_PRIVATE_H
