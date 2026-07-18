/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file gfx_math.c
 *  GFX mathematical layer API function file.
 *
 * @author Awin Huang
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gfx.h"
#include "gfx_math.h"
#include "msg.h"
#include "hw.h"
// =============================================================================
//                              Compile Option
// =============================================================================

// =============================================================================
//                              Extern Reference
// =============================================================================
#ifdef CFG_M2D_HUD_ENABLE
extern volatile uint32_t    gBlock_W;
extern volatile uint32_t    gBlock_H;
extern volatile uint32_t    gW_num;
extern volatile uint32_t    gH_num;
extern volatile uint32_t    gBlock_num;
extern volatile uint32_t    gHUDAddrDataNum;

GFX_MATRIX                  * transform_m   = 0;
GFX_DST_SURFACE_INFO        * dstInfo       = 0;
GFX_DST_SURFACE_INFO        * srcInfo       = 0;

GFX_RECTANGLE               * pointSrc      = 0;
GFX_RECTANGLE               * pointDst      = 0;
GFX_RECTANGLE               * pointDstFish  = 0;
#endif
// =============================================================================
//                              Macro Definition
// =============================================================================
/* True if address a has acceptable alignment */
#define is_aligned(A)       (((size_t)((A)) & (CHUNK_ALIGN_MASK)) == 0)

#undef MIN
#undef MAX
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define floatsEqual(x, y)   (fabs(x - y) <= 0.00001f * MIN(fabs(x), fabs(y)))
#define floatIsZero(x)      (floatsEqual((x) + 1, 1))

// =============================================================================
//                              Structure Definition
// =============================================================================

// =============================================================================
//                              Global Data Definition
// =============================================================================

// =============================================================================
//                              Private Function Declaration
// =============================================================================

// =============================================================================
//                              Public Function Definition
// =============================================================================
void
gfxMatrixIdentify (
    GFX_MATRIX * mtx)
{
    GFX_FUNC_ENTRY;
    mtx->m[0][0]    = 1;
    mtx->m[0][1]    = 0;
    mtx->m[0][2]    = 0;
    mtx->m[1][0]    = 0;
    mtx->m[1][1]    = 1;
    mtx->m[1][2]    = 0;
    mtx->m[2][0]    = 0;
    mtx->m[2][1]    = 0;
    mtx->m[2][2]    = 1;
    GFX_FUNC_LEAVE;
}

void
gfxMatrixSet (
    GFX_MATRIX  * mtx,
    float       v00,
    float       v01,
    float       v02,
    float       v10,
    float       v11,
    float       v12,
    float       v20,
    float       v21,
    float       v22)
{
    GFX_FUNC_ENTRY;
    mtx->m[0][0]    = v00;
    mtx->m[0][1]    = v01;
    mtx->m[0][2]    = v02;
    mtx->m[1][0]    = v10;
    mtx->m[1][1]    = v11;
    mtx->m[1][2]    = v12;
    mtx->m[2][0]    = v20;
    mtx->m[2][1]    = v21;
    mtx->m[2][2]    = v22;
    GFX_FUNC_LEAVE;
}

void
gfxMatrixSet2 (
    GFX_MATRIX  * mtx,
    GFX_MATRIX  * newmtx)
{
    GFX_FUNC_ENTRY;
    memcpy(mtx, newmtx, sizeof(GFX_MATRIX));
    GFX_FUNC_LEAVE;
}

void
gfxMatrixMultiply (
    GFX_MATRIX  * mtx,
    GFX_MATRIX  * rightMtx)
{
    GFX_MATRIX  tempMtx;
    int         i, j;

    GFX_FUNC_ENTRY;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            tempMtx.m[i][j] =
                mtx->m[i][0] * rightMtx->m[0][j] +
                mtx->m[i][1] * rightMtx->m[1][j] +
                mtx->m[i][2] * rightMtx->m[2][j];
        }
    }

    gfxMatrixSet2(mtx, &tempMtx);
    GFX_FUNC_LEAVE;
}

void
gfxMatrixRotate (
    GFX_MATRIX  * mtx,
    float       degree)
{
    float       rad     = (degree * GFX_PI / 180.0f);
    float       cosA    = cos(rad);
    float       sinA    = sin(rad);
    GFX_MATRIX  rotateMtx;

    GFX_FUNC_ENTRY;
    gfxMatrixSet(&rotateMtx,
        cosA, -sinA, 0,
        sinA, cosA,  0,
        0,    0,     1);
    gfxMatrixMultiply(mtx, &rotateMtx);
    GFX_FUNC_LEAVE;
}

void
gfxMatrixTranslate (
    GFX_MATRIX  * mtx,
    int         offsetX,
    int         offsetY)
{
    GFX_MATRIX translateMtx;

    GFX_FUNC_ENTRY;
    gfxMatrixSet(&translateMtx,
        1, 0, offsetX,
        0, 1, offsetY,
        0, 0, 1);
    gfxMatrixMultiply(mtx, &translateMtx);
    GFX_FUNC_LEAVE;
}

void
gfxMatrixScale (
    GFX_MATRIX  * mtx,
    float       scaleX,
    float       scaleY)
{
    GFX_MATRIX scaleMtx;

    GFX_FUNC_ENTRY;
    gfxMatrixSet(&scaleMtx,
        scaleX, 0,      0,
        0,      scaleY, 0,
        0,      0,      1);
    gfxMatrixMultiply(mtx, &scaleMtx);
    GFX_FUNC_LEAVE;
}

bool
gfxMatrixInverse (
    GFX_MATRIX  * originMtx,
    GFX_MATRIX  * inverseMtx)
{
    bool    result;
    bool    affine;
    float   D0;
    float   D1;
    float   D2;
    float   D;

    result = true;

    GFX_FUNC_ENTRY;

    affine = gfxIsAffine(originMtx);

    /* Calculate determinant */

    D0  = originMtx->m[1][1] * originMtx->m[2][2] - originMtx->m[2][1] * originMtx->m[1][2];
    D1  = originMtx->m[2][0] * originMtx->m[1][2] - originMtx->m[1][0] * originMtx->m[2][2];
    D2  = originMtx->m[1][0] * originMtx->m[2][1] - originMtx->m[2][0] * originMtx->m[1][1];
    D   = originMtx->m[0][0] * D0 + originMtx->m[0][1] * D1 + originMtx->m[0][2] * D2;

    /* Check if singular */
    if (D == 0.0f)
    {
        return false;
    }
    D                   = 1.0f / D;

    /* Calculate inverse */
    inverseMtx->m[0][0] = D * D0;
    inverseMtx->m[1][0] = D * D1;
    inverseMtx->m[2][0] = D * D2;
    inverseMtx->m[0][1] = D * (originMtx->m[2][1] * originMtx->m[0][2] - originMtx->m[0][1] * originMtx->m[2][2]);
    inverseMtx->m[1][1] = D * (originMtx->m[0][0] * originMtx->m[2][2] - originMtx->m[2][0] * originMtx->m[0][2]);
    inverseMtx->m[2][1] = D * (originMtx->m[2][0] * originMtx->m[0][1] - originMtx->m[0][0] * originMtx->m[2][1]);
    inverseMtx->m[0][2] = D * (originMtx->m[0][1] * originMtx->m[1][2] - originMtx->m[1][1] * originMtx->m[0][2]);
    inverseMtx->m[1][2] = D * (originMtx->m[1][0] * originMtx->m[0][2] - originMtx->m[0][0] * originMtx->m[1][2]);
    inverseMtx->m[2][2] = D * (originMtx->m[0][0] * originMtx->m[1][1] - originMtx->m[1][0] * originMtx->m[0][1]);

    // affine matrix stays affine
    if (affine)
    {
        inverseMtx->m[0][2] = 0;
        inverseMtx->m[1][2] = 0;
        inverseMtx->m[2][2] = 1;
    }

    GFX_FUNC_LEAVE;
    return result;
}

void
gfxMatrixTransform (
    GFX_MATRIX  * mtx,
    int         srcX,
    int         srcY,
    float       * dstX,
    float       * dstY)
{
    GFX_FUNC_ENTRY;
    *dstX   = (srcX * mtx->m[0][0] + srcY * mtx->m[0][1] + mtx->m[0][2]) /
            (srcX * mtx->m[2][0] + srcY * mtx->m[2][1] + mtx->m[2][2]);
    *dstY   = (srcX * mtx->m[1][0] + srcY * mtx->m[1][1] + mtx->m[1][2]) /
            (srcX * mtx->m[2][0] + srcY * mtx->m[2][1] + mtx->m[2][2]);
    GFX_FUNC_LEAVE;
}

bool
gfxMatrixWarpQuadToSquare (
    GFX_RECTANGLE   s,
    GFX_MATRIX      * mtx,
    bool            bInverse)
{
    bool        result;
    bool        ret;
    GFX_MATRIX  Invmat;
    GFX_FUNC_ENTRY;

    result = true;

    if (!mtx)
    {
        result = false;
        goto exit;
    }

    ret = gfxMatrixWarpSquareToQuad(s, mtx, bInverse);
    if (ret == false)
    {
        result = false;
        goto exit;
    }

    if (!gfxMatrixInverse(mtx, &Invmat))
    {
        result = false;
        goto exit;
    }

    mtx->m[0][0]    = Invmat.m[0][0];
    mtx->m[1][0]    = Invmat.m[1][0];
    mtx->m[2][0]    = Invmat.m[2][0];
    mtx->m[0][1]    = Invmat.m[0][1];
    mtx->m[1][1]    = Invmat.m[1][1];
    mtx->m[2][1]    = Invmat.m[2][1];
    mtx->m[0][2]    = Invmat.m[0][2];
    mtx->m[1][2]    = Invmat.m[1][2];
    mtx->m[2][2]    = Invmat.m[2][2];

exit:
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxMatrixWarpSquareToQuad (
    GFX_RECTANGLE   d,
    GFX_MATRIX      * mtx,
    bool            bInverse)
{
    bool    result;
    float   diffx1, diffy1, diffx2 = d.x2 - d.x3, diffy2;
    float   det;
    float   sumx, sumy;
    float   oodet, g, h;
    GFX_FUNC_ENTRY;

    result = true;

    if (!mtx)
    {
        result = false;
        goto exit;
    }

    // from Heckbert:Fundamentals of Texture Mapping and Image Warping
    // Note that his mapping of vertices is different from OpenVG's
    // (0,0) => (dx0,dy0)
    // (1,0) => (dx1,dy1)
    // (0,1) => (dx2,dy2)
    // (1,1) => (dx3,dy3)

    diffx1  = d.x1 - d.x3;
    diffy1  = d.y1 - d.y3;
    diffx2  = d.x2 - d.x3;
    diffy2  = d.y2 - d.y3;

    det     = diffx1 * diffy2 - diffx2 * diffy1;
    if (det == 0.0f)
    {
        result = false;
        goto exit;
    }

    sumx    = d.x0 - d.x1 + d.x3 - d.x2;
    sumy    = d.y0 - d.y1 + d.y3 - d.y2;

    // (void)printf("sumx:%f,sumy:%f\n",sumx,sumy);
#if 1
    if ((sumx == 0.0f) && (sumy == 0.0f))
    {   // affine mapping
        mtx->m[0][0]    = d.x1 - d.x0;
        mtx->m[0][1]    = d.y1 - d.y0;
        mtx->m[0][2]    = 0.0f;
        mtx->m[1][0]    = d.x3 - d.x1;
        mtx->m[1][1]    = d.y3 - d.y1;
        mtx->m[1][2]    = 0.0f;
        mtx->m[2][0]    = d.x0;
        mtx->m[2][1]    = d.y0;
        mtx->m[2][2]    = 1.0f;

        if (bInverse)
        {
            mtx->m[2][0] = d.x1;
        }

        result = true;
        goto exit;
    }

    oodet           = 1.0f / det;
    g               = (sumx * diffy2 - diffx2 * sumy) * oodet;
    h               = (diffx1 * sumy - sumx * diffy1) * oodet;
    // (void)printf("g:%f h:%f oodet:%f det:%f\n",g,h,oodet,det);
    mtx->m[0][0]    = d.x1 - d.x0 + g * d.x1;
    mtx->m[0][1]    = d.y1 - d.y0 + g * d.y1;
    mtx->m[0][2]    = g;
    mtx->m[1][0]    = d.x2 - d.x0 + h * d.x2;
    mtx->m[1][1]    = d.y2 - d.y0 + h * d.y2;
    mtx->m[1][2]    = h;
    mtx->m[2][0]    = d.x0;
    mtx->m[2][1]    = d.y0;
    mtx->m[2][2]    = 1.0f;

    if (bInverse)
    {
        mtx->m[0][0]    = d.x0 - d.x1 + g * d.x0;
        mtx->m[1][0]    = d.x3 - d.x1 + h * d.x3;
        mtx->m[2][0]    = d.x1;
    }
#else
    if ((sumx == 0.0f) && (sumy == 0.0f))
    {   // affine mapping
        mtx->m[0][0]    = d.x1 - d.x0;
        mtx->m[0][1]    = d.y1 - d.y0;
        mtx->m[0][2]    = d.x0;
        mtx->m[1][0]    = d.x3 - d.x1;
        mtx->m[1][1]    = d.y3 - d.y1;
        mtx->m[1][2]    = d.y0;
        mtx->m[2][0]    = 0.0f;
        mtx->m[2][1]    = 0.0f;
        mtx->m[2][2]    = 1.0f;
        result          = true;
        goto exit;
    }

    oodet           = 1.0f / det;
    g               = (sumx * diffy2 - diffx2 * sumy) * oodet;
    h               = (diffx1 * sumy - sumx * diffy1) * oodet;

    mtx->m[0][0]    = d.x1 - d.x0 + g * d.x1;
    mtx->m[0][1]    = d.y1 - d.y0 + g * d.y1;
    mtx->m[0][2]    = d.x0;
    mtx->m[1][0]    = d.x2 - d.x0 + h * d.x2;
    mtx->m[1][1]    = d.y2 - d.y0 + h * d.y2;
    mtx->m[1][2]    = d.y0;
    mtx->m[2][0]    = g;
    mtx->m[2][1]    = h;
    mtx->m[2][2]    = 1.0f;
#endif
exit:
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxMatrixWarpQuadToQuad (
    GFX_RECTANGLE   d,
    GFX_RECTANGLE   s,
    GFX_MATRIX      * mtx,
    bool            bInverse)
{
    bool        result;
    GFX_MATRIX  mtx1;
    GFX_MATRIX  mtx2;
    GFX_FUNC_ENTRY;

    
    result  = gfxMatrixWarpQuadToSquare(s, &mtx1, bInverse);
    if (result == false)
    {
        goto exit;
    }

    result = gfxMatrixWarpSquareToQuad(d, &mtx2, bInverse);
    if (result == false)
    {
        goto exit;
    }

    gfxMatrixSet2(mtx, &mtx2);
    gfxMatrixMultiply(mtx, &mtx1);


exit:
    GFX_FUNC_LEAVE;

    return true;
}

bool
gfxIsAffine (
    GFX_MATRIX * mtx)
{
    return floatIsZero(mtx->m[0][2]) && floatIsZero(mtx->m[1][2])
           && floatsEqual(mtx->m[2][2], 1);
}

#ifdef CFG_M2D_HUD_ENABLE

bool
gfxGetCoordinate (
    char        * srcfilepath,
    char        * dstfilepath,
    uint16_t    * dstBuf,
    GFXHUDRotateType rotType)
{
    bool    result;
    int     i, j, n = 0;
    int     num         = 0;

    FILE    * srcFile   = NULL;
    FILE    * dstFile   = NULL;
    int     fileSize    = 0;
    float   * src;
    float   * dst;
    float   mesh_std[gW_num + 1][gH_num + 1][2];

    result = true;

    fileSize = (gW_num + 1) * (gH_num + 1) * 2;

    if (pointSrc)
    {
        free(pointSrc);
    }
    pointSrc = malloc(gBlock_num * sizeof(GFX_RECTANGLE));

    if (pointDst)
    {
        free(pointDst);
    }
    pointDst = malloc(gBlock_num * sizeof(GFX_RECTANGLE));


    if (srcfilepath)
    {
        srcFile = fopen(srcfilepath, "rb");
        if (srcFile)
        {
            src = (float *)malloc(fileSize * sizeof(float));
            for (i = 0; i < fileSize; i++)
            {
                fscanf(srcFile, "%f", &src[i]);
            }
            memcpy(mesh_std, src, fileSize * sizeof(float));

            fclose(srcFile);
            free(src);
        }
        else
        {
            (void)printf("ERROR NOT EXIST srcfilepath\n");
            return false;
        }

        (void)printf("mesh_std[i][j][0]:(%f %f) (%f %f)\n", mesh_std[1][0][0], mesh_std[1][0][1], mesh_std[0][1][0], mesh_std[0][1][1]);

    #ifdef HUD_HORIZONTAL
        for (j = 0; j < gH_num; j++)
        {
            for (i = 0; i < gW_num; i++)
    #else
        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
    #endif
            {
    #ifdef HUD_HORIZONTAL
                num                 = i + (j * gW_num);
    #else
                num                 = j + (i * gH_num);
    #endif
                pointSrc[num].x0    = mesh_std[i][j][0];
                pointSrc[num].y0    = mesh_std[i][j][1];

                pointSrc[num].x1    = mesh_std[i][j + 1][0];
                pointSrc[num].y1    = mesh_std[i][j + 1][1];

                pointSrc[num].x3    = mesh_std[i + 1][j][0];
                pointSrc[num].y3    = mesh_std[i + 1][j][1];

                pointSrc[num].x2    = mesh_std[i + 1][j + 1][0];
                pointSrc[num].y2    = mesh_std[i + 1][j + 1][1];
            }
        }
    }
    else
    {
        // auto generate src coordinates
    #ifdef HUD_HORIZONTAL
        for (j = 0; j < gH_num; j++)
        {
            for (i = 0; i < gW_num; i++)
    #else
        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
    #endif
            {
    #ifdef HUD_HORIZONTAL
                num                 = i + (j * gW_num);
    #else
                num                 = j + (i * gH_num);
    #endif
                pointSrc[num].x0    = gBlock_W * i;
                pointSrc[num].y0    = gBlock_H * j;

                pointSrc[num].x1    = gBlock_W * i;
                pointSrc[num].y1    = gBlock_H * (j + 1);

                pointSrc[num].x3    = gBlock_W * (i + 1);
                pointSrc[num].y3    = gBlock_H * j;

                pointSrc[num].x2    = gBlock_W * (i + 1);
                pointSrc[num].y2    = gBlock_H * (j + 1);
            }
        }
        (void)printf("pointSrc[0]:(%f %f) (%f %f)\n", pointSrc[0].x3, pointSrc[0].y3, pointSrc[0].x1, pointSrc[0].y1);
    }

    if (dstBuf)
    {
        memcpy(mesh_std, dstBuf, fileSize * sizeof(float));
    }
    else if (dstfilepath)
    {
        dstFile = fopen(dstfilepath, "rb");

        if (dstFile)
        {
            dst = (float *)malloc(fileSize * sizeof(float));
            for (i = 0; i < fileSize; i++)
            {
                fscanf(dstFile, "%f", &dst[i]);
            }
            memcpy(mesh_std, dst, fileSize * sizeof(float));
            fclose(dstFile);
            free(dst);
        }
        else
        {
            memset(mesh_std, 0, fileSize * sizeof(float));
            (void)printf("ERROR NOT EXIST dstfilepath\n");
            result = false;
        }
    }
    else
    {
        memset(mesh_std, 0, fileSize * sizeof(float));
        (void)printf("ERROR NOT EXIST dst file\n");
        result = false;
    }

    (void)printf("mesh_dst[i][j][0]:(%f %f) (%f %f)\n", mesh_std[1][0][0], mesh_std[1][0][1], mesh_std[0][1][0], mesh_std[0][1][1]);

    #ifdef HUD_HORIZONTAL
    for (j = 0; j < gH_num; j++)
    {
        for (i = 0; i < gW_num; i++)
    #else
    for (i = 0; i < gW_num; i++)
    {
        for (j = 0; j < gH_num; j++)
    #endif
        {
    #ifdef HUD_HORIZONTAL
            num                 = i + (j * gW_num);
    #else
            num                 = j + (i * gH_num);
    #endif

            if (isnan(mesh_std[i][j][0]) || isnan(mesh_std[i][j][1]) 
                || (mesh_std[i][j][0] > (float)(gBlock_W * 5)) || (mesh_std[i][j][1] > (float)(gBlock_H * 5))
                || (mesh_std[i][j][0] < -(float)(gBlock_W * 5)) || (mesh_std[i][j][1] < -(float)(gBlock_H * 5)))
            {
                (void)printf("dst value is invalid (%f %f)\n", mesh_std[i][j][0], mesh_std[i][j][1]);
                result = false;
                break;
            }

            pointDst[num].x0    = mesh_std[i][j][0] + 0;
            pointDst[num].y0    = mesh_std[i][j][1] + 0;

            pointDst[num].x1    = mesh_std[i][j + 1][0] + 0;
            pointDst[num].y1    = mesh_std[i][j + 1][1] + gBlock_H;

            pointDst[num].x3    = mesh_std[i + 1][j][0] + gBlock_W;
            pointDst[num].y3    = mesh_std[i + 1][j][1] + 0;

            pointDst[num].x2    = mesh_std[i + 1][j + 1][0] + gBlock_W;
            pointDst[num].y2    = mesh_std[i + 1][j + 1][1] + gBlock_H;
        }
    }

// #ifndef HUD_ADVANCE_ROTATE_90
// #ifndef HUD_HORIZONTAL
//    for (i = 0; i < gW_num; i++)
//    {
//        for (j = NUMBER_BLOCK + 1; j < gH_num; j++)
//        {
//            {
//                pointDst[i*gH_num + j].y0 = pointDst[i*gH_num + NUMBER_BLOCK].y1 - gBlock_H;
//                pointDst[i*gH_num + j].y1 = pointDst[i*gH_num + NUMBER_BLOCK].y1;
//
//                pointDst[i*gH_num + j].y3 = pointDst[i*gH_num + NUMBER_BLOCK].y2 - gBlock_H;
//                pointDst[i*gH_num + j].y2 = pointDst[i*gH_num + NUMBER_BLOCK].y2;
//            }
//        }
//    }
// #endif
// #endif

    float dx1, dx2, dx3, dy1, dy2, dy3;
    float denominator;

    float u0, u1, u2, u3, v0, v1, v2, v3;
    float x0, x1, x2, x3, y0, y1, y2, y3;

    u0  = 0;
    u1  = 0;
    u2  = gBlock_W;
    u3  = gBlock_W;
    v0  = 0;
    v1  = gBlock_H;
    v2  = gBlock_H;
    v3  = 0;

    for (num = 0; num < gBlock_num; num++)
    {
        GFX_MATRIX  mtx;
        GFX_MATRIX  mtx1;
        GFX_MATRIX  inverseMtx;
        int32_t     dx;
        int32_t     dy;

    #ifndef HUD_ADVANCE_ROTATE_90
        switch (rotType)
        {
        case GFX_HUD_ROT90:
            dx = -(int32_t)pointSrc[num].y0 + (gBlock_H * gH_num);
            dy = (int32_t)pointSrc[num].x0;
            break;
        case GFX_HUD_ROT180:
            dx = -(int32_t)pointSrc[num].x0 + (gBlock_W * gW_num);
            dy = -(int32_t)pointSrc[num].y0 + (gBlock_H * gH_num);
            break;
        case GFX_HUD_ROT270:
            dx = (int32_t)pointSrc[num].y0;
            dy = -(int32_t)pointSrc[num].x0 + (gBlock_W * gW_num);
            break;
        case GFX_HUD_MIRROR:
            dx = -(int32_t)pointSrc[num].x0 + (gBlock_W * gW_num);
            dy = (int32_t)pointSrc[num].y0;
            break;
        case GFX_HUD_FLIP:
            dx = (int32_t)pointSrc[num].x0;
            dy = -(int32_t)pointSrc[num].y0 + (gBlock_H * gH_num);
            break;
        default: 
            dx = (int32_t)pointSrc[num].x0;
            dy = (int32_t)pointSrc[num].y0;
            break;
        }   
    #else
        dx  = -(int32_t)pointSrc[num].y0 + (gBlock_H * gH_num); // 576
        dy  = (int32_t)pointSrc[num].x0;
    #endif

        if (result)
        {
            x0 = pointDst[num].x0;
            x1 = pointDst[num].x1;
            x2 = pointDst[num].x2;
            x3 = pointDst[num].x3;
            y0 = pointDst[num].y0;
            y1 = pointDst[num].y1;
            y2 = pointDst[num].y2;
            y3 = pointDst[num].y3;
        }
        else
        {
            x0 = 0.0f;
            x1 = 0.0f;
            x2 = (float)gBlock_W;
            x3 = (float)gBlock_W;
            y0 = 0.0f;
            y1 = (float)gBlock_H;
            y2 = (float)gBlock_H;
            y3 = 0.0f;
        }

        dx1                         = x3 - x2;
        dx2                         = x1 - x2;
        dx3                         = x0 - x3 + x2 - x1;
        dy1                         = y3 - y2;
        dy2                         = y1 - y2;
        dy3                         = y0 - y3 + y2 - y1;

        denominator                 = dx1 * dy2 - dx2 * dy1;

        transform_m[num].m[2][0]    = (dx3 * dy2 - dx2 * dy3) / u3 / denominator;
        transform_m[num].m[2][1]    = (dx1 * dy3 - dx3 * dy1) / v1 / denominator;
        transform_m[num].m[0][0]    = (x3 - x0) / u3 + (transform_m[num].m[2][0] * x3);
        transform_m[num].m[0][1]    = (x1 - x0) / v1 + (transform_m[num].m[2][1] * x1);// (x1 - x0)/v1 + (m[1][2] * x2);
        transform_m[num].m[0][2]    = x0;
        transform_m[num].m[1][2]    = y0;
        transform_m[num].m[1][0]    = (y3 - y0) / u3 + (transform_m[num].m[2][0] * y3);
        transform_m[num].m[1][1]    = (y1 - y0) / v1 + (transform_m[num].m[2][1] * y1);
        transform_m[num].m[2][2]    = 1;


    #ifdef HUD_ADVANCE_ROTATE_90
        // rotate 90
        mtx1.m[0][0] = 0.0f;
        mtx1.m[0][1] = -1.0f;
        mtx1.m[0][2] = 0.0f;
        mtx1.m[1][0] = 1.0f;
        mtx1.m[1][1] = 0.0f;
        mtx1.m[1][2] = 0.0f;
        mtx1.m[2][0] = 0.0f;
        mtx1.m[2][1] = 0.0f;
        mtx1.m[2][2] = 1.0f;

        gfxMatrixMultiply(&mtx1, &transform_m[num]);
    #else
        if (rotType == GFX_HUD_ROT90)
        {
            mtx1.m[0][0] = 0.0f;
            mtx1.m[0][1] = -1.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = 1.0f;
            mtx1.m[1][1] = 0.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
        }
        else if (rotType == GFX_HUD_ROT180)
        {
            mtx1.m[0][0] = -1.0f;
            mtx1.m[0][1] = 0.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = 0.0f;
            mtx1.m[1][1] = -1.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
        }
        else if (rotType == GFX_HUD_ROT270)
        {
            mtx1.m[0][0] = 0.0f;
            mtx1.m[0][1] = 1.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = -1.0f;
            mtx1.m[1][1] = 0.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
        }
        else if (rotType == GFX_HUD_MIRROR)
        {
            mtx1.m[0][0] = -1.0f;
            mtx1.m[0][1] = 0.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = 0.0f;
            mtx1.m[1][1] = 1.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
        }
        else if (rotType == GFX_HUD_FLIP)
        {
            mtx1.m[0][0] = 1.0f;
            mtx1.m[0][1] = 0.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = 0.0f;
            mtx1.m[1][1] = -1.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
        }
        else
        {
            /* do nothing */
        }
    #endif

        mtx.m[0][0] = 1.0f;
        mtx.m[0][1] = 0.0f;
        mtx.m[0][2] = dx;
        mtx.m[1][0] = 0.0f;
        mtx.m[1][1] = 1.0f;
        mtx.m[1][2] = dy;
        mtx.m[2][0] = 0.0f;
        mtx.m[2][1] = 0.0f;
        mtx.m[2][2] = 1.0f;

    #ifdef HUD_ADVANCE_ROTATE_90
        gfxMatrixMultiply(&mtx, &mtx1);
    #else
        if (rotType != GFX_HUD_ROT0)
            gfxMatrixMultiply(&mtx, &mtx1);
        else
            gfxMatrixMultiply(&mtx, &transform_m[num]);
    #endif
        if (1)
        {
            float   topLX, topLY, topRX, topRY;
            float   botLX, botLY, botRX, botRY;
            int32_t topX;
            int32_t topY;
            int32_t botX;
            int32_t botY;

            int32_t boundaryW;
            int32_t boundaryH;

            // Find the top left and bottom right coordinate
            {
                topLX   = (mtx.m[0][2]) / (mtx.m[2][2]);
                topLY   = (mtx.m[1][2]) / (mtx.m[2][2]);

                topRX   = (gBlock_W * mtx.m[0][0] + mtx.m[0][2]) /
                    (gBlock_W * mtx.m[2][0] + mtx.m[2][2]);
                topRY   = (gBlock_W * mtx.m[1][0] + mtx.m[1][2]) /
                    (gBlock_W * mtx.m[2][0] + mtx.m[2][2]);

                botLX   = (gBlock_H * mtx.m[0][1] + mtx.m[0][2]) /
                    (gBlock_H * mtx.m[2][1] + mtx.m[2][2]);
                botLY   = (gBlock_H * mtx.m[1][1] + mtx.m[1][2]) /
                    (gBlock_H * mtx.m[2][1] + mtx.m[2][2]);

                botRX   = (gBlock_W * mtx.m[0][0] + gBlock_H * mtx.m[0][1] + mtx.m[0][2]) /
                    (gBlock_W * mtx.m[2][0] + gBlock_H * mtx.m[2][1] + mtx.m[2][2]);
                botRY   = (gBlock_W * mtx.m[1][0] + gBlock_H * mtx.m[1][1] + mtx.m[1][2]) /
                    (gBlock_W * mtx.m[2][0] + gBlock_H * mtx.m[2][1] + mtx.m[2][2]);

                topX    = (int32_t)floorf(MIN(MIN(topLX, topRX), MIN(botLX, botRX))) - 1;
                topY    = (int32_t)floorf(MIN(MIN(topLY, topRY), MIN(botLY, botRY))) - 1;
                botX    = (int32_t)ceilf(MAX(MAX(topLX, topRX), MAX(botLX, botRX))) + 1;
                botY    = (int32_t)ceilf(MAX(MAX(topLY, topRY), MAX(botLY, botRY))) + 1;

                // (void)printf("topL (%f, %f), topR (%f, %f)\n", topLX, topLY, topRX, topRY);
                // (void)printf("botL (%f, %f), botR (%f, %f)\n", botLX, botLY, botRX, botRY);
                // (void)printf("top (%d, %d), bot (%d, %d)\n", topX, topY, botX, botY);
            }

            {
    #ifdef HUD_ADVANCE_ROTATE_90
                boundaryW = gBlock_H * gH_num;
                boundaryH = gBlock_W * gW_num;
    #else
                if (rotType == GFX_HUD_ROT90 || rotType == GFX_HUD_ROT270)
                {
                    boundaryW = gBlock_H * gH_num;
                    boundaryH = gBlock_W * gW_num;
                }
                else
                {
                    boundaryW = gBlock_W * gW_num;
                    boundaryH = gBlock_H * gH_num;
                }
    #endif
                //if ((topX < 0) || (topY < 0) || (botX >= boundaryW) || (botY >= boundaryH))
                //{
                //    (void)printf("Out of boundary (%d %d) top (%d, %d), bot (%d, %d)\n", dx, dy, topX, topY, botX, botY);
                //}
                topX = (topX < 0) ? 0 : topX;
                topY = (topY < 0) ? 0 : topY;
                topX = topX >= boundaryW ? boundaryW : topX;
                topY = topY >= boundaryH ? boundaryH : topY;
                botX = (botX < 0) ? 0 : botX;
                botY = (botY < 0) ? 0 : botY;
                botX = botX >= boundaryW ? boundaryW - 1 : botX;
                botY = botY >= boundaryH ? boundaryH - 1 : botY;
            }

    #ifdef HUD_ADVANCE_ROTATE_90
            // rotate 90
            mtx1.m[0][0] = 0.0f;
            mtx1.m[0][1] = -1.0f;
            mtx1.m[0][2] = 0.0f;
            mtx1.m[1][0] = 1.0f;
            mtx1.m[1][1] = 0.0f;
            mtx1.m[1][2] = 0.0f;
            mtx1.m[2][0] = 0.0f;
            mtx1.m[2][1] = 0.0f;
            mtx1.m[2][2] = 1.0f;

            gfxMatrixMultiply(&mtx1, &transform_m[num]);
    #else
            if (rotType == GFX_HUD_ROT90)
            {
                mtx1.m[0][0] = 0.0f;
                mtx1.m[0][1] = -1.0f;
                mtx1.m[0][2] = 0.0f;
                mtx1.m[1][0] = 1.0f;
                mtx1.m[1][1] = 0.0f;
                mtx1.m[1][2] = 0.0f;
                mtx1.m[2][0] = 0.0f;
                mtx1.m[2][1] = 0.0f;
                mtx1.m[2][2] = 1.0f;

                gfxMatrixMultiply(&mtx1, &transform_m[num]);
            }
            else if (rotType == GFX_HUD_ROT180)
            {
                mtx1.m[0][0] = -1.0f;
                mtx1.m[0][1] = 0.0f;
                mtx1.m[0][2] = 0.0f;
                mtx1.m[1][0] = 0.0f;
                mtx1.m[1][1] = -1.0f;
                mtx1.m[1][2] = 0.0f;
                mtx1.m[2][0] = 0.0f;
                mtx1.m[2][1] = 0.0f;
                mtx1.m[2][2] = 1.0f;

                gfxMatrixMultiply(&mtx1, &transform_m[num]);
            }
            else if (rotType == GFX_HUD_ROT270)
            {
                mtx1.m[0][0] = 0.0f;
                mtx1.m[0][1] = 1.0f;
                mtx1.m[0][2] = 0.0f;
                mtx1.m[1][0] = -1.0f;
                mtx1.m[1][1] = 0.0f;
                mtx1.m[1][2] = 0.0f;
                mtx1.m[2][0] = 0.0f;
                mtx1.m[2][1] = 0.0f;
                mtx1.m[2][2] = 1.0f;

                gfxMatrixMultiply(&mtx1, &transform_m[num]);
            }
            else if (rotType == GFX_HUD_MIRROR)
            {
                mtx1.m[0][0] = -1.0f;
                mtx1.m[0][1] = 0.0f;
                mtx1.m[0][2] = 0.0f;
                mtx1.m[1][0] = 0.0f;
                mtx1.m[1][1] = 1.0f;
                mtx1.m[1][2] = 0.0f;
                mtx1.m[2][0] = 0.0f;
                mtx1.m[2][1] = 0.0f;
                mtx1.m[2][2] = 1.0f;

                gfxMatrixMultiply(&mtx1, &transform_m[num]);
            }
            else if (rotType == GFX_HUD_FLIP)
            {
                mtx1.m[0][0] = 1.0f;
                mtx1.m[0][1] = 0.0f;
                mtx1.m[0][2] = 0.0f;
                mtx1.m[1][0] = 0.0f;
                mtx1.m[1][1] = -1.0f;
                mtx1.m[1][2] = 0.0f;
                mtx1.m[2][0] = 0.0f;
                mtx1.m[2][1] = 0.0f;
                mtx1.m[2][2] = 1.0f;

                gfxMatrixMultiply(&mtx1, &transform_m[num]);
            }
            else
            {
                /* do nothing */
            }
    #endif


            mtx.m[0][0] = 1.0f;
            mtx.m[0][1] = 0.0f;
            mtx.m[0][2] = dx - topX;
            mtx.m[1][0] = 0.0f;
            mtx.m[1][1] = 1.0f;
            mtx.m[1][2] = dy - topY;
            mtx.m[2][0] = 0.0f;
            mtx.m[2][1] = 0.0f;
            mtx.m[2][2] = 1.0f;


    #ifdef HUD_ADVANCE_ROTATE_90
            gfxMatrixMultiply(&mtx, &mtx1);
    #else
            if (rotType != GFX_HUD_ROT0)
                gfxMatrixMultiply(&mtx, &mtx1);
            else
                gfxMatrixMultiply(&mtx, &transform_m[num]);
    #endif
            gfxMatrixInverse(&mtx, &inverseMtx);

            transform_m[num].m[2][0]    = inverseMtx.m[2][0];
            transform_m[num].m[2][1]    = inverseMtx.m[2][1];
            transform_m[num].m[0][0]    = inverseMtx.m[0][0];
            transform_m[num].m[0][1]    = inverseMtx.m[0][1];
            transform_m[num].m[0][2]    = inverseMtx.m[0][2];
            transform_m[num].m[1][2]    = inverseMtx.m[1][2];
            transform_m[num].m[1][0]    = inverseMtx.m[1][0];
            transform_m[num].m[1][1]    = inverseMtx.m[1][1];
            transform_m[num].m[2][2]    = inverseMtx.m[2][2];

            dstInfo[num].new_dstX       = topX;
            dstInfo[num].new_dstY       = topY;
            dstInfo[num].new_dstWidth   = botX - topX + 1;
            dstInfo[num].new_dstHeight  = botY - topY + 1;
        }
    }

    return result;
}

// bool
// gfxGetFishCoordinate(
//    void)
// {
//    int i, j, n = 0;
//    int num = 0;
//
//    if (pointSrc)
//        free(pointSrc);
//    pointSrc = malloc(gBlock_num * sizeof(GFX_RECTANGLE));
//    if (pointDst)
//        free(pointDst);
//    pointDst = malloc(gBlock_num * sizeof(GFX_RECTANGLE));
//    if (pointDstFish)
//        free(pointDstFish);
//    pointDstFish = malloc(gBlock_num * sizeof(GFX_RECTANGLE));
//
//    for (j = 0; j < gH_num; j++)
//    {
//        for (i = 0; i < gW_num; i++)
//        {
//            num = i + (j * gW_num);
//
//            pointSrc[num].x0 = fish_mesh_std[i][j][0];
//            pointSrc[num].y0 = fish_mesh_std[i][j][1];
//
//            pointSrc[num].x1 = fish_mesh_std[i + 1][j][0];
//            pointSrc[num].y1 = fish_mesh_std[i + 1][j][1];
//
//            pointSrc[num].x3 = fish_mesh_std[i][j + 1][0];
//            pointSrc[num].y3 = fish_mesh_std[i][j + 1][1];
//
//            pointSrc[num].x2 = fish_mesh_std[i + 1][j + 1][0];
//            pointSrc[num].y2 = fish_mesh_std[i + 1][j + 1][1];
//        }
//    }
//    num = 0;
//    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointSrc[num].x0, pointSrc[num].y0, pointSrc[num].x1, pointSrc[num].y1, pointSrc[num].x2, pointSrc[num].y2, pointSrc[num].x3, pointSrc[num].y3);
//    num = 1;
//    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointSrc[num].x0, pointSrc[num].y0, pointSrc[num].x1, pointSrc[num].y1, pointSrc[num].x2, pointSrc[num].y2, pointSrc[num].x3, pointSrc[num].y3);
//
//    for (j = 0; j < gH_num; j++)
//    {
//        for (i = 0; i < gW_num; i++)
//        {
//            num = i + (j * gW_num);
//
//            pointDst[num].x0 = fish_mesh_new2[i][j][0] - fish_mesh_std[i][j][0];
//            pointDst[num].y0 = fish_mesh_new2[i][j][1] - fish_mesh_std[i][j][1];
//
//            pointDst[num].x1 = fish_mesh_new2[i + 1][j][0] - fish_mesh_std[i + 1][j][0];
//            pointDst[num].y1 = fish_mesh_new2[i + 1][j][1] - fish_mesh_std[i + 1][j][1] + gBlock_H;
//
//            pointDst[num].x3 = fish_mesh_new2[i][j + 1][0] - fish_mesh_std[i][j + 1][0] + gBlock_W;
//            pointDst[num].y3 = fish_mesh_new2[i][j + 1][1] - fish_mesh_std[i][j + 1][1];
//
//            pointDst[num].x2 = fish_mesh_new2[i + 1][j + 1][0] - fish_mesh_std[i + 1][j + 1][0] + gBlock_W;
//            pointDst[num].y2 = fish_mesh_new2[i + 1][j + 1][1] - fish_mesh_std[i + 1][j + 1][1] + gBlock_H;
//
//
//            pointDstFish[num].x0 = fish_mesh_new2[i][j][0];
//            pointDstFish[num].y0 = fish_mesh_new2[i][j][1];
//
//            pointDstFish[num].x1 = fish_mesh_new2[i + 1][j][0];
//            pointDstFish[num].y1 = fish_mesh_new2[i + 1][j][1];
//
//            pointDstFish[num].x3 = fish_mesh_new2[i][j + 1][0];
//            pointDstFish[num].y3 = fish_mesh_new2[i][j + 1][1];
//
//            pointDstFish[num].x2 = fish_mesh_new2[i + 1][j + 1][0];
//            pointDstFish[num].y2 = fish_mesh_new2[i + 1][j + 1][1];
//        }
//    }
//
//    //num = 0;
//    //(void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDst[num].x0, pointDst[num].y0, pointDst[num].x1, pointDst[num].y1, pointDst[num].x2, pointDst[num].y2, pointDst[num].x3, pointDst[num].y3);
//    //num = 1;
//    //(void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDst[num].x0, pointDst[num].y0, pointDst[num].x1, pointDst[num].y1, pointDst[num].x2, pointDst[num].y2, pointDst[num].x3, pointDst[num].y3);
//    //num = 2;
//    //(void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDst[num].x0, pointDst[num].y0, pointDst[num].x1, pointDst[num].y1, pointDst[num].x2, pointDst[num].y2, pointDst[num].x3, pointDst[num].y3);
//    //num = 3;
//    //(void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDst[num].x0, pointDst[num].y0, pointDst[num].x1, pointDst[num].y1, pointDst[num].x2, pointDst[num].y2, pointDst[num].x3, pointDst[num].y3);
//
//    num = 0;
//    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDstFish[num].x0, pointDstFish[num].y0, pointDstFish[num].x1, pointDstFish[num].y1, pointDstFish[num].x2, pointDstFish[num].y2, pointDstFish[num].x3, pointDstFish[num].y3);
//    num = 1;
//    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, pointDstFish[num].x0, pointDstFish[num].y0, pointDstFish[num].x1, pointDstFish[num].y1, pointDstFish[num].x2, pointDstFish[num].y2, pointDstFish[num].x3, pointDstFish[num].y3);
//
//    float dx1, dx2, dx3, dy1, dy2, dy3;
//    float denominator;
//
//    float u0, u1, u2, u3, v0, v1, v2, v3;
//    float x0, x1, x2, x3, y0, y1, y2, y3;
//
//    u0 = 0; u1 = 0;        u2 = gBlock_W; u3 = gBlock_W;
//    v0 = 0; v1 = gBlock_H; v2 = gBlock_H; v3 = 0;
//
//    for (num = 0; num < gBlock_num; num++)
//    {
//        GFX_MATRIX  mtx;
//        GFX_MATRIX  mtx1;
//        GFX_MATRIX  inverseMtx;
//        int32_t dx;
//        int32_t dy;
//
//        dx = (int32_t)pointSrc[num].x0;
//        dy = (int32_t)pointSrc[num].y0;
//
//        x0 = pointDst[num].x0; x1 = pointDst[num].x1; x2 = pointDst[num].x2; x3 = pointDst[num].x3;
//        y0 = pointDst[num].y0; y1 = pointDst[num].y1; y2 = pointDst[num].y2; y3 = pointDst[num].y3;
//
//
//        dx1 = x3 - x2; dx2 = x1 - x2; dx3 = x0 - x3 + x2 - x1;
//        dy1 = y3 - y2; dy2 = y1 - y2; dy3 = y0 - y3 + y2 - y1;
//
//        denominator = dx1 * dy2 - dx2 * dy1;
//
//        transform_m[num].m[2][0] = (dx3 * dy2 - dx2 * dy3) / u3 / denominator;
//        transform_m[num].m[2][1] = (dx1 * dy3 - dx3 * dy1) / v1 / denominator;
//        transform_m[num].m[0][0] = (x3 - x0) / u3 + (transform_m[num].m[2][0] * x3);
//        transform_m[num].m[0][1] = (x1 - x0) / v1 + (transform_m[num].m[2][1] * x1);
//        transform_m[num].m[0][2] = x0;
//        transform_m[num].m[1][2] = y0;
//        transform_m[num].m[1][0] = (y3 - y0) / u3 + (transform_m[num].m[2][0] * y3);
//        transform_m[num].m[1][1] = (y1 - y0) / v1 + (transform_m[num].m[2][1] * y1);
//        transform_m[num].m[2][2] = 1;
//
//        //gfxMatrixInverse(&transform_m[num], &inverseMtx);
//
//        //transform_m[num].m[2][0] = inverseMtx.m[2][0];
//        //transform_m[num].m[2][1] = inverseMtx.m[2][1];
//        //transform_m[num].m[0][0] = inverseMtx.m[0][0];
//        //transform_m[num].m[0][1] = inverseMtx.m[0][1];
//        //transform_m[num].m[0][2] = inverseMtx.m[0][2];
//        //transform_m[num].m[1][2] = inverseMtx.m[1][2];
//        //transform_m[num].m[1][0] = inverseMtx.m[1][0];
//        //transform_m[num].m[1][1] = inverseMtx.m[1][1];
//        //transform_m[num].m[2][2] = inverseMtx.m[2][2];
//
//        dstInfo[num].new_dstX = dx;
//        dstInfo[num].new_dstY = dy;
//        dstInfo[num].new_dstWidth = gBlock_W;
//        dstInfo[num].new_dstHeight = gBlock_H;
//
//        {
//            float   topLX, topLY, topRX, topRY;
//            float   botLX, botLY, botRX, botRY;
//            int32_t topX;
//            int32_t topY;
//            int32_t botX;
//            int32_t botY;
//
//            topLX = pointDstFish[num].x0;
//            topLY = pointDstFish[num].y0;
//
//            botLX = pointDstFish[num].x1;
//            botLY = pointDstFish[num].y1;
//
//            botRX = pointDstFish[num].x2;
//            botRY = pointDstFish[num].y2;
//
//            topRX = pointDstFish[num].x3;
//            topRY = pointDstFish[num].y3;
//
//            //if(num == 0)
//            //    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, topLX, topLY, botLX, botLY, botRX, botRY, topRX, topRY);
//            //if (num == 1)
//            //    (void)printf("m[%d](%f %f) (%f %f) (%f %f) (%f %f)\n", num, topLX, topLY, botLX, botLY, botRX, botRY, topRX, topRY);
//
//            topX = (int32_t)floorf(MIN(MIN(topLX, topRX), MIN(botLX, botRX))) - 1;
//            topY = (int32_t)floorf(MIN(MIN(topLY, topRY), MIN(botLY, botRY))) - 1;
//            botX = (int32_t)ceilf(MAX(MAX(topLX, topRX), MAX(botLX, botRX))) + 1;
//            botY = (int32_t)ceilf(MAX(MAX(topLY, topRY), MAX(botLY, botRY))) + 1;
//
//            //if (num == 0 || num == 1)
//            //    (void)printf("m[%d] top (%d, %d), bot (%d, %d)\n",num, topX, topY, botX, botY);
//
//            srcInfo[num].new_dstX = topX;
//            srcInfo[num].new_dstY = topY;
//            srcInfo[num].new_dstWidth = botX - topX + 1;
//            srcInfo[num].new_dstHeight = botY - topY + 1;
//        }
// #if 1
//        mtx.m[0][0] = 1;   mtx.m[0][1] = 0;   mtx.m[0][2] = dx;
//        mtx.m[1][0] = 0;   mtx.m[1][1] = 1;   mtx.m[1][2] = dy;
//        mtx.m[2][0] = 0;   mtx.m[2][1] = 0;   mtx.m[2][2] = 1;
//
//        gfxMatrixMultiply(&mtx, &transform_m[num]);
//
//        if (1)
//        {
//            float   topLX, topLY, topRX, topRY;
//            float   botLX, botLY, botRX, botRY;
//            int32_t topX;
//            int32_t topY;
//            int32_t botX;
//            int32_t botY;
//
//            // Find the top left and bottom right coordinate
//            {
//                topLX = (mtx.m[0][2]) / (mtx.m[2][2]);
//                topLY = (mtx.m[1][2]) / (mtx.m[2][2]);
//
//                topRX = (gBlock_W*mtx.m[0][0] + mtx.m[0][2]) /
//                    (gBlock_W*mtx.m[2][0] + mtx.m[2][2]);
//                topRY = (gBlock_W*mtx.m[1][0] + mtx.m[1][2]) /
//                    (gBlock_W*mtx.m[2][0] + mtx.m[2][2]);
//
//                botLX = (gBlock_H*mtx.m[0][1] + mtx.m[0][2]) /
//                    (gBlock_H*mtx.m[2][1] + mtx.m[2][2]);
//                botLY = (gBlock_H*mtx.m[1][1] + mtx.m[1][2]) /
//                    (gBlock_H*mtx.m[2][1] + mtx.m[2][2]);
//
//                botRX = (gBlock_W*mtx.m[0][0] + gBlock_H*mtx.m[0][1] + mtx.m[0][2]) /
//                    (gBlock_W*mtx.m[2][0] + gBlock_H*mtx.m[2][1] + mtx.m[2][2]);
//                botRY = (gBlock_W*mtx.m[1][0] + gBlock_H*mtx.m[1][1] + mtx.m[1][2]) /
//                    (gBlock_W*mtx.m[2][0] + gBlock_H*mtx.m[2][1] + mtx.m[2][2]);
//
//                topX = (int32_t)floorf(MIN(MIN(topLX, topRX), MIN(botLX, botRX))) - 1;
//                topY = (int32_t)floorf(MIN(MIN(topLY, topRY), MIN(botLY, botRY))) - 1;
//                botX = (int32_t)ceilf(MAX(MAX(topLX, topRX), MAX(botLX, botRX))) + 1;
//                botY = (int32_t)ceilf(MAX(MAX(topLY, topRY), MAX(botLY, botRY))) + 1;
//
//                //(void)printf("topL (%f, %f), topR (%f, %f)\n", topLX, topLY, topRX, topRY);
//                //(void)printf("botL (%f, %f), botR (%f, %f)\n", botLX, botLY, botRX, botRY);
//                //(void)printf("top (%d, %d), bot (%d, %d)\n", topX, topY, botX, botY);
//            }
//
//            mtx.m[0][0] = 1;   mtx.m[0][1] = 0;   mtx.m[0][2] = dx - topX;
//            mtx.m[1][0] = 0;   mtx.m[1][1] = 1;   mtx.m[1][2] = dy - topY;
//            mtx.m[2][0] = 0;   mtx.m[2][1] = 0;   mtx.m[2][2] = 1;
//
//            gfxMatrixMultiply(&mtx, &transform_m[num]);
//
//            gfxMatrixInverse(&mtx, &inverseMtx);
//
//            transform_m[num].m[2][0] = inverseMtx.m[2][0];
//            transform_m[num].m[2][1] = inverseMtx.m[2][1];
//            transform_m[num].m[0][0] = inverseMtx.m[0][0];
//            transform_m[num].m[0][1] = inverseMtx.m[0][1];
//            transform_m[num].m[0][2] = inverseMtx.m[0][2];
//            transform_m[num].m[1][2] = inverseMtx.m[1][2];
//            transform_m[num].m[1][0] = inverseMtx.m[1][0];
//            transform_m[num].m[1][1] = inverseMtx.m[1][1];
//            transform_m[num].m[2][2] = inverseMtx.m[2][2];
//        }
// #endif
//    }
//
// }
#endif
// =============================================================================
//                              Private Function Definition
// =============================================================================
