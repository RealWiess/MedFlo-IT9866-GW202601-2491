#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define ITUCURVE_CURVE_ALGORITHM 1     // 1: original, 2: vandermonde

#define ITUCURVE_VDM_MAX_ARRAYSIZE 512 // the maximum array size for vandermonde fit
#define ITUCURVE_VDM_POINTS 200        // vandermonde fit points (default: 200, must < ITUCURVE_VDM_MAX_ARRAYSIZE)
#define ITUCURVE_VDM_MIN_STEP_DIST 10  // the minimum distance between 2 real points to active vandermonde fit. (default: 10)
#define ITUCURVE_MAX_TMP_ARRSIZE  2048 // the maximum array size for local calculate

//internal usage array
static int ARRAYX[ITUCURVE_VDM_MAX_ARRAYSIZE];
static int ARRAYY[ITUCURVE_VDM_MAX_ARRAYSIZE];

static const char curveName[] = "ITUCurve";

void ituCurveExit(ITUWidget* widget)
{
    ITUCurve* curve = (ITUCurve*) widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (curve->cacheSurf)
    {
        ituSurfaceRelease(curve->cacheSurf);
        curve->cacheSurf = NULL;
    }

    ituIconExit(widget);
}

static void CurveDraw(ITUSurface* dest, int destx, int desty, ITUCurve* curve, int x, int y, uint8_t edge_alpha, uint8_t desta)
{
    ITUWidget* widget = (ITUWidget*)curve;
    ITUBackground* bg = (ITUBackground*)widget;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    ITURectangle prevClip;
    int height = rect->height - y;

    if (bg->icon.surf && !(curve->curveFlags & ITU_CURVE_LINE_ONLY))
    {
        ituWidgetSetClipping(widget, dest, x, y, &prevClip);

        if (height > 0)
        {
            if (desta > 0)
            {
                if (desta == 255)
                {
                    if (edge_alpha != 255)
                    {
                        ituAlphaBlend(dest, destx + x, desty + y, 1, 1, bg->icon.surf, x, y, 255 - edge_alpha);
                        ituBitBlt(dest, destx + x, desty + y + 1, 1, height - 1, bg->icon.surf, x, y);
                    }
                    else
                    {
                        ituBitBlt(dest, destx + x, desty + y, 1, height, bg->icon.surf, x, y);
                    }
                }
                else
                {
                    ituAlphaBlend(dest, destx + x, desty + y, 1, height, bg->icon.surf, x, y, desta);
                }
            }
        }
        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    }
    else
    {
        if (desta == 255)
        {
            if (height > 0)
            {
                int h = desty + rect->height - height;

                if (h < 0)
                {
                    h = 0;
                }

                if (edge_alpha != 255)
                {
                    if (bg->icon.surf && (curve->curveFlags & ITU_CURVE_LINE_ONLY))
                    {
                        ituAlphaBlend(dest, destx + x - bg->icon.surf->width / 2, desty + rect->height - height - bg->icon.surf->height / 2, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0, 255 - edge_alpha);
                    }
                    else
                    {
                        ITUSurface* surf = ituCreateSurface(1, rect->height, 0, dest->format, NULL, 0);
                        if (surf)
                        {
                            ituWidgetSetClipping(widget, dest, x, y, &prevClip);
                            ituSurfaceSetClipping(dest, destx + x, h, prevClip.width, prevClip.height);

                            if (bg->graidentMode == ITU_GF_NONE)
                            {
                                ituColorFill(surf, 0, 0, 1, 1, &widget->color);
                                ituAlphaBlend(dest, destx + x, desty + rect->height - height, 1, 1, surf, 0, 0, 255 - edge_alpha);

                                if (!(curve->curveFlags & ITU_CURVE_LINE_ONLY))
                                {
                                    ituColorFill(dest, destx + x, desty + rect->height - height + 1, 1, height - 1,
                                                 &widget->color);
                                }
                            }
                            else
                            {
                                ITUSurface* surf2 = ituCreateSurface(1, 1, 0, dest->format, NULL, 0);
                                if (surf2)
                                {
                                    ituBitBlt(surf2, 0, 0, 1, rect->height, dest, destx + x, desty + rect->height - height);
                                    ituGradientFill(surf, 0, 0, 1, rect->height, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                                    ituAlphaBlend(surf2, 0, 0, 1, 1, surf, 0, rect->height - height, 255 - edge_alpha);
                                    if (!(curve->curveFlags & ITU_CURVE_LINE_ONLY))
                                    {
                                        ituGradientFill(dest, destx + x, desty + 1, 1, rect->height - 1, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                                    }
                                    ituBitBlt(dest, destx + x, desty + rect->height - height, 1, 1, surf2, 0, 0);

                                    ituDestroySurface(surf2);
                                }
                            }
                            ituDestroySurface(surf);
                            ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                        }
                    }
                }
                else
                {
                    if (bg->icon.surf && (curve->curveFlags & ITU_CURVE_LINE_ONLY))
                    {
                        ituBitBlt(dest, destx + x - bg->icon.surf->width / 2, desty + rect->height - height - bg->icon.surf->height / 2, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0);
                    }
                    else
                    {
                        ituWidgetSetClipping(widget, dest, x, y, &prevClip);
                        ituSurfaceSetClipping(dest, destx + x, h, prevClip.width, prevClip.height);

                        if (bg->graidentMode == ITU_GF_NONE)
                        {
                            if (curve->curveFlags & ITU_CURVE_LINE_ONLY)
                            {
                                ituColorFill(dest, destx + x, desty + rect->height - height, 1, 1, &widget->color);
                            }
                            else
                            {
                                ituColorFill(dest, destx + x, desty + rect->height - height, 1, height, &widget->color);
                            }
                        }
                        else
                        {
                            if (curve->curveFlags & ITU_CURVE_LINE_ONLY)
                            {
                                ITUSurface* surf = ituCreateSurface(1, rect->height, 0, dest->format, NULL, 0);
                                if (surf)
                                {
                                    ituGradientFill(surf, 0, 0, 1, rect->height, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                                    ituBitBlt(dest, destx + x, desty + rect->height - height, 1, 1, surf, 0, rect->height - height);
                                    ituDestroySurface(surf);
                                }
                            }
                            else
                            {
                                ituGradientFill(dest, destx + x, desty + rect->height - height, 1, height, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                            }
                        }
                        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                    }
                }
            }
        }
        else if (desta > 0)
        {
            if (bg->icon.surf && (curve->curveFlags & ITU_CURVE_LINE_ONLY))
            {
                ituAlphaBlend(dest, destx + x - bg->icon.surf->width / 2, desty - bg->icon.surf->height / 2, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0, desta);
            }
            else
            {
                ITUSurface* surf = ituCreateSurface(1, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituWidgetSetClipping(widget, dest, x, y, &prevClip);
                    if (height > 0)
                    {
                        int h = desty + rect->height - height;

                        if (h < 0)
                        {
                            h = 0;
                        }

                        ituSurfaceSetClipping(dest, prevClip.x, h, prevClip.width, prevClip.height);

                        if (bg->graidentMode == ITU_GF_NONE)
                        {
                            if (curve->curveFlags & ITU_CURVE_LINE_ONLY)
                            {
                                ituColorFill(surf, 0, 0, 1, 1, &widget->color);
                            }
                            else
                            {
                                ituColorFill(surf, 0, 0, 1, height, &widget->color);
                            }
                        }
                        else
                        {
                            if (curve->curveFlags & ITU_CURVE_LINE_ONLY)
                            {
                                ITUSurface* surf2 = ituCreateSurface(1, rect->height, 0, dest->format, NULL, 0);
                                if (surf2)
                                {
                                    ituGradientFill(surf2, 0, 0, 1, rect->height, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                                    ituBitBlt(surf, 0, 0, 1, 1, surf, 0, rect->height - height);
                                    ituDestroySurface(surf2);
                                }
                            }
                            else
                            {
                                ituGradientFill(surf, 0, 0, 1, rect->height, &widget->color, &bg->graidentColor, ITU_GF_VERTICAL);
                            }
                        }
                    }
                    ituAlphaBlend(dest, destx + x, desty, 1, rect->height, surf, 0, 0, desta);
                    ituDestroySurface(surf);
                    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                }
            }
        }
        else
        {
            /* do nothing */
        }
    }
}

int itucurve_vandermonde_curve(int px0, int py0, int px1, int py1, int px2, int py2, int px3, int py3, int* X, int* Y, int size)
{
    int i = 0;
    float t = 0.0;
    double step;
    float x0 = (float)px0;
    float y0 = (float)py0;
    float x1 = (float)px1;
    float y1 = (float)py1;
    float x2 = (float)px2;
    float y2 = (float)py2;
    float x3 = (float)px3;
    float y3 = (float)py3;

    assert(size);

    step = 1.0 / (float)size;


    for (t = 0.0; t < 1.0; t += (float)step)
    {
        X[i] = (int)round((pow(1.0 - t, 3.0) * x0) + (3.0 * t * pow(1.0 - t, 2.0) * x1) + (3.0 * pow(t, 2.0) * (1.0 - t) * x2) + (pow(t, 3.0) * x3));
        Y[i] = (int)round((pow(1.0 - t, 3.0) * y0) + (3.0 * t * pow(1.0 - t, 2.0) * y1) + (3.0 * pow(t, 2.0) * (1.0 - t) * y2) + (pow(t, 3.0) * y3));
        i++;
    }
    return i;
}

static void plotLineAA(int x0, int y0, int x1, int y1, ITUSurface* dest, int destx, int desty, ITUCurve* curve, uint8_t alpha)
{
    int dx = abs(x1 - x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0<y1 ? 1 : -1;
    int err = dx - dy;                       /* error value e_xy */
    int ed = dx + dy == 0 ? 1 : (int)sqrtf((float)dx*dx + (float)dy*dy);

    for (;;)
    { /* pixel loop */
        CurveDraw(dest, destx, desty, curve, x0, y0, 255 * abs(err - dx + dy) / ed, alpha);
        int e2 = err, x2 = x0;
        if (2 * e2 >= -dx)
        { /* x step */
            if (x0 == x1)
            {
                break;
            }
            if (e2 + dy < ed)
            {
                CurveDraw(dest, destx, desty, curve, x0, y0 + sy, 255 * (e2 + dy) / ed, alpha);
            }
            err -= dy;
            x0 += sx;
        }
        if (2 * e2 <= dy)
        { /* y step */
            if (y0 == y1)
            {
                break;
            }
            if (dx - e2 < ed)
            {
                CurveDraw(dest, destx, desty, curve, x2 + sx, y0, 255 * (dx - e2) / ed, alpha);
            }
            err += dx;
            y0 += sy;
        }
    }
}

static void plotQuadBezierSegAA(int x0, int y0, int x1, int y1, int x2, int y2, ITUSurface* dest, int destx, int desty, ITUCurve* curve, uint8_t alpha)
{
    int sx = x2-x1, sy = y2-y1;
    long  xx = x0 - x1, yy = y0 - y1;     /* relative values for checks */
    float cur = (float)xx * sy - yy * sx; /* curvature */

    assert(xx * sx <= 0 && yy * sy <= 0); /* sign of gradient must not change */

    if (sx * (long)sx + sy * (long)sy > xx * xx + yy * yy)
    { /* begin with longer part */
        x2  = x0;
        x0  = sx + x1;
        y2  = y0;
        y0  = sy + y1;
        cur = -cur; /* swap P0 P2 */
    }
    if (cur != 0)
    { /* no straight line */
        float dx, dy, err;
        long  xy;
        xx += sx;
        xx *= sx = x0 < x2 ? 1 : -1; /* x step direction */
        yy += sy;
        yy *= sy = y0 < y2 ? 1 : -1; /* y step direction */
        xy       = 2 * xx * yy;
        xx *= xx;
        yy *= yy; /* differences 2nd degree */
        if (cur * sx * sy < 0)
        {         /* negated curvature? */
            xx  = -xx;
            yy  = -yy;
            xy  = -xy;
            cur = -cur;
        }
        dx = 4.0f * sy * cur * (x1 - x0) + xx - xy; /* differences 1st degree */
        dy = 4.0f * sx * cur * (y0 - y1) + yy - xy;
        xx += xx;
        yy += yy;
        err = dx + dy + xy; /* error 1st step */
        do
        {
            float ed = fmaxf(dx + xy, -xy - dy); /* approximate error distance */
            cur      = fminf(dx + xy, -xy - dy);
            ed       = 255 / (ed + 2 * ed * cur * cur / (4.0f * ed * ed + cur * cur));
            CurveDraw(dest, destx, desty, curve, x0, y0, (uint8_t)(ed * fabs(err - dx - dy - xy)),
                      alpha); /* plot curve */
            if (x0 == x2 && y0 == y2)
            {
                return; /* last pixel -> curve finished */
            }
            x1  = x0;
            cur = dx - err;
            y1  = 2 * err + dy < 0;
            if (2 * err + dx > 0)
            { /* x step */
                if (err - dy < ed)
                {
                    CurveDraw(dest, destx, desty, curve, x0, y0 + sy, (uint8_t)(ed * fabs(err - dy)), alpha);
                }
                x0 += sx;
                dx -= xy;
                err += dy += yy;
            }
            if (y1)
            { /* y step */
                if (cur < ed)
                {
                    CurveDraw(dest, destx, desty, curve, x1 + sx, y0, (uint8_t)(ed * fabs(cur)), alpha);
                }
                y0 += sy;
                dy -= xy;
                err += dx += xx;
            }
        } while (dy < dx);                                        /* gradient negates -> close curves */
    }
    plotLineAA(x0, y0, x2, y2, dest, destx, desty, curve, alpha); /* plot remaining needle to end */
}

static void plotCubicBezierSeg (int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                                ITUSurface * dest, int destx, int desty, ITUCurve * curve, uint8_t alpha)
{                                                       /* plot limited cubic Bezier segment */
    int   f, fx, fy, leg = 1;
    int   sx = x0 < x3 ? 1 : -1, sy = y0 < y3 ? 1 : -1; /* step direction */
    float xc = -fabsf(x0 + x1 - x2 - x3), xa = xc - 4 * sx * (x1 - x2), xb = sx * (x0 - x1 - x2 + x3);
    float yc = -fabsf(y0 + y1 - y2 - y3), ya = yc - 4 * sy * (y1 - y2), yb = sy * (y0 - y1 - y2 + y3);
    float ab, ac, bc, ba, xx, xy, yy, dx, dy, ex, px, py, ed, ip, EP = 0.01f;

    /* check for curve restrains */
    /* slope P0-P1 == P2-P3     and  (P0-P3 == P1-P2      or  no slope change) */
    assert((x1 - x0) * (x2 - x3) < EP && ((x3 - x0) * (x1 - x2) < EP || xb * xb < xa * xc + EP));
    assert((y1 - y0) * (y2 - y3) < EP && ((y3 - y0) * (y1 - y2) < EP || yb * yb < ya * yc + EP));

    if (xa == 0 && ya == 0)
    {                                                      /* quadratic Bezier */
        sx = (int)floorf((float)(3 * x1 - x0 + 1) / 2.0f);
        sy = (int)floorf((float)(3 * y1 - y0 + 1) / 2.0f); /* new midpoint */
        plotQuadBezierSegAA(x0, y0, sx, sy, x3, y3, dest, destx, desty, curve, alpha);
        return;
    }
    x1 = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + 1; /* line lengths */
    x2 = (x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3) + 1;
    do
    { /* loop over both ends */
        ab = xa * yb - xb * ya;
        ac = xa * yc - xc * ya;
        bc = xb * yc - xc * yb;
        ip = 4 * ab * bc - ac * ac;                  /* self intersection loop at all? */
        ex = ab * (ab + ac - 3 * bc) + ac * ac;      /* P0 part of self-intersection loop? */
        f  = ex > 0 ? 1 : (int)sqrtf(1 + 1024 / x1); /* calculate resolution */
        ab *= f;
        ac *= f;
        bc *= f;
        ex *= f * f;        /* increase resolution */
        xy = 9 * (ab + ac + bc) / 8;
        ba = 8 * (xa - ya); /* init differences of 1st degree */
        dx = 27 * (8 * ab * (yb * yb - ya * yc) + ex * (ya + 2 * yb + yc)) / 64 - ya * ya * (xy - ya);
        dy = 27 * (8 * ab * (xb * xb - xa * xc) - ex * (xa + 2 * xb + xc)) / 64 - xa * xa * (xy + xa);
        /* init differences of 2nd degree */
        xx = 3 * (3 * ab * (3 * yb * yb - ya * ya - 2 * ya * yc) - ya * (3 * ac * (ya + yb) + ya * ba)) / 4;
        yy = 3 * (3 * ab * (3 * xb * xb - xa * xa - 2 * xa * xc) - xa * (3 * ac * (xa + xb) + xa * ba)) / 4;
        xy = xa * ya * (6 * ab + 6 * ac - 3 * bc + ba);
        ac = ya * ya;
        ba = xa * xa;
        xy = 3 * (xy + 9 * f * (ba * yb * yc - xb * xc * ac) - 18 * xb * yb * ab) / 8;

        if (ex < 0)
        { /* negate values if inside self-intersection loop */
            dx = -dx;
            dy = -dy;
            xx = -xx;
            yy = -yy;
            xy = -xy;
            ac = -ac;
            ba = -ba;
        } /* init differences of 3rd degree */
        ab = 6 * ya * ac;
        ac = -6 * xa * ac;
        bc = 6 * ya * ba;
        ba = -6 * xa * ba;
        dx += xy;
        ex = dx + dy;
        dy += xy; /* error of 1st step */

        for (fx = fy = f; x0 != x3 && y0 != y3;)
        {
            y1 = fminf(fabsf(xy - dx), fabsf(dy - xy));
            ed = fmaxf(fabsf(xy - dx), fabsf(dy - xy)); /* approximate error distance */
            ed = f * (ed + 2 * ed * y1 * y1 / (4 * ed * ed + y1 * y1));
            y1 = 255 * fabsf(ex - (f - fx + 1) * dx - (f - fy + 1) * dy + f * xy) / ed;
            if (y1 < 256)
            {
                CurveDraw(dest, destx, desty, curve, x0, y0, (uint8_t)y1, alpha); /* plot curve */
            }
            px = fabsf(ex - (f - fx + 1) * dx + (fy - 1) * dy);                   /* pixel intensity x move */
            py = fabsf(ex + (fx - 1) * dx - (f - fy + 1) * dy);                   /* pixel intensity y move */
            y2 = (float)y0;
            do
            {                  /* move sub-steps of one pixel */
                if (ip >= -EP) /* intersection possible? -> check.. */
                {
                    if (dx + xx > xy || dy + yy < xy)
                    {
                        goto exit; /* two x or y steps */
                    }
                }
                y1 = 2 * ex + dx; /* save value for test of y step */
                if (2 * ex + dy > 0)
                {                 /* x sub-step */
                    fx--;
                    ex += dx += xx;
                    dy += xy += ac;
                    yy += bc;
                    xx += ab;
                }
                else if (y1 > 0)
                {
                    goto exit; /* tiny nearly cusp */
                }
                else
                {
                    /* do nothing */
                }

                if (y1 <= 0)
                { /* y sub-step */
                    fy--;
                    ex += dy += yy;
                    dx += xy += bc;
                    xx += ac;
                    yy += ba;
                }
            } while (fx > 0 && fy > 0); /* pixel complete? */
            if (2 * fy <= f)
            {                           /* x+ anti-aliasing pixel */
                if (py < ed)
                {
                    CurveDraw(dest, destx, desty, curve, x0 + sx, y0, (uint8_t)(255 * py / ed), alpha); /* plot curve */
                }
                y0 += sy;
                fy += f; /* y step */
            }
            if (2 * fx <= f)
            { /* y+ anti-aliasing pixel */
                if (px < ed)
                {
                    CurveDraw(dest, destx, desty, curve, x0, (int)y2 + sy, (uint8_t)(255 * px / ed),
                              alpha); /* plot curve */
                }
                x0 += sx;
                fx += f; /* x step */
            }
        }
        break; /* finish curve by line */
    exit:
        if (2 * ex < dy && 2 * fy <= f + 2)
        { /* round x+ approximation pixel */
            if (py < ed)
            {
                CurveDraw(dest, destx, desty, curve, x0 + sx, y0, (uint8_t)(255 * py / ed), alpha); /* plot curve */
            }
            y0 += sy;
        }
        if (2 * ex > dx && 2 * fx <= f + 2)
        { /* round y+ approximation pixel */
            if (px < ed)
            {
                CurveDraw(dest, destx, desty, curve, x0, (int)y2 + sy, (uint8_t)(255 * px / ed),
                          alpha); /* plot curve */
            }
            x0 += sx;
        }
        xx = (float)x0;
        x0 = x3;
        x3 = (int)xx;
        sx = -sx;
        xb = -xb; /* swap legs */
        yy = (float)y0;
        y0 = y3;
        y3 = (int)yy;
        sy = -sy;
        yb = -yb;
        x1 = x2;
    } while (leg--); /* try other end */
    plotLineAA(x0, y0, x3, y3, dest, destx, desty, curve, alpha);
}

static void plotCubicBezier (int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, ITUSurface * dest,
                             int destx, int desty, ITUCurve * curve, uint8_t alpha)
{ /* plot any cubic Bezier curve */
    int   n = 0, i = 0;
    long  xc = x0 + x1 - x2 - x3, xa = xc - 4 * (x1 - x2);
    long  xb = x0 - x1 - x2 + x3, xd = xb + 4 * (x1 + x2);
    long  yc = y0 + y1 - y2 - y3, ya = yc - 4 * (y1 - y2);
    long  yb = y0 - y1 - y2 + y3, yd = yb + 4 * (y1 + y2);
    float fx0 = (float)x0, fy0 = (float)y0;
    float t1 = (float)xb * xb - xa * xc, t2, t[5];
    /* sub-divide curve at gradient sign changes */
    if (xa == 0)
    { /* horizontal */
        if (fabsf((float)xc) < 2 * fabsf((float)xb))
        {
            t[n++] = xc / (2.0f * xb); /* one change */
        }
    }
    else if (t1 > 0.0)
    { /* two changes */
        t2 = sqrtf(t1);
        t1 = (xb - t2) / xa;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
        t1 = (xb + t2) / xa;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
    }
    else
    {
        /* do nothing */
    }

    t1 = (float)(yb * yb - ya * yc);
    if (ya == 0)
    { /* vertical */
        if (fabsf((float)yc) < 2 * fabsf((float)yb))
        {
            t[n++] = yc / (2.0f * yb); /* one change */
        }
    }
    else if (t1 > 0.0f)
    { /* two changes */
        t2 = sqrtf(t1);
        t1 = (yb - t2) / ya;
        if (fabs(t1) < 1.0f)
        {
            t[n++] = t1;
        }
        t1 = (yb + t2) / ya;
        if (fabs(t1) < 1.0f)
        {
            t[n++] = t1;
        }
    }
    else
    {
        /* do nothing */
    }

    for (i = 1; i < n; i++) /* bubble sort of 4 points */
    {
        if ((t1 = t[i - 1]) > t[i])
        {
            t[i - 1] = t[i];
            t[i]     = t1;
            i        = 0;
        }
    }
    t1   = -1.0f;
    t[n] = 1.0f;           /* begin / end point */
    for (i = 0; i <= n; i++)
    {                      /* plot each segment separately */
        float fx1, fx2, fx3, fy1, fy2, fy3;
        t2         = t[i]; /* sub-divide at t[i-1], t[i] */
        fx1        = (t1 * (t1 * xb - 2 * xc) - t2 * (t1 * (t1 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
        fy1        = (t1 * (t1 * yb - 2 * yc) - t2 * (t1 * (t1 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
        fx2        = (t2 * (t2 * xb - 2 * xc) - t1 * (t2 * (t2 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
        fy2        = (t2 * (t2 * yb - 2 * yc) - t1 * (t2 * (t2 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
        fx0 -= fx3 = (t2 * (t2 * (3 * xb - t2 * xa) - 3 * xc) + xd) / 8;
        fy0 -= fy3 = (t2 * (t2 * (3 * yb - t2 * ya) - 3 * yc) + yd) / 8;
        x3         = (int)floorf(fx3 + 0.5f);
        y3         = (int)floorf(fy3 + 0.5f); /* scale bounds to int */
        if (fx0 != 0.0f)
        {
            fx1 *= fx0 = (x0 - x3) / fx0;
            fx2 *= fx0;
        }
        if (fy0 != 0.0f)
        {
            fy1 *= fy0 = (y0 - y3) / fy0;
            fy2 *= fy0;
        }
        if (x0 != x3 || y0 != y3) /* segment t1 - t2 */
        {
            plotCubicBezierSeg(x0, y0, x0 + fx1, y0 + fy1, x0 + fx2, y0 + fy2, x3, y3, dest, destx, desty, curve,
                               alpha);
        }
        x0  = x3;
        y0  = y3;
        fx0 = fx3;
        fy0 = fy3;
        t1  = t2;
    }
}

static void plotCubicSpline (int n, int x[], int y[], ITUSurface * dest, int destx, int desty, ITUCurve * curve,
                             uint8_t alpha)
{                                        /* plot cubic spline, destroys input arrays x,y */
#define M_MAX 6
    float mi = 0.25f, m[M_MAX] = {0.0f}; /* diagonal constants of matrix */
    int   x3 = x[n - 1], y3 = y[n - 1], x4 = x[n], y4 = y[n];
    int   i, x0, y0, x1, y1, x2, y2;

    assert(n > 2);                    /* need at least 4 points P[0]..P[n] */

    x[1] = x0 = 12 * x[1] - 3 * x[0]; /* first row of matrix */
    y[1] = y0 = 12 * y[1] - 3 * y[0];

    for (i = 2; i < n; i++)
    { /* foreward sweep */
        if (i - 2 < M_MAX)
        {
            m[i - 2] = mi = 0.25f / (2.0f - mi);
        }
        x[i] = x0 = (int)floor(12.0f * x[i] - 2 * x0 * mi + 0.5f);
        y[i] = y0 = (int)floor(12.0f * y[i] - 2 * y0 * mi + 0.5f);
    }
    x2 = (int)floor((float)(x0 - 3 * x4) / (7.0f - 4 * mi) + 0.5f); /* correct last row */
    y2 = (int)floor((float)(y0 - 3 * y4) / (7.0f - 4 * mi) + 0.5f);
    plotCubicBezier(x3, y3, (x2 + x4) / 2, (y2 + y4) / 2, x4, y4, x4, y4, dest, destx, desty, curve, alpha);

    if (n - 3 < M_MAX)
    {
        mi = m[n - 3];
    }
    x1 = (int)floor((x[n - 2] - 2 * x2) * mi + 0.5f);
    y1 = (int)floor((y[n - 2] - 2 * y2) * mi + 0.5f);
    for (i = n - 3; i > 0; i--)
    { /* back substitution */
        if (i <= M_MAX)
        {
            mi = m[i - 1];
        }
        x0 = (int)floor((x[i] - 2 * x1) * mi + 0.5f);
        y0 = (int)floor((y[i] - 2 * y1) * mi + 0.5f);
        x4 = (int)floor((x0 + 4 * x1 + x2 + 3) / 6.0f); /* reconstruct P[i] */
        y4 = (int)floor((y0 + 4 * y1 + y2 + 3) / 6.0f);
        plotCubicBezier(x4, y4, (int)roundf((float)(2 * x1 + x2) / 3.0f + 0.5f),
                        (int)floor((float)(2 * y1 + y2) / 3.0f + 0.5f), (int)roundf((float)(x1 + 2 * x2) / 3.0f + 0.5f),
                        (int)floor((float)(y1 + 2 * y2) / 3.0f + 0.5f), x3, y3, dest, destx, desty, curve, alpha);
        x3 = x4;
        y3 = y4;
        x2 = x1;
        y2 = y1;
        x1 = x0;
        y1 = y0;
    }
    x0 = x[0];
    x4 = (int)floor((float)(3 * x0 + 7 * x1 + 2 * x2 + 6) / 12.0f); /* reconstruct P[1] */
    y0 = y[0];
    y4 = (int)floor((float)(3 * y0 + 7 * y1 + 2 * y2 + 6) / 12.0f);
    plotCubicBezier(x4, y4, (int)floor((float)(2 * x1 + x2) / 3.0f + 0.5f),
                    (int)floor((float)(2 * y1 + y2) / 3.0f + 0.5f), (int)floor((float)(x1 + 2 * x2) / 3.0f + 0.5f),
                    (int)floor((float)(y1 + 2 * y2) / 3.0f + 0.5f), x3, y3, dest, destx, desty, curve, alpha);
    plotCubicBezier(x0, y0, x0, y0, (x0 + x1) / 2, (y0 + y1) / 2, x4, y4, dest, destx, desty, curve, alpha);
}

bool ituCurveUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool       result;
    ITUCurve * curve = (ITUCurve *)widget;
    assert(curve);

    result = ituIconUpdate(widget, ev, arg1, arg2, arg3);
    if (ev == ITU_EVENT_LAYOUT)
    {
        if (curve->cacheSurf)
        {
            ituSurfaceRelease(curve->cacheSurf);
            curve->cacheSurf = NULL;
        }

        (void)memset(ARRAYX, 0, sizeof(ARRAYX));
        (void)memset(ARRAYY, 0, sizeof(ARRAYY));
    }
    return widget->visible ? result : false;
}

void ituCurveDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int             destx, desty;
    uint8_t         desta;
    ITURectangle    prevClip;
    ITUBackground * bg         = (ITUBackground *)widget;
    ITUCurve *      curve      = (ITUCurve *)widget;
    ITURectangle *  rect       = (ITURectangle *)&widget->rect;
    // int xPoints[ITU_CURVE_MAX_POINT_COUNT];
    // int yPoints[ITU_CURVE_MAX_POINT_COUNT];
    int             pointCount = (curve->pointCount < 4) ? (4) : (curve->pointCount);
    int *           xPoints    = (int *)malloc(sizeof(int) * pointCount);
    int *           yPoints    = (int *)malloc(sizeof(int) * pointCount);
    assert(curve);
    assert(dest);

    if ((pointCount == 0) || (xPoints == NULL) || (yPoints == NULL) || (curve->xPP == NULL) || (curve->yPP == NULL))
    {
        if (xPoints != NULL)
        {
            free(xPoints);
        }

        if (yPoints != NULL)
        {
            free(yPoints);
        }

        return;
    }

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->color.alpha / 255;
    desta = desta * widget->alpha / 255;

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    if ((curve->curveFlags & ITU_CURVE_STATIC))
    {
        if (!curve->cacheSurf)
        {
            (void)memcpy(xPoints, curve->xPP, sizeof(int) * pointCount);
            (void)memcpy(yPoints, curve->yPP, sizeof(int) * pointCount);

            if (pointCount < 4)
            {
                int i = pointCount;
                for (; i < 4; ++i)
                {
                    xPoints[i] = xPoints[pointCount - 1];
                    yPoints[i] = yPoints[pointCount - 1];
                }
                pointCount = 4;
            }

            if (bg->icon.surf && (curve->curveFlags & ITU_CURVE_LINE_ONLY))
            {
                ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituSurfaceSetClipping(surf, 0, 0, rect->width, rect->height);

                    ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);

                    if (ITUCURVE_CURVE_ALGORITHM == 1)
                    {
                        plotCubicSpline(pointCount - 1, xPoints, yPoints, surf, 0, 0, curve, desta);
                    }

                    curve->cacheSurf = surf;
                }
            }
            else
            {
                ITUSurface * surf = ituCreateSurface(dest->width, dest->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituBitBlt(surf, destx, desty, rect->width, rect->height, dest, destx, desty);
                    surf->clipping = dest->clipping;

                    if (ITUCURVE_CURVE_ALGORITHM == 1)
                    {
                        plotCubicSpline(pointCount - 1, xPoints, yPoints, surf, destx, desty, curve, desta);
                    }
                    else
                    {
                    }

                    curve->cacheSurf = surf;
                }
            }
        }

        if (curve->cacheSurf)
        {
            if (bg->icon.surf && (curve->curveFlags & ITU_CURVE_LINE_ONLY))
            {
                ituBitBlt(dest, destx, desty, rect->width, rect->height, curve->cacheSurf, 0, 0);
            }
            else
            {
                ituBitBlt(dest, destx, desty, rect->width, rect->height, curve->cacheSurf, destx, desty);
            }
        }
    }
    else
    {
        (void)memcpy(xPoints, curve->xPP, sizeof(int) * pointCount);
        (void)memcpy(yPoints, curve->yPP, sizeof(int) * pointCount);

        if (pointCount < 4)
        {
            int i = pointCount;
            for (; i < 4; ++i)
            {
                xPoints[i] = xPoints[pointCount - 1];
                yPoints[i] = yPoints[pointCount - 1];
            }
            pointCount = 4;
        }

        if (ITUCURVE_CURVE_ALGORITHM == 1)
        {
            plotCubicSpline(pointCount - 1, xPoints, yPoints, dest, destx, desty, curve, desta);
        }
        else
        {
            int i                       = 0;
            int j                       = 0;
            int LX                      = xPoints[0];
            int LY                      = yPoints[0];
            int count                   = (pointCount < 4) ? (4) : (pointCount);
            int edgea                   = (curve->curveFlags & ITU_CURVE_LINE_ONLY) ? (100) : (255);
            int apArrX[ITUCURVE_MAX_TMP_ARRSIZE]           = {0};
            int apArrY[ITUCURVE_MAX_TMP_ARRSIZE]           = {0};
            int apCnt                   = 1;
            int apStep                  = 0;
            int fitRefDist              = widget->rect.width / (count * 3); // 30
            int fitStepBlock            = 5;
            int fitValueCheckArr[ITUCURVE_MAX_TMP_ARRSIZE] = {0};
            int arrayDensity            = 0;

            if (fitRefDist < ITUCURVE_VDM_MIN_STEP_DIST)
            {
                fitRefDist = ITUCURVE_VDM_MIN_STEP_DIST;
            }
            arrayDensity = (curve->curveFlags & ITU_CURVE_LINE_ONLY) ? (ITUCURVE_VDM_POINTS) : (fitRefDist * 3);

            for (i = 0; i < (count - 1); i++)
            {

                float Slope         = 0;
                bool  fitValueCheck = false;

                if (1)
                {
                    if (abs(xPoints[i] - xPoints[i + 1]) >= fitRefDist)
                    {
                        fitValueCheck = true;
                        apStep        = abs(xPoints[i] - xPoints[i + 1]);
                        Slope         = (float)(yPoints[i + 1] - yPoints[i]) /
                                (xPoints[i + 1] - xPoints[i] - (4 * fitRefDist / fitStepBlock));
                    }
                }
                
                if (j >= ITUCURVE_MAX_TMP_ARRSIZE)
                {
                    continue;
                }

                if (fitValueCheck)
                {
                    int m      = 0;
                    int arrIdx = j;
                    for (m = 0; m < (2 * fitRefDist / fitStepBlock);)
                    {
                        apArrX[arrIdx]           = xPoints[i] + m;
                        apArrY[arrIdx]           = yPoints[i];
                        fitValueCheckArr[arrIdx] = 1;
                        m += fitRefDist / fitStepBlock;
                        arrIdx++;
                    }

                    for (m = (2 * fitRefDist / fitStepBlock); m < apStep;)
                    {
                        apArrX[arrIdx] = xPoints[i] + m;
                        apArrY[arrIdx] =
                            (int)roundf((apArrX[arrIdx] - apArrX[arrIdx - 1]) * Slope) + apArrY[arrIdx - 1];
                        if (Slope < 0.0)
                        {
                            if (apArrY[arrIdx] < yPoints[i + 1])
                            {
                                apArrY[arrIdx] = yPoints[i + 1];
                            }
                        }
                        else if (Slope > 0.0)
                        {
                            if (apArrY[arrIdx] > yPoints[i + 1])
                            {
                                apArrY[arrIdx] = yPoints[i + 1];
                            }
                        }
                        else
                        {
                            /* do nothing */
                        }
                        fitValueCheckArr[arrIdx] = 1;
                        m += fitRefDist / fitStepBlock;
                        arrIdx++;
                    }
                    apCnt += arrIdx - j;
                    j = arrIdx;
                }
                else
                {
                    apArrX[j]           = xPoints[i];
                    apArrY[j]           = yPoints[i];
                    fitValueCheckArr[j] = 0;
                    j++;
                    apCnt++;
                }
                if ((i + 1) == (count - 1))
                {
                    apArrX[j]           = xPoints[i + 1];
                    apArrY[j]           = yPoints[i + 1];
                    fitValueCheckArr[j] = 0;
                }
            }

            if (count < apCnt)
            {
                count = pointCount = apCnt;
            }

            for (i = 0; count > 0;)
            {
                if ((i + 3) < ITUCURVE_MAX_TMP_ARRSIZE)
                {
                    int x1 = apArrX[i];
                    int y1 = apArrY[i];
                    int x2, y2, x3, y3, x4, y4;
                    if ((i + 1) < pointCount)
                    {
                        x2 = LX = apArrX[i + 1];
                        y2 = LY = apArrY[i + 1];
                        if ((i + 2) < pointCount)
                        {
                            x3 = LX = apArrX[i + 2];
                            y3 = LY = apArrY[i + 2];
                            if ((i + 3) < pointCount)
                            {
                                x4 = LX = apArrX[i + 3];
                                y4 = LY = apArrY[i + 3];
                            }
                            else
                            {
                                x4 = LX;
                                y4 = LY;
                            }
                        }
                        else
                        {
                            x3 = x4 = LX;
                            y3 = y4 = LY;
                        }
                    }
                    else
                    {
                        x2 = x3 = x4 = LX;
                        y2 = y3 = y4 = LY;
                    }
                    if (fitValueCheckArr[i])
                    {
                        int cstep, vanArrayCnt = itucurve_vandermonde_curve(x1, y1, x2, y2, x3, y3, x4, y4, &ARRAYX[0],
                            &ARRAYY[0], arrayDensity);
                        i += 3;
                        count -= 3;
                        for (cstep = 0; cstep < vanArrayCnt; cstep++)
                        {
                            CurveDraw(dest, destx, desty, curve, ARRAYX[cstep], ARRAYY[cstep], edgea, desta);
                        }
                    }
                    else
                    {
                        CurveDraw(dest, destx, desty, curve, apArrX[i], apArrY[i], edgea, desta);
                        i++;
                        count--;
                    }
                }
                else
                {
                    break;
                }
                // (void)printf("1\n");
            }
        } // else
    }
    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);

    free(xPoints);
    free(yPoints);
}

void ituCurveInit (ITUCurve * curve)
{
    assert(curve);
    ITU_ASSERT_THREAD();

    (void)memset(curve, 0, sizeof (ITUCurve));

    ituBackgroundInit(&curve->bg);

    ituWidgetSetType(curve, ITU_CURVE);
    ituWidgetSetName(curve, curveName);
    ituWidgetSetExit(curve, ituCurveExit);
    ituWidgetSetUpdate(curve, ituCurveUpdate);
    ituWidgetSetDraw(curve, ituCurveDraw);
}

void ituCurveLoad(ITUCurve* curve, uint32_t base)
{
    assert(curve);

    ituBackgroundLoad(&curve->bg, base);
    ituWidgetSetExit(curve, ituCurveExit);
    ituWidgetSetUpdate(curve, ituCurveUpdate);
    ituWidgetSetDraw(curve, ituCurveDraw);

    if (1)
    {
        int arrCount = curve->pointCount;
        if (arrCount < 4)
        {
            arrCount = 4;
        }
        curve->xPP = (int *)malloc(sizeof(int) * arrCount);
        curve->yPP = (int *)malloc(sizeof(int) * arrCount);
        if ((curve->xPP != NULL) && (curve->yPP != NULL))
        {
            int copySize = (curve->pointCount < arrCount) ? (curve->pointCount) : (arrCount);
            (void)memcpy(curve->xPP, curve->xPoints, sizeof(int) * copySize);
            (void)memcpy(curve->yPP, curve->yPoints, sizeof(int) * copySize);
            if ((curve->pointCount < 4) && (curve->pointCount < copySize))
            {
                int i;
                for (i = curve->pointCount; i < 4; i++)
                {
                    if (i >= 1) //array safe
                    {
                        *(curve->xPP + i) = *(curve->xPP + i - 1);
                        *(curve->yPP + i) = *(curve->yPP + i - 1);
#if 0
                        curve->xPP[i] = curve->xPP[i - 1];
                        curve->yPP[i] = curve->yPP[i - 1];
#endif
                    }
                }
                curve->pointCount = 4;
            }
        }
    }
}

void ituCurveSetPointCount(ITUCurve* curve, int count)
{
    if (curve != NULL)
    {
        if ((count >= 4) && (curve->xPP != NULL) && (curve->yPP != NULL))
        {
            int lastCount = curve->pointCount;
            int* xPoints = (int*)malloc(sizeof(int) * lastCount);
            int* yPoints = (int*)malloc(sizeof(int) * lastCount);
            if ((xPoints != NULL) && (yPoints != NULL))
            {
                (void)memcpy(xPoints, curve->xPP, sizeof(int) * lastCount);
                (void)memcpy(yPoints, curve->yPP, sizeof(int) * lastCount);
            }
            curve->pointCount = count;
            free(curve->xPP);
            free(curve->yPP);
            curve->xPP = (int*)malloc(sizeof(int) * count);
            curve->yPP = (int*)malloc(sizeof(int) * count);
            if ((curve->xPP != NULL) && (curve->yPP != NULL) && (xPoints != NULL) && (yPoints != NULL))
            {
                (void)memset(curve->xPP, 0, sizeof(int) * count);
                (void)memset(curve->yPP, 0, sizeof(int) * count);
                (void)memcpy(curve->xPP, xPoints, sizeof(int) * ((lastCount <= count) ? (lastCount) : (count)));
                (void)memcpy(curve->yPP, yPoints, sizeof(int) * ((lastCount <= count) ? (lastCount) : (count)));
            }
            free(xPoints);
            free(yPoints);
        }
    }
}

void ituCurveSetPoint (ITUCurve * curve, int * pointX, int * pointY, int count)
{
    if (curve)
    {
        const ITUWidget * widget = (ITUWidget *)curve;
        if (count <= curve->pointCount)
        {
            (void)memcpy(curve->xPP, pointX, sizeof(int) * count);
            (void)memcpy(curve->yPP, pointY, sizeof(int) * count);
        }
        else
        {
            (void)printf("[curve %s] ituCurveSetPoint set count too large.[error]\n", widget->name);
        }
    }
}
