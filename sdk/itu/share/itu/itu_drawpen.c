#include <assert.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifdef CFG_BUILD_JPEG
    #include <jpeglib.h>
#endif
#include <pthread.h>
#include <semaphore.h>
#include "ite/itu.h"
#include "ite/itp.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define PENCURSOR_HEADSIZE           6    // 6
#define MAX_PENCURVE_ARRAY           1024 // 1024
#define MAX_PEN_PRESSURE             16   // 16
#define MIN_PEN_PRESSURE             4
#define SMOOTH_FACTOR                20   // 20
// 20 is limit
// #define DRAWPEN_USB_DEVICE_DEBUG
// #define SEND_MAX_CURVE_RAW
#define SEND_MAX_CURVE_RAW_FACTOR    2

#define ID_CUSTOER_DEMOLOGGER        0x99990000
#define ID_CUSTOER_DEMOECHO          0x99990001
#define ID_DRAWPEN_USB_CURVE         0x99990002
#define ID_DRAWPEN_USB_MOUSEDOWN     0x99990003
#define ID_DRAWPEN_USB_MOUSEUP       0x99990004
#define ID_DRAWPEN_USB_BUTTON_OK     0x99990005
#define ID_DRAWPEN_USB_BUTTON_CANCEL 0x99990006
#define ID_DRAWPEN_USB_BUTTON_CLEAR  0x99990007

#define ID_DRAWPEN_USB_TEST          0x99999999

#define FLAGS_OVERRUN                0x1
#define DRAWPEN_FIT_POINT_MODE       0

#pragma warning(disable : 4028)

struct FIFOB{
    int *buf;
    int putP, getP, size, free, flags;
};

static const char drawpenName[] = "ITUDrawPen";

#ifdef SEND_MAX_CURVE_RAW
static int ARRAYDATA[(MAX_PENCURVE_ARRAY * 2 * SEND_MAX_CURVE_RAW_FACTOR) + 4 + 5] = { 0 };
static int ARRAYX2[MAX_PENCURVE_ARRAY * SEND_MAX_CURVE_RAW_FACTOR] = { 0 };
static int ARRAYY2[MAX_PENCURVE_ARRAY * SEND_MAX_CURVE_RAW_FACTOR] = { 0 };
#else
static int ARRAYDATA[(MAX_PENCURVE_ARRAY * 2) + 4 + 5] = { 0 };
static int ARRAYX[MAX_PENCURVE_ARRAY] = { 0 };
static int ARRAYY[MAX_PENCURVE_ARRAY] = { 0 };
#endif

static int BUFFERX[1024] = { 0 };
static int BUFFERY[1024] = { 0 };
static struct FIFOB fifoX;
static struct FIFOB fifoY;

static ITUColor emptycolor;
static ITUSurface* pressure_surf[MAX_PEN_PRESSURE + 1];
static sem_t sem;
static pthread_t task;

static int fsg_connected = 0;
struct prop {
    unsigned int size;
    unsigned int id;
    unsigned int flags;
    signed int status;
};

#ifndef CFG_M2D_ENABLE
int idbSend2Host (void * data, int size, int wait)
{
    return 0;
}
int idbSend2HostAsync (void * data, int size, int wait)
{
    return 0;
}
#else
    #ifndef CFG_USBD_IDB
int        idbSend2Host (void * data, int size, int wait)
{
    return 0;
}
int idbSend2HostAsync (void * data, int size, int wait)
{
    return 0;
}
    #endif
#endif

void fifoB_init(struct FIFOB *fifo, int size, int *buf)
{
    fifo->buf = buf;
    fifo->flags = 0;
    fifo->free = size;
    fifo->size = size;
    fifo->putP = 0;
    fifo->getP = 0;

    return;
}

int fifoB_putPut(struct FIFOB *fifo, int data)
{
    if (fifo->free == 0)
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->putP] = data;
    fifo->putP++;

    if (fifo->putP == fifo->size)
    {
        fifo->putP = 0;
    }
    fifo->free--;

    return 0;
}

int fifoB_get (struct FIFOB * fifo)
{
    int data;
    if (fifo->free == fifo->size)
    {
        return -1;
    }
    data = fifo->buf[fifo->getP];
    fifo->getP++;
    if (fifo->getP == fifo->size)
    {
        fifo->getP = 0;
    }
    fifo->free++;

    return data;
}

int fifoB_status(const struct FIFOB *fifo)
{
    return fifo->size - fifo->free;
}

int fifoB_free(const struct FIFOB *fifo)
{
    return fifo->free;
}

void fifoB_getall(struct FIFOB *fifox, struct FIFOB *fifoy, ITUDrawPen* drawpen)
{
    drawpen->px0 = fifoB_get(fifox);
    drawpen->py0 = fifoB_get(fifoy);
    drawpen->px1 = fifoB_get(fifox);
    drawpen->py1 = fifoB_get(fifoy);
    drawpen->px2 = fifoB_get(fifox);
    drawpen->py2 = fifoB_get(fifoy);
    drawpen->px3 = fifoB_get(fifox);
    drawpen->py3 = fifoB_get(fifoy);
}


static void* USB_SEND_TASK(void* arg)
{
    ITUDrawPen * drawpen = (ITUDrawPen *)arg;

    if (drawpen != NULL)
    {
        if (!drawpen->rawdata)
        {
#ifdef SEND_MAX_CURVE_RAW
            (void)memset(ARRAYDATA, 0, sizeof(int) * (MAX_PENCURVE_ARRAY * 2 * SEND_MAX_CURVE_RAW_FACTOR + 4 + 5));
#else
            (void)memset(ARRAYDATA, 0, sizeof(int) * (MAX_PENCURVE_ARRAY * 2 + 4 + 5));
#endif
            drawpen->rawdata = &ARRAYDATA[0];
        }

#if 0
        // sem_t* sem = (sem_t *)drawpen->usb_lock;
        // int rc = sem_wait(&sem);
#endif

        if (drawpen->usb_working == 0)
        {
            (void)printf("[USB]Job stop!\n");
        }
        else
        {
            if (drawpen->rawdata != NULL)
            {
                int           dsize = drawpen->rawdata[0];
                struct prop* propOut = (struct prop*)drawpen->rawdata;

                if (propOut != NULL)
                {
                    if (dsize == 1)
                    {
                        propOut->id = cpu_to_le32(ID_DRAWPEN_USB_MOUSEDOWN);
                    }
                    else
                    {
                        propOut->id = cpu_to_le32(ID_DRAWPEN_USB_CURVE);
                    }

                    propOut->flags = 0;
                    // propOut->size = cpu_to_le32(sizeof(int)* (MAX_PENCURVE_ARRAY * 2 + 4 + 5));
                    propOut->size = cpu_to_le32(sizeof(int) * (dsize * 2 + 4 + 5));
                    propOut->status = 0;
                    idbSend2HostAsync(propOut, le32_to_cpu(propOut->size), 1);
                }
                // (void)printf("[target 0] %d,%d status %d\n", drawpen->rawdata[0 + 9], drawpen->rawdata[1 + 9], ret);
            }
        }

    }

    return NULL;
}

static void DrawPenOnStop(ITUDrawPen* drawpen)
{
    // DO NOTHING
}

static void pos_surf_draw_circle(ITUSurface *surf, int cx, int cy, int radius, ITUColor* color)
{
    int      x, y, dx, dy;
    int      pLength;
    int      pRadLen = (radius * radius);
    int      LTX, LTY;
    ITUColor fC = {255, color->red, color->green, color->blue};

    for (y = 0; y <= surf->height; y++)
    {
        for (x = 0; x <= surf->width; x++)
        {
            dx = x - cx;
            dy = y - cy;
            if ((x == cx) && ((dy * dy) == pRadLen))
            {
                continue;
            }
            if ((y == cy) && ((dx * dx) == pRadLen))
            {
                continue;
            }
            pLength = (dx * dx) + (dy * dy);
            LTX     = ((x >= cx) ? (x - 1) : (x));
            LTY     = ((y >= cy) ? (y - 1) : (y));
            if (pLength <= pRadLen)
            {
                ituColorFill(surf, LTX, LTY, 1, 1, color);
            }
            else if (pLength <= (pRadLen + 2))
            {
                fC.alpha = 255 / ((pLength - pRadLen) * 2);
                ituColorFill(surf, LTX, LTY, 1, 1, &fC);
            }
            else
            {
                /* do nothing */
            }
        }
    }
}

void blitpen(ITUDrawPen* drawpen, int x, int y, int size)
{
    /*if ((drawpen->lastcolor.alpha != drawpen->pencolor.alpha)
        || (drawpen->lastcolor.red != drawpen->pencolor.red)
        || (drawpen->lastcolor.green != drawpen->pencolor.green)
        || (drawpen->lastcolor.blue != drawpen->pencolor.blue)
        || ((drawpen->pensurf->width != size) || (drawpen->pensurf->height != size))
        )
    {
        if (drawpen->pensurf)
            ituSurfaceRelease(drawpen->pensurf);

        drawpen->pensurf = ituCreateSurface(size, size, 0, ITU_ARGB8888, NULL, 0); //ITU_ARGB8888

        ituColorFill(drawpen->pensurf, 0, 0, size, size, &emptycolor);

        pos_surf_draw_circle(drawpen->pensurf, size / 2, size / 2, size / 2, &drawpen->pencolor);

        drawpen->lastcolor = drawpen->pencolor;
    }*/

    if ((drawpen->pensurf->width != size) || (drawpen->pensurf->height != size))
    {
        drawpen->pensurf = pressure_surf[size];
        (void)printf("pen size is %d now\n", size);
    }

    //ituAlphaBlend(drawpen->bitsurf, x - (size / 2), y - (size / 2), size, size, drawpen->pensurf, 0, 0, 255);
    ituBitBlt(drawpen->bitsurf, x - (size / 2), y - (size / 2), size, size, drawpen->pensurf, 0, 0);
}


void bzc(int px0, int py0, int px1, int py1, int px2, int py2, int px3, int py3, int* X, int* Y, int size)
{
    int i = 0;
    float t = 0.0;
    float step;

    assert(size);

    step = 1.0 / (float)size;

    for (t = 0.0; t <= 1.0; t += step)
    {
        X[i] = (int)((-t*t*t + 3 * t*t - 3 * t + 1) * px0
            + (3 * t*t*t - 6 * t*t + 3 * t) * px1
            + (-3 * t*t*t + 3 * t*t) * px2
            + (t*t*t) * px3);
        Y[i] = (int)((-t*t*t + 3 * t*t - 3 * t + 1) * py0
            + (3 * t*t*t - 6 * t*t + 3 * t) * py1
            + (-3 * t*t*t + 3 * t*t) * py2
            + (t*t*t) * py3);

        //X++;
        //Y++;
        i++;
    }
}

void check_pressure(ITUDrawPen* drawpen)
{
    if (drawpen->pressure < MIN_PEN_PRESSURE) //minimum pen width
    {
        drawpen->pressure = MIN_PEN_PRESSURE;
    }
    else if (drawpen->pressure > MAX_PEN_PRESSURE)
    {
        drawpen->pressure = MAX_PEN_PRESSURE;
    }
    else
    {
        /* do nothing */
    }
}

void vandermonde_curve(int px0, int py0, int px1, int py1, int px2, int py2, int px3, int py3, int* X, int* Y, int size)
{
    int i = 0;
    float t = 0.0;
    float step;
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

    //(void)printf("\n");

    for (t = 0.0; t < 1.0; t += step)
    {
        /*
        if (i == 0)
        {
            X[i] = px0;
            Y[i] = py0;
            i++;
            continue;
        }
        else if (i == (size - 1))
        {
            X[i] = px3;
            Y[i] = py3;
            break;
        }*/

        X[i] = (lround)((pow(1.0 - t, 3.0) * x0) + (3.0 * t * pow(1.0 - t, 2.0) * x1) + (3.0 * pow(t, 2.0) * (1.0 - t) * x2) + (pow(t, 3.0) * x3));

        Y[i] = (lround)((pow(1.0 - t, 3.0) * y0) + (3.0 * t * pow(1.0 - t, 2.0) * y1) + (3.0 * pow(t, 2.0) * (1.0 - t) * y2) + (pow(t, 3.0) * y3));

        /*
        X[i] = (int)(((-4.5 * (t - 0.33333) * (t - 0.66666) * (t - 1.0)) * x0)
            + ((13.5 * t * (t - 0.66666) * (t - 1.0)) * x1)
            + ((-13.5 * t * (t - 0.33333) * (t - 1.0)) * x2)
            + ((4.5 * t * (t - 0.33333) * (t - 0.66666)) * x3));

        Y[i] = (int)(((-4.5 * (t - 0.33333) * (t - 0.66666) * (t - 1.0)) * y0)
            + ((13.5 * t * (t - 0.66666) * (t - 1.0)) * y1)
            + ((-13.5 * t * (t - 0.33333) * (t - 1.0)) * y2)
            + ((4.5 * t * (t - 0.33333) * (t - 0.66666)) * y3));*/

        i++;
    }

    //for (i = 0; i < size; i++)
    //{
    //  (void)printf("T0[%d,%d]T1[%d,%d] [%d,%d]\n", px0, py0, px3, py3, X[i], Y[i]);
    //}
}


bool ituDrawPenUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool result;
    ITUDrawPen* drawpen = (ITUDrawPen*)widget;
    assert(drawpen);

    result = ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_MOUSEDOWN)
    {
        int x = arg2 - widget->rect.x;
        int y = arg3 - widget->rect.y;

        if (ituWidgetIsEnabled(widget) && !(drawpen->drawpenFlags & ITU_DRAWPEN_NOT))
        {
            if (ituWidgetIsInside(widget, x, y))
            {
                if (drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE)
                {
                    arg1 = (drawpen->pressure * 2048) / MAX_PEN_PRESSURE;
                }
                //if (drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE)
                //  drawpen->drawpenFlags &= ~ITU_DRAWPEN_SIM_MODE;

                drawpen->pressure = (int)((arg1 * MAX_PEN_PRESSURE) / 2048.0);
                drawpen->pressure_rawdata = arg1;
                check_pressure(drawpen);

                fifoB_init(&fifoX, 1024, BUFFERX);
                fifoB_init(&fifoY, 1024, BUFFERY);

                fifoB_putPut(&fifoX, x);
                fifoB_putPut(&fifoY, y);

                drawpen->last_pen_x = 10000;
                drawpen->last_pen_y = 10000;
                drawpen->px0 = x;
                drawpen->py0 = y;
                drawpen->px1 = x;
                drawpen->py1 = y;
                drawpen->px2 = x;
                drawpen->py2 = y;
                drawpen->px3 = x;
                drawpen->py3 = y;
                drawpen->pen_x = x;
                drawpen->pen_y = y;

                drawpen->hasmove = 1;

                if ((drawpen->pen_x > drawpen->pressure) && (drawpen->pen_x < (drawpen->surf->width - drawpen->pressure))
                    && (drawpen->pen_y > drawpen->pressure) && (drawpen->pen_y < (drawpen->surf->height - drawpen->pressure)))
                {
                    blitpen(drawpen, drawpen->pen_x, drawpen->pen_y, drawpen->pressure);

                    if (drawpen->usb_working == 1)
                    {
                        if (drawpen->rawdata)
                        {
                            (void)memset(drawpen->rawdata, 0, sizeof(int)* (MAX_PENCURVE_ARRAY * 2 + 4 + 5));

                            drawpen->rawdata[0] = 1;
                            drawpen->rawdata[4] = drawpen->pressure_rawdata;
                            drawpen->rawdata[5] = drawpen->pencolor.alpha;
                            drawpen->rawdata[6] = drawpen->pencolor.red;
                            drawpen->rawdata[7] = drawpen->pencolor.green;
                            drawpen->rawdata[8] = drawpen->pencolor.blue;
                            drawpen->rawdata[9] = drawpen->pen_x;
                            drawpen->rawdata[10] = drawpen->pen_y;
#ifdef DRAWPEN_USB_DEVICE_DEBUG
                            (void)printf("\n");
                            (void)printf("[DrawPen][USB][Mouse Down] %d\n", drawpen->rawdata[0]);
                            (void)printf("[DrawPen][USB][Pressure] %d\n", drawpen->rawdata[4]);
                            (void)printf("[DrawPen][USB][ColorARGB] %d,%d,%d,%d\n", drawpen->rawdata[5], drawpen->rawdata[6], drawpen->rawdata[7], drawpen->rawdata[8]);
                            (void)printf("[DrawPen][USB][x,y] %d,%d\n", drawpen->rawdata[9], drawpen->rawdata[10]);
                            (void)printf("\n");
#endif
                            drawpen->usb_ready_to_rw = 1;
                        }
                    }
                }

                drawpen->drawpenFlags |= ITU_DRAWPEN_WORKING;
                result = true;
                result |= ituExecActions((ITUWidget*)drawpen, drawpen->actions, ev, 0);

                //(void)printf("[DrawPen]PenWidth => %d / %d\n", drawpen->pressure, MAX_PEN_PRESSURE);
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEMOVE)
    {
        int x = arg2 - widget->rect.x;
        int y = arg3 - widget->rect.y;

        if (ituWidgetIsEnabled(widget))
        {
            if (ituWidgetIsInside(widget, x, y))
            {
                //if ((drawpen->drawpenFlags & ITU_DRAWPEN_WORKING) && (drawpen->hasmove <= 1))
                if ((drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
                {
                    int newp = (int)((((float)arg1) * MAX_PEN_PRESSURE) / 2048.0);
                    if (drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE)
                    {
                        newp = drawpen->pressure;
                    }
                    if ((newp != drawpen->pressure) && (newp != 0))
                    {
                        //drawpen->pressure = (int)((arg1 * MAX_PEN_PRESSURE) / 2048.0);
                        drawpen->pressure = newp;
                        drawpen->pressure_rawdata = arg1;
                        check_pressure(drawpen);
                    }
                }

                drawpen->last_pen_x = drawpen->pen_x;
                drawpen->last_pen_y = drawpen->pen_y;
                drawpen->pen_x = x;
                drawpen->pen_y = y;

                //if ((drawpen->hasmove < 4) && (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
                if (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING)
                {
                    fifoB_putPut(&fifoX, x);
                    fifoB_putPut(&fifoY, y);
                    drawpen->hasmove++;

                    drawpen->px0 = drawpen->px1;
                    drawpen->py0 = drawpen->py1;
                    drawpen->px1 = drawpen->px2;
                    drawpen->py1 = drawpen->py2;
                    drawpen->px2 = drawpen->px3;
                    drawpen->py2 = drawpen->py3;
                    drawpen->px3 = x;
                    drawpen->py3 = y;

                    if ((drawpen->hasmove % 4) == 0)
                    {
                        fifoB_putPut(&fifoX, x);
                        fifoB_putPut(&fifoY, y);
                        drawpen->hasmove++;
                    }
                    //(void)printf("mmm %d,%d\n", x, y);
                }
                else
                {
                    drawpen->drawpenFlags |= ITU_DRAWPEN_MOVING;
                }

                //if (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING)
                //result = true;

                //if (drawpen->hasmove >= 4)
                //{
                //  widget->dirty = 1;
                //}
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEUP)
    {
        int x = arg2 - widget->rect.x;
        int y = arg3 - widget->rect.y;

        if (ituWidgetIsEnabled(widget))
        {
            if (ituWidgetIsInside(widget, x, y))
            {
                if ((drawpen->usb_working == 1) && (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
                {
                    int pdata[11] = { 0 };
                    int dsize = 1;
                    struct prop* propOut = (struct prop*)pdata;
                    propOut->id = cpu_to_le32(ID_DRAWPEN_USB_MOUSEUP);
                    propOut->flags = 0;
                    propOut->size = cpu_to_le32(sizeof(int)* (dsize * 2 + 4 + 5));
                    propOut->status = 0;
#ifdef DRAWPEN_USB_DEVICE_DEBUG
                    pdata[4] = 5; //float value?
                    pdata[5] = drawpen->pencolor.alpha;
                    pdata[6] = drawpen->pencolor.red;
                    pdata[7] = drawpen->pencolor.green;
                    pdata[8] = drawpen->pencolor.blue;
                    pdata[9] = x;
                    pdata[10] = y;

                    (void)printf("\n");
                    (void)printf("[DrawPen][USB][Mouse UP] %d\n", dsize);
                    (void)printf("[DrawPen][USB][Pressure] %d\n", pdata[4]);
                    (void)printf("[DrawPen][USB][ColorARGB] %d,%d,%d,%d\n", pdata[5], pdata[6], pdata[7], pdata[8]);
                    (void)printf("[DrawPen][USB][x,y] %d,%d\n", pdata[9], pdata[10]);
                    (void)printf("\n");
#endif
                    idbSend2HostAsync(propOut, le32_to_cpu(propOut->size), 1);
                }

                if (!(drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE))
                {
                    drawpen->drawpenFlags &= ~ITU_DRAWPEN_WORKING;
                }
                //drawpen->pressure = MIN_PEN_PRESSURE;
                //drawpen->hasmove = 0;
            }
        }
    }
    else if (ev == ITU_EVENT_TIMER)
    {
        if (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING)
        {
            result = true;
            widget->dirty = 1;
            if (drawpen->drawpenFlags & ITU_DRAWPEN_DRAWING)
            {
                if (drawpen->hasmove == 0)
                {
                    drawpen->drawpenFlags &= ~ITU_DRAWPEN_DRAWING;
                    ituDrawPenStop(drawpen);
                }
            }
        }
        else if (drawpen->drawpenFlags & ITU_DRAWPEN_MOVING)
        {
            result = true;
            widget->dirty = 1;
        }
        else if (drawpen->drawpenFlags & ITU_DRAWPEN_DRAWING)
        {
            result = true;
            widget->dirty = 1;
        }
        else if (drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE)
        {
            result = true;
            widget->dirty = 1;
        }
        else
        {
            result = false;
            widget->dirty = 0;
        }

        //widget->dirty = 1;
        //result = true;
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        widget->dirty = true;
    }
    else
    {
        /* do nothing */
    }

    return widget->visible ? result : false;
}

void ituDrawPenDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    uint8_t desta;
    ITUDrawPen* drawpen = (ITUDrawPen*)widget;
    const ITURectangle* rect = &widget->rect;
    ITCTree* node;
    assert(drawpen);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255;

    for (node = widget->tree.child; node; node = node->sibling)
    {
        ITUWidget* child = (ITUWidget*)node;

        if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
        {
            ituWidgetDraw(node, dest, destx, desty, desta);
        }

        child->dirty = false;
    }

    check_pressure(drawpen);

    //when drawing first time, backup the screen/pad into drawpen->surf
    if (drawpen->surf == NULL)
    {
        int aX = 0;
        int aY = 0;
        ituWidgetGetGlobalPosition(drawpen, &aX, &aY);
        drawpen->surf = ituCreateSurface(drawpen->widget.rect.width, drawpen->widget.rect.height, 0, ITU_RGB565, NULL, 0);
        ituColorFill(drawpen->surf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, &emptycolor);
        ituBitBlt(drawpen->surf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, drawpen->screensurf, aX, aY);
    }
    else
    {
        int aX = 0;
        int aY = 0;
        ituWidgetGetGlobalPosition(drawpen, &aX, &aY);
        ituColorFill(drawpen->surf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, &emptycolor);
        //ituBitBlt(drawpen->surf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, ituGetDisplaySurface(), drawpen->widget.rect.x, drawpen->widget.rect.y);
        ituBitBlt(drawpen->surf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, ituGetDisplaySurface(), aX, aY);
    }

    if (drawpen->bitsurf == NULL)
    {
        drawpen->bitsurf = ituCreateSurface(drawpen->widget.rect.width, drawpen->widget.rect.height, 0, ITU_ARGB8888, NULL, 0); //ITU_ARGB8888
        if (drawpen->bitsurf)
        {
            ituColorFill(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height, &emptycolor);
        }

#if 0
        ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, drawpen->screensurf, destx, desty);
#endif
    }
    else
    {
#if 0
        ituColorFill(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height, &emptycolor);
        ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height, drawpen->surf, 0, 0);
#endif
    }

    if ((drawpen->screensurf) && (drawpen->bitsurf))
    {
        int pressure = drawpen->pressure;
        int i;
        int dd;
        double dx, dy;
        double dt;
        double diff;
        double Fa;
#ifdef SEND_MAX_CURVE_RAW
        int dd2 = SEND_MAX_CURVE_RAW_FACTOR;
#else
        int dd2 = 1;
#endif

        //if ((drawpen->drawpenFlags & ITU_DRAWPEN_WORKING) && (drawpen->hasmove >= 4))
        if (drawpen->hasmove >= 4)
        {
            fifoB_getall(&fifoX, &fifoY, drawpen);

            if (DRAWPEN_FIT_POINT_MODE)
            {
                diff = sqrt((drawpen->pen_x - drawpen->last_pen_x)*(drawpen->pen_x - drawpen->last_pen_x)
                    + (drawpen->pen_y - drawpen->last_pen_y)*(drawpen->pen_y - drawpen->last_pen_y));
            }
            else
            {
                diff = sqrt((drawpen->px3 - drawpen->px0)*(drawpen->px3 - drawpen->px0)
                    + (drawpen->py3 - drawpen->py0)*(drawpen->py3 - drawpen->py0));
            }

            if (DRAWPEN_FIT_POINT_MODE)
            {
                if ((drawpen->pen_x == drawpen->last_pen_x) || (drawpen->pen_y == drawpen->last_pen_y))
                {
                    Fa = 0.0;
                }
                else
                {
                    Fa = fabs((double)(drawpen->pen_y - drawpen->last_pen_y) /
                              (double)(drawpen->pen_x - drawpen->last_pen_x));
                }
            }


            //if ((drawpen->pen_x > pressure) && (drawpen->pen_x < (drawpen->surf->width - pressure))
            //  && (drawpen->pen_y > pressure) && (drawpen->pen_y < (drawpen->surf->height - pressure)))
            if (1)
            {
                dd = (int)(diff / ((double)pressure / (double)SMOOTH_FACTOR));

                if (DRAWPEN_FIT_POINT_MODE) //fit point mode
                {
                    if ((diff > pressure) && (drawpen->last_pen_x < 10000) && (drawpen->last_pen_y < 10000))
                    {
                        if (Fa != 0.0)
                        {
                            for (i = 0; i < dd; i++)
                            {
                                dt = (double)(pressure / SMOOTH_FACTOR) * (double)(i + 0.5);

                                dx = dt / sqrt(1 + Fa * Fa);
                                dy = dx * Fa;

                                dx *= (drawpen->pen_x >= drawpen->last_pen_x) ? (1) : (-1);
                                dy *= (drawpen->pen_y >= drawpen->last_pen_y) ? (1) : (-1);

                                //pos_surf_draw_circle(drawpen->surf, drawpen->last_pen_x + (int)dx, drawpen->last_pen_y + (int)dy, radious, &drawpen->pencolor);
                                blitpen(drawpen, drawpen->last_pen_x + (int)dx, drawpen->last_pen_y + (int)dy, pressure);
                            }
                        }
                        else
                        {
                            for (i = 0; i < dd; i++)
                            {
                                dt = (double)(pressure / SMOOTH_FACTOR) * (double)(i + 0.5);

                                if (drawpen->pen_x == drawpen->last_pen_x)
                                {
                                    dx = 0.0;
                                    dy = dt * ((drawpen->pen_y >= drawpen->last_pen_y) ? (1) : (-1));
                                }
                                else
                                {
                                    dy = 0.0;
                                    dx = dt * ((drawpen->pen_x >= drawpen->last_pen_x) ? (1) : (-1));
                                }

                                //pos_surf_draw_circle(drawpen->surf, drawpen->last_pen_x + (int)dx, drawpen->last_pen_y + (int)dy, radious, &drawpen->pencolor);
                                blitpen(drawpen, drawpen->last_pen_x + (int)dx, drawpen->last_pen_y + (int)dy, pressure);
                            }
                        }
                    }
                    else
                    {
                        //pos_surf_draw_circle(drawpen->surf, drawpen->pen_x, drawpen->pen_y, radious, &drawpen->pencolor);
                        blitpen(drawpen, drawpen->pen_x, drawpen->pen_y, pressure);
                    }
                }
                else if (drawpen->hasmove >= 4) //fit curve mode
                {
                    //fifoB_getall(&fifoX, &fifoY, drawpen);
                    drawpen->hasmove -= 4;
                    //(void)printf("[CHECK %d] P1(%d,%d) P2(%d,%d)\n", drawpen->hasmove, drawpen->px0, drawpen->py0, drawpen->px3, drawpen->py3);

                    if (dd < 4)
                    {
                        dd = 4;
                    }
                    else if (dd >= MAX_PENCURVE_ARRAY)
                    {
                        dd = MAX_PENCURVE_ARRAY - 1;
                        (void)printf("[DrawPen] MAX_PENCURVE_ARRAY %d limited!\n", MAX_PENCURVE_ARRAY);
                    }


#ifdef SEND_MAX_CURVE_RAW
                    (void)memset(ARRAYX2, 0, MAX_PENCURVE_ARRAY * SEND_MAX_CURVE_RAW_FACTOR);
                    (void)memset(ARRAYY2, 0, MAX_PENCURVE_ARRAY * SEND_MAX_CURVE_RAW_FACTOR);
                    vandermonde_curve(drawpen->px0, drawpen->py0, drawpen->px1, drawpen->py1, drawpen->px2, drawpen->py2, drawpen->px3, drawpen->py3, &ARRAYX2[0], &ARRAYY2[0], dd * SEND_MAX_CURVE_RAW_FACTOR);
#else
                    (void)memset(ARRAYX, 0, MAX_PENCURVE_ARRAY * sizeof(int));
                    (void)memset(ARRAYY, 0, MAX_PENCURVE_ARRAY * sizeof(int));
                    vandermonde_curve(drawpen->px0, drawpen->py0, drawpen->px1, drawpen->py1, drawpen->px2, drawpen->py2, drawpen->px3, drawpen->py3, &ARRAYX[0], &ARRAYY[0], dd);
#endif

                    //bzc(drawpen->px0, drawpen->py0, drawpen->px1, drawpen->py1, drawpen->px2, drawpen->py2, drawpen->px3, drawpen->py3, X, Y, dd);

                    /*(void)printf("\n\n=====p1,p2,p3,p4=====\n");
                    (void)printf("[%d,%d]\n", drawpen->px0, drawpen->py0);
                    (void)printf("[%d,%d]\n", drawpen->px1, drawpen->py1);
                    (void)printf("[%d,%d]\n", drawpen->px2, drawpen->py2);
                    (void)printf("[%d,%d]\n", drawpen->px3, drawpen->py3);*/

                    //drawpen->px3 = drawpen->pen_x;
                    //drawpen->py3 = drawpen->pen_y;

                    for (i = 0; i < (dd * dd2); i++)
                    {
                        //pos_surf_draw_circle(drawpen->surf, X[i], Y[i], radious, &drawpen->pencolor);
#ifdef SEND_MAX_CURVE_RAW
                        if (((i + 1) >= (dd * dd2)) || ((i % SEND_MAX_CURVE_RAW_FACTOR) == 0))
                        {
                            blitpen(drawpen, ARRAYX2[i], ARRAYY2[i], pressure);
                        }
#else
                        blitpen(drawpen, ARRAYX[i], ARRAYY[i], pressure);
                        //(void)printf("[%d, %d]\n", ARRAYX[i], ARRAYY[i]);
#endif

                        if ((drawpen->usb_working == 1) && drawpen->rawdata)
                        {
                            if (i == 0)
                            {
                                (void)memset(drawpen->rawdata, 0, sizeof(int)* (MAX_PENCURVE_ARRAY * 2 * dd2 + 4 + 5));

                                drawpen->rawdata[0] = dd * dd2;

                                drawpen->rawdata[4] = drawpen->pressure_rawdata;
                                drawpen->rawdata[5] = drawpen->pencolor.alpha;
                                drawpen->rawdata[6] = drawpen->pencolor.red;
                                drawpen->rawdata[7] = drawpen->pencolor.green;
                                drawpen->rawdata[8] = drawpen->pencolor.blue;
                            }
#ifdef SEND_MAX_CURVE_RAW
                            drawpen->rawdata[i * 2 + 9] = ARRAYX2[i];
                            drawpen->rawdata[(i * 2) + 9 + 1] = ARRAYY2[i];
#else
                            drawpen->rawdata[i * 2 + 9] = ARRAYX[i];
                            drawpen->rawdata[(i * 2) + 9 + 1] = ARRAYY[i];
#endif
                        }

                        if ((i == (dd * dd2 - 1)) && (drawpen->usb_working == 1))
                        {
#ifdef DRAWPEN_USB_DEVICE_DEBUG
                            (void)printf("\n");
                            (void)printf("[DrawPen][USB][ARR size] %d\n", drawpen->rawdata[0]);
                            (void)printf("[DrawPen][USB][Pressure] %d\n", drawpen->rawdata[4]);
                            (void)printf("[DrawPen][USB][ColorARGB] %d,%d,%d,%d\n", drawpen->rawdata[5], drawpen->rawdata[6], drawpen->rawdata[7], drawpen->rawdata[8]);
                            (void)printf("[DrawPen][USB][P1] %d,%d\n", drawpen->rawdata[9], drawpen->rawdata[10]);
                            (void)printf("[DrawPen][USB][P2] %d,%d\n", drawpen->rawdata[11], drawpen->rawdata[12]);
                            (void)printf("[DrawPen][USB][P3] %d,%d\n", drawpen->rawdata[13], drawpen->rawdata[14]);
                            (void)printf("\n");
#endif
                            drawpen->usb_ready_to_rw = 1;

                            //sem_post(&sem);
                        }
                    }

                    //(void)printf("[dd][%d]\n", dd);
                }
                else
                {
                    //pos_surf_draw_circle(drawpen->surf, drawpen->pen_x, drawpen->pen_y, radious, &drawpen->pencolor);
                    //blitpen(drawpen, drawpen->pen_x, drawpen->pen_y, pressure);
                }
            }
        }

#if 0
        // main
        ituBitBlt(drawpen->surf, 0, 0, drawpen->surf->width, drawpen->surf->height, drawpen->bitsurf, 0, 0);

        // modify for draw background first
        ituBitBlt(drawpen->screensurf, destx, desty, drawpen->bitsurf->width, drawpen->bitsurf->height, drawpen->bitsurf, 0, 0);
#endif

        ituAlphaBlend(drawpen->surf, 0, 0, drawpen->surf->width, drawpen->surf->height, drawpen->bitsurf, 0, 0, 255);
#if 0
        ituBitBlt(drawpen->surf, 0, 0, drawpen->surf->width, drawpen->surf->height, drawpen->bitsurf, 0, 0);
#endif

        if (drawpen->drawpenFlags & ITU_DRAWPEN_ONESHOT_MODE)
        {
            if (!(drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
            {
                ituBitBlt(drawpen->screensurf, destx, desty, drawpen->surf->width, drawpen->surf->height, drawpen->surf,
                          0, 0);
            }
        }
        else
        {
            ituBitBlt(drawpen->screensurf, destx, desty, drawpen->surf->width, drawpen->surf->height, drawpen->surf, 0,
                      0);
        }

        if (drawpen->usb_working == 1 && (drawpen->usb_ready_to_rw == 1))
        {
            drawpen->usb_ready_to_rw = 0;
            sem_post(&sem);
        }

        if (!(drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
        {
            if ((drawpen->hasmove > 0) && (drawpen->hasmove < 4))
            {
                drawpen->hasmove = 0;
            }

            ituAlphaBlend(drawpen->screensurf, drawpen->pen_x + destx - PENCURSOR_HEADSIZE / 2, drawpen->pen_y + desty - PENCURSOR_HEADSIZE / 2, PENCURSOR_HEADSIZE, PENCURSOR_HEADSIZE, drawpen->cursorsurf, 0, 0, 128);
        }
        else if (drawpen->hasmove < 4)
        {
            if (drawpen->drawpenFlags & ITU_DRAWPEN_DRAWING)
            {
                if (drawpen->hasmove > 0)
                {
                    int checkVarX = (drawpen->px0 + drawpen->px1 + drawpen->px2 + drawpen->px3) / 4;
                    int checkVarY = (drawpen->py0 + drawpen->py1 + drawpen->py2 + drawpen->py3) / 4;
                    bool checkArrX = ((ARRAYX[4] + ARRAYX[5] + ARRAYX[6] + ARRAYX[7]) == 0);
                    bool checkArrY = ((ARRAYY[4] + ARRAYY[5] + ARRAYY[6] + ARRAYY[7]) == 0);
                    if ((drawpen->drawpenFlags & ITU_DRAWPEN_SIM_MODE) && (checkVarX == drawpen->pen_x) && (checkVarY == drawpen->pen_y) && checkArrX && checkArrY)
                    {
                        (void)memset(ARRAYX, 0, MAX_PENCURVE_ARRAY * sizeof(int));
                        (void)memset(ARRAYY, 0, MAX_PENCURVE_ARRAY * sizeof(int));
                        drawpen->hasmove = 0;
                        drawpen->drawpenFlags &= ~ITU_DRAWPEN_SIM_MODE;
                        drawpen->drawpenFlags &= ~ITU_DRAWPEN_DRAWING;
                        drawpen->drawpenFlags &= ~ITU_DRAWPEN_WORKING;
                    }
                    else
                    {
                        //ituDrawPenDrawPoint(drawpen, drawpen->pressure, drawpen->pen_x + widget->rect.x, drawpen->pen_y + widget->rect.y);
                        ituDrawPenDrawPoint(drawpen, drawpen->pressure, drawpen->pen_x, drawpen->pen_y);
                        //drawpen->hasmove++;
                    }
                }
            }
            widget->dirty = 1;
        }
        else
        {
            /* do nothing */
        }

        if (!(drawpen->drawpenFlags & ITU_DRAWPEN_INIT_DONE))
        {
            drawpen->drawpenFlags |= ITU_DRAWPEN_INIT_DONE;
        }
    }

}

void ituDrawPenOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUDrawPen* drawpen = (ITUDrawPen*)widget;
    assert(drawpen);

    switch (action)
    {
        case ITU_ACTION_PLAY:
            if (widget->flags & ITU_ENABLED)
            {
                ituDrawPenPlay(drawpen);
            }
            break;

        case ITU_ACTION_BIND:
            if (widget->flags & ITU_ENABLED)
            { // ColorPicker use BIND to change the color of DrawPen
                if (param)
                {
                    unsigned int value      = atoi(param);

                    drawpen->pencolor.alpha = (value & 0xFF000000) >> 24;
                    drawpen->pencolor.red   = (value & 0x00FF0000) >> 16;
                    drawpen->pencolor.green = (value & 0x0000FF00) >> 8;
                    drawpen->pencolor.blue  = (value & 0x000000FF);

                    (void)printf("[DrawPen] %s Color[A,R,G,B][%d,%d,%d,%d]\n", widget->name, drawpen->pencolor.alpha,
                           drawpen->pencolor.red, drawpen->pencolor.green, drawpen->pencolor.blue);
                }
            }
            break;

        case ITU_ACTION_GOTO:
            if (widget->flags & ITU_ENABLED)
            {
                if (param)
                {
                    int pressure = atoi(param);

                    if (pressure >= 0)
                    {
                        drawpen->pressure = pressure;
                    }
                }
            }
            break;

        case ITU_ACTION_STOP:
            if (widget->flags & ITU_ENABLED)
            {
                ituDrawPenStop(drawpen);
                ituDrawPenOnStop(drawpen);
                ituExecActions(widget, drawpen->actions, ITU_EVENT_STOPPED, 0);
            }
            break;

        case ITU_ACTION_CLEAR:
            if (widget->flags & ITU_ENABLED)
            {
                ituDrawPenStop(drawpen);
                ituDrawPenOnStop(drawpen);

                if (drawpen->surf)
                {
                    if (drawpen->bitsurf)
                    {
                        ituSurfaceRelease(drawpen->bitsurf);
                        drawpen->bitsurf = NULL;
                    }

                    if (drawpen->screensurf == NULL)
                    {
                        drawpen->screensurf = ituGetDisplaySurface();
                    }

                    if (drawpen->screensurf)
                    {
                        drawpen->bitsurf = ituCreateSurface(drawpen->widget.rect.width, drawpen->widget.rect.height, 0,
                                                            ITU_ARGB8888, NULL, 0); // ITU_ARGB8888
                        if (drawpen->bitsurf)
                        {
                            ituColorFill(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height,
                                         &emptycolor);
                        }

                        ////ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height,
                        ///drawpen->surf, 0, 0);

                        // ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height,
                        // drawpen->screensurf, drawpen->widget.rect.x, drawpen->widget.rect.y);
                        ituBackgroundDraw(widget, drawpen->screensurf, 0, 0, 255);
                    }

                    (void)printf("[DrawPen] %s clear!\n", widget->name);
                }
            }
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

void ituDrawPenInit(ITUDrawPen* drawpen)
{
    assert(drawpen);
    ITU_ASSERT_THREAD();

    (void)memset(drawpen, 0, sizeof (ITUDrawPen));

    ituWidgetInit(&drawpen->widget);

    ituWidgetSetType(drawpen, ITU_DRAWPEN);
    ituWidgetSetName(drawpen, drawpenName);
    ituWidgetSetUpdate(drawpen, ituDrawPenUpdate);
    ituWidgetSetDraw(drawpen, ituDrawPenDraw);
    ituWidgetSetOnAction(drawpen, ituDrawPenOnAction);
    ituDrawPenSetOnStop(drawpen, DrawPenOnStop);
}

void ituDrawPenLoad(ITUDrawPen* drawpen, uint32_t base)
{
    assert(drawpen);

    ituWidgetLoad((ITUWidget*)drawpen, base);
    ituWidgetSetUpdate(drawpen, ituDrawPenUpdate);
    ituWidgetSetDraw(drawpen, ituDrawPenDraw);
    ituWidgetSetOnAction(drawpen, ituDrawPenOnAction);
    ituDrawPenSetOnStop(drawpen, DrawPenOnStop);

    emptycolor.alpha = 0;
    emptycolor.red = 0;
    emptycolor.green = 0;
    emptycolor.blue = 0;

    if (drawpen->screensurf == NULL)
    {
        if (!drawpen->rawdata)
        {
            //int *newdata = malloc(sizeof(int)* (MAX_PENCURVE_ARRAY * 2 + 4 + 5));
#ifdef SEND_MAX_CURVE_RAW
            (void)memset(ARRAYDATA, 0, sizeof(int)* (MAX_PENCURVE_ARRAY * 2 * SEND_MAX_CURVE_RAW_FACTOR + 4 + 5));
#else
            (void)memset(ARRAYDATA, 0, sizeof(int)* (MAX_PENCURVE_ARRAY * 2 + 4 + 5));
#endif
            drawpen->rawdata = &ARRAYDATA[0];
        }

        drawpen->screensurf = ituGetDisplaySurface();

        drawpen->last_pen_x = 10000;
        drawpen->last_pen_y = 10000;
        drawpen->px0 = 10000;       drawpen->py0 = 10000;
        drawpen->px1 = 10000;       drawpen->py1 = 10000;
        drawpen->px2 = 10000;       drawpen->py2 = 10000;
        drawpen->px3 = 10000;       drawpen->py3 = 10000;
    }

    if (drawpen->screensurf)
    {
        if (drawpen->pensurf == NULL)
        {
            //drawpen->pensurf = ituCreateSurface(100, 100, 0, ITU_ARGB8888, NULL, 0);
            //ituColorFill(drawpen->pensurf, 0, 0, 100, 100, &emptycolor);

            int i = 0;

            for (i = MIN_PEN_PRESSURE; i <= MAX_PEN_PRESSURE; i++)
            {
                pressure_surf[i] = ituCreateSurface(i, i, 0, ITU_ARGB8888, NULL, 0); //ITU_ARGB8888

                ituColorFill(pressure_surf[i], 0, 0, i, i, &emptycolor);

                pos_surf_draw_circle(pressure_surf[i], i / 2, i / 2, i / 2, &drawpen->pencolor);
            }

            drawpen->pensurf = pressure_surf[MIN_PEN_PRESSURE];

            drawpen->lastcolor = drawpen->pencolor;
        }

        if (drawpen->cursorsurf == NULL)
        {
            drawpen->cursorsurf = ituCreateSurface(PENCURSOR_HEADSIZE, PENCURSOR_HEADSIZE, 0, ITU_ARGB8888, NULL, 0);
            ituColorFill(drawpen->cursorsurf, 0, 0, PENCURSOR_HEADSIZE, PENCURSOR_HEADSIZE, &drawpen->pencolor);
        }
    }
}

void ituDrawPenPlay(ITUDrawPen* drawpen)
{
    assert(drawpen);
    ITU_ASSERT_THREAD();

    if ((drawpen->widget.flags & ITU_ENABLED) == 0)
    {
        return;
    }

    drawpen->drawpenFlags |= ITU_DRAWPEN_WORKING;
}

void ituDrawPenStop(ITUDrawPen* drawpen)
{
    assert(drawpen);
    ITU_ASSERT_THREAD();

    if ((drawpen->widget.flags & ITU_ENABLED) == 0)
    {
        return;
    }

    drawpen->drawpenFlags &= ~ITU_DRAWPEN_WORKING;
    drawpen->drawpenFlags &= ~ITU_DRAWPEN_SIM_MODE;

    drawpen->pressure = MIN_PEN_PRESSURE;
    drawpen->hasmove = 0;

    if (drawpen->bitsurf)
    {
        ituSurfaceRelease(drawpen->bitsurf);
        drawpen->bitsurf = NULL;
        drawpen->widget.dirty = 1;
    }

    //ituScene->playingAudio = NULL;
}

void ituDrawPenClear(ITUDrawPen* drawpen)
{
    ITUWidget* widget = (ITUWidget*)drawpen;
    if (widget)
    {
        if (widget->flags & ITU_ENABLED)
        {
            ituDrawPenStop(drawpen);
            ituDrawPenOnStop(drawpen);

            if (drawpen->surf)
            {
                if (drawpen->bitsurf)
                {
                    ituSurfaceRelease(drawpen->bitsurf);
                    drawpen->bitsurf = NULL;
                }

                if (drawpen->screensurf == NULL)
                {
                    drawpen->screensurf = ituGetDisplaySurface();
                }

                if (drawpen->screensurf)
                {
                    drawpen->bitsurf = ituCreateSurface(drawpen->widget.rect.width, drawpen->widget.rect.height, 0, ITU_ARGB8888, NULL, 0); //ITU_ARGB8888
                    ituColorFill(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height, &emptycolor);
                    //ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->bitsurf->width, drawpen->bitsurf->height, drawpen->surf, 0, 0);
                    //ituBitBlt(drawpen->bitsurf, 0, 0, drawpen->widget.rect.width, drawpen->widget.rect.height, drawpen->screensurf, drawpen->widget.rect.x, drawpen->widget.rect.y);
                    ituBackgroundDraw(widget, drawpen->screensurf, 0, 0, 255);
                }

                (void)printf("[DrawPen] %s clear!\n", widget->name);
            }
        }
    }
}

bool ituDrawPenDumpJpeg(ITUDrawPen* drawpen, int percent, char* filename)
{
    bool result = false;
    assert(drawpen);
    ITU_ASSERT_THREAD();

    if ((filename != NULL) && (drawpen->surf != NULL))
    {
        FILE* outfile = fopen(filename, "wb");

        if (outfile != NULL)
        {
            fclose(outfile);
            result = true;
        }
    }

    return result;
}

void ituDrawPenSetPenColor(ITUDrawPen* drawpen, ITUColor color)
{
    int i = 0;

    assert(drawpen);
    ITU_ASSERT_THREAD();

    drawpen->pencolor.alpha = color.alpha;
    drawpen->pencolor.red = color.red;
    drawpen->pencolor.green = color.green;
    drawpen->pencolor.blue = color.blue;

    for (i = MIN_PEN_PRESSURE; i <= MAX_PEN_PRESSURE; i++)
    {
        if (pressure_surf[i])
        {
            ituSurfaceRelease(pressure_surf[i]);
        }

        pressure_surf[i] = ituCreateSurface(i, i, 0, ITU_ARGB8888, NULL, 0);

        ituColorFill(pressure_surf[i], 0, 0, i, i, &emptycolor);

        pos_surf_draw_circle(pressure_surf[i], i / 2, i / 2, i / 2, &drawpen->pencolor);
        drawpen->pensurf = pressure_surf[drawpen->pressure];
    }
}

void ituDrawPenUSBTask(ITUDrawPen* drawpen, bool usbtask)
{
    if (!usbtask && (drawpen->usb_working == 1))
    {
        //sem_t * sem = (sem_t *)drawpen->usb_lock;
        drawpen->usb_working = 0;
        fsg_connected = 0;
        sem_post(&sem);
        pthread_join(task, NULL);
    }
    else if (usbtask && (drawpen->usb_working == 0))
    {
        //pthread_mutex_t usbjob;
        //pthread_t task;
        //sem_t sem;

        pthread_attr_t attr;

        drawpen->usb_working = 1;
        sem_init(&sem, 0, 0);
        //drawpen->usb_lock = (unsigned int *)(&sem);

        if (ioctl(ITP_DEVICE_USBDFSG, ITP_IOCTL_IS_CONNECTED, NULL))
        {
            (void)printf("\n\nusb fsg enter.... \n");
            fsg_connected = 1;
            //idbSetCallback(idbCallback);
        }
        else
        {
            fsg_connected = 0;
            drawpen->usb_working = 0;
        }

        if ((fsg_connected == 1) || (drawpen->usb_working == 1))
        {
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, sizeof(int)* MAX_PENCURVE_ARRAY * 2 * 2);
            pthread_create(&task, &attr, USB_SEND_TASK, drawpen);
            //drawpen->usb_thread = (unsigned long *)(&task);

            (void)printf("[USB]Job start!\n");
        }
        else
        {
            (void)printf("[USB]Connect fail!\n");
        }
    }
}

void ituDrawPenUSBSendID(ITUDrawPen* drawpen, unsigned int id)
{
    assert(drawpen);
    ITU_ASSERT_THREAD();

    if (fsg_connected == 1)
    {
        int pdata[4] = { 0 };
        struct prop* propOut = (struct prop*)pdata;
        propOut->id = cpu_to_le32(id);
        propOut->flags = 0;
        propOut->size = cpu_to_le32(sizeof(int) * 4);
        propOut->status = 0;
        idbSend2HostAsync(propOut, le32_to_cpu(propOut->size), 1);
#ifdef DRAWPEN_USB_DEVICE_DEBUG
        (void)printf("[DrawPen][USB][SendID][0x%8X]\n", id);
#endif
    }
}

void ituDrawPenCursorSwitch(ITUDrawPen* drawpen, bool switchon)
{
    ITU_ASSERT_THREAD();

    if ((drawpen) && (drawpen->cursorsurf))
    {
        if (switchon)
        {
            ituColorFill(drawpen->cursorsurf, 0, 0, PENCURSOR_HEADSIZE, PENCURSOR_HEADSIZE, &drawpen->pencolor);
        }
        else
        {
            ituColorFill(drawpen->cursorsurf, 0, 0, PENCURSOR_HEADSIZE, PENCURSOR_HEADSIZE, &emptycolor);
        }
    }
}

void ituDrawPenTouchSwitch(ITUDrawPen* drawpen, bool touch)
{
    ITU_ASSERT_THREAD();

    if (drawpen)
    {
        if (!touch)
        {
            drawpen->drawpenFlags |= ITU_DRAWPEN_NOT;
        }
        else
        {
            drawpen->drawpenFlags &= ~ITU_DRAWPEN_NOT;
        }
    }
}

void ituDrawPenOneShotSwitch(ITUDrawPen* drawpen, bool oneshot)
{
    ITU_ASSERT_THREAD();

    if (drawpen)
    {
        if (oneshot)
        {
            drawpen->drawpenFlags |= ITU_DRAWPEN_ONESHOT_MODE;
        }
        else
        {
            drawpen->drawpenFlags &= ~ITU_DRAWPEN_ONESHOT_MODE;
        }
    }
}

bool ituDrawPenInitDone(const ITUDrawPen* drawpen)
{
    if (drawpen)
    {
        if (drawpen->drawpenFlags & ITU_DRAWPEN_INIT_DONE)
        {
            return true;
        }
    }
    return false;
}

bool ituDrawPenIsWorking(const ITUDrawPen* drawpen)
{
    if (drawpen)
    {
        if (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING)
        {
            return true;
        }
    }
    return false;
}

void ituDrawPenDrawPoint(ITUDrawPen* drawpen, int width, int x, int y)
{
    int newp = 0;
    if (width <= 0)
    {
        return;
    }
    else
    {
        if (width < MIN_PEN_PRESSURE)
        {
            newp = MIN_PEN_PRESSURE;
        }
        else if (width > MAX_PEN_PRESSURE)
        {
            newp = MAX_PEN_PRESSURE;
        }
        else
        {
            newp = width;
        }
    }

    drawpen->drawpenFlags |= ITU_DRAWPEN_SIM_MODE;
    drawpen->pressure = newp;
    check_pressure(drawpen);

    if (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING)
    {
        ITUWidget* widget = (ITUWidget*)drawpen;

        if (ituWidgetIsEnabled(widget))
        {
            int vX = x;
            int vY = y;
            if (ituWidgetIsInside(widget, vX, vY))
            {
                drawpen->last_pen_x = drawpen->pen_x;
                drawpen->last_pen_y = drawpen->pen_y;
                drawpen->pen_x = vX;
                drawpen->pen_y = vY;

                //if ((drawpen->hasmove < 4) && (drawpen->drawpenFlags & ITU_DRAWPEN_WORKING))
                if (drawpen->drawpenFlags & ITU_DRAWPEN_DRAWING)
                {
                    //important
                    drawpen->hasmove = 4;

                    fifoB_putPut(&fifoX, drawpen->last_pen_x);
                    fifoB_putPut(&fifoY, drawpen->last_pen_y);
                    fifoB_putPut(&fifoX, vX);
                    fifoB_putPut(&fifoY, vY);
                    fifoB_putPut(&fifoX, vX);
                    fifoB_putPut(&fifoY, vY);
                    fifoB_putPut(&fifoX, vX);
                    fifoB_putPut(&fifoY, vY);

                    //if ((drawpen->hasmove % 4) == 0)
                        drawpen->hasmove++;
                }
            }
        }//end
        drawpen->drawpenFlags |= ITU_DRAWPEN_DRAWING;
        drawpen->drawpenFlags |= ITU_DRAWPEN_MOVING;
    }
    else
    {
        ITUWidget* widget = (ITUWidget*)drawpen;

        if (ituWidgetIsEnabled(widget))
        {
            int vX = x;
            int vY = y;
            if (ituWidgetIsInside(widget, vX, vY))
            {
                fifoB_init(&fifoX, 1024, BUFFERX);
                fifoB_init(&fifoY, 1024, BUFFERY);

                fifoB_putPut(&fifoX, vX);
                fifoB_putPut(&fifoY, vY);

                drawpen->pen_x = vX;
                drawpen->pen_y = vY;
                drawpen->last_pen_x = drawpen->pen_x;
                drawpen->last_pen_y = drawpen->pen_y;


                drawpen->px0 = vX;
                drawpen->py0 = vY;
                drawpen->px1 = vX;
                drawpen->py1 = vY;
                drawpen->px2 = vX;
                drawpen->py2 = vY;
                drawpen->px3 = vX;
                drawpen->py3 = vY;
                drawpen->pen_x = vX;
                drawpen->pen_y = vY;

                drawpen->hasmove = 0;

                if ((drawpen->pen_x > drawpen->pressure) && (drawpen->pen_x < (drawpen->surf->width - drawpen->pressure))
                    && (drawpen->pen_y > drawpen->pressure) && (drawpen->pen_y < (drawpen->surf->height - drawpen->pressure)))
                {
                    blitpen(drawpen, drawpen->pen_x, drawpen->pen_y, drawpen->pressure);

                    if (drawpen->usb_working == 1)
                    {
                        if (drawpen->rawdata)
                        {
                            (void)memset(drawpen->rawdata, 0, sizeof(int)* (MAX_PENCURVE_ARRAY * 2 + 4 + 5));

                            drawpen->rawdata[0] = 1;
                            drawpen->rawdata[4] = drawpen->pressure_rawdata;
                            drawpen->rawdata[5] = drawpen->pencolor.alpha;
                            drawpen->rawdata[6] = drawpen->pencolor.red;
                            drawpen->rawdata[7] = drawpen->pencolor.green;
                            drawpen->rawdata[8] = drawpen->pencolor.blue;
                            drawpen->rawdata[9] = drawpen->pen_x;
                            drawpen->rawdata[10] = drawpen->pen_y;
#ifdef DRAWPEN_USB_DEVICE_DEBUG
                            (void)printf("\n");
                            (void)printf("[DrawPen][USB][Mouse Down] %d\n", drawpen->rawdata[0]);
                            (void)printf("[DrawPen][USB][Pressure] %d\n", drawpen->rawdata[4]);
                            (void)printf("[DrawPen][USB][ColorARGB] %d,%d,%d,%d\n", drawpen->rawdata[5], drawpen->rawdata[6], drawpen->rawdata[7], drawpen->rawdata[8]);
                            (void)printf("[DrawPen][USB][x,y] %d,%d\n", drawpen->rawdata[9], drawpen->rawdata[10]);
                            (void)printf("\n");
#endif
                            drawpen->usb_ready_to_rw = 1;
                        }
                    }
                }

                drawpen->drawpenFlags |= ITU_DRAWPEN_DRAWING;
                drawpen->drawpenFlags |= ITU_DRAWPEN_WORKING;
            }
        }//end
    }
}

void ituDrawPenDrawLine(ITUDrawPen* drawpen, int width, int x1, int y1, int x2, int y2)
{
    if (drawpen)
    {
        int Lw = width * 1;
        int dX = abs(x1 - x2);
        int dY = abs(y1 - y2);
        int lineLength = (int)sqrtf((float)(dX*dX) + (float)(dY*dY));
        ITUSurface* emptySurf = NULL;
        ITUColor emptyColor = { 0, 0, 0, 0 };
        float angle = 0.0f;

        if ((lineLength % 2) == 1)
        {
            lineLength += 1;
        }
        emptySurf = ituCreateSurface(lineLength + Lw, lineLength + Lw, 0, ITU_ARGB8888, 0, 0);
        if (emptySurf)
        {
            ituColorFill(emptySurf, 0, 0, emptySurf->width, emptySurf->height, &emptyColor);
        }

        if ((x1 < x2) && (y1 >= y2))
        {
            float rad = asinf((float)(x2 - x1) / (float)lineLength);
            angle = 90.0f + (rad * 180.0f / 3.14159265f);
        }
        else if ((x1 < x2) && (y1 < y2))
        {
            float rad = asinf((float)(x2 - x1) / (float)lineLength);
            angle = 90.0f - (rad * 180.0f / 3.14159265f);
        }
        else if ((x1 > x2) && (y1 > y2))
        {
            float rad = asinf((float)(x1 - x2) / (float)lineLength);
            angle = 90.0f - (rad * 180.0f / 3.14159265f);
        }
        else
        {
            float rad = asinf((float)(x1 - x2) / (float)lineLength);
            angle = 90.0f + (rad * 180.0f / 3.14159265f);
        }

        if (drawpen->bitsurf && drawpen->pensurf && emptySurf)
        {
            int lineLX = (emptySurf->width - lineLength) / 2;
            int lineRX = lineLX + lineLength;
            int lineY = (emptySurf->height - Lw) / 2;
#ifdef CFG_M2D_ENABLE
            int minBX = ((x1 < x2) ? (x1) : (x2));
            int minBY = ((y1 < y2) ? (y1) : (y2));
#endif
            if ((drawpen->pensurf->width != width) || (drawpen->pensurf->height != width))
            {
                drawpen->pensurf = pressure_surf[width];
                drawpen->pressure = width;
            }

            ituColorFill(emptySurf, lineLX, lineY, lineLength, Lw, &drawpen->pencolor);
            ituBitBlt(emptySurf, lineLX - (Lw / 2) + 1, lineY, Lw, Lw, drawpen->pensurf, 0, 0);
            ituBitBlt(emptySurf, lineRX - (Lw / 2) - 1, lineY, Lw, Lw, drawpen->pensurf, 0, 0);
#ifndef CFG_M2D_ENABLE
            ituRotate(drawpen->bitsurf, ((x1 + x2) / 2) + 1, ((y1 + y2) / 2), emptySurf, emptySurf->width / 2, emptySurf->height / 2, angle, 1.0f, 1.0f);
#else
            ituRotate(drawpen->bitsurf, minBX + 1, minBY, emptySurf, emptySurf->width / 2, emptySurf->height / 2, angle, 1.0f, 1.0f);
#endif
        }
        if (emptySurf)
        {
            ituSurfaceRelease(emptySurf);
        }
    }
}
