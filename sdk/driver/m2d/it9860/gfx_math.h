/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file gfx_math.h
 *  GFX mathematical layer API header file.
 *
 * @author Awin Huang
 * @version 1.0
 */

#ifndef __GFX_MATH_H__
#define __GFX_MATH_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "gfx.h"

/**
 * DLL export API declaration for Win32.
 */

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                              Extern Reference
//=============================================================================
extern volatile uint32_t gBlock_num;

//=============================================================================
//                              Macro Definition
//=============================================================================
#define GFX_PI  3.14159f

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct _GFX_MATRIX
{
    float m[3][3];
} GFX_MATRIX;

#ifdef CFG_M2D_HUD_ENABLE
//typedef struct _GFX_MATRIX_BLOCK
//{
//    GFX_RECTANGLE m[gBlock_num];;
//} GFX_MATRIX_BLOCK;

typedef struct _GFX_DST_SURFACE_INFO
{
    int32_t       new_dstX;
    int32_t       new_dstY;
    int32_t       new_dstWidth;
    int32_t       new_dstHeight;
} GFX_DST_SURFACE_INFO;
#endif
//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Declaration
//=============================================================================
void
gfxMatrixIdentify(
    GFX_MATRIX* mtx);

void
gfxMatrixSet(
    GFX_MATRIX* mtx,
    float v00, float v01, float v02,
    float v10, float v11, float v12,
    float v20, float v21, float v22);

void
gfxMatrixSet2(
    GFX_MATRIX* mtx,
    GFX_MATRIX* newmtx);

void
gfxMatrixMultiply(
    GFX_MATRIX* mtx,
    GFX_MATRIX* rightMtx);

void
gfxMatrixRotate(
    GFX_MATRIX* mtx,
    float       degree);

void
gfxMatrixTranslate(
    GFX_MATRIX* mtx,
    int         offsetX,
    int         offsetY);

void
gfxMatrixScale(
    GFX_MATRIX* mtx,
    float       scaleX,
    float       scaleY);

bool
gfxMatrixInverse(
    GFX_MATRIX* originMtx,
    GFX_MATRIX* inverseMtx);

void
gfxMatrixTransform(
    GFX_MATRIX* mtx,
    int         srcX,
    int         srcY,
    float*      dstX,
    float*      dstY);

bool
gfxMatrixWarpQuadToQuad(
    GFX_RECTANGLE d,
    GFX_RECTANGLE s,
    GFX_MATRIX* mtx,
    bool bInverse);

bool
gfxMatrixWarpSquareToQuad(
    GFX_RECTANGLE d,
    GFX_MATRIX* mtx,
    bool bInverse);

bool
gfxIsAffine(
    GFX_MATRIX* mtx);

#ifdef __cplusplus
}
#endif

#endif // __GFX_MATH_H__
