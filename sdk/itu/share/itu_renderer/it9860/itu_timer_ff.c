#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include "ite/itp.h"
#include "ite/itv.h"
#include "ite/itu.h"

#if defined(CFG_M2D_ENABLE)
    #include "gfx/gfx.h"
#endif

#define MAX_UI_BUF_CNT     3


static ITUSurface      *gITUSurface[MAX_UI_BUF_CNT];
static uint32_t        g_ui_buff_addr[MAX_UI_BUF_CNT];
static char            g_timer_buff[32] = {0};
static bool            g_timer_ff_init;
static pthread_mutex_t TIMER_FF_MUTEX = PTHREAD_MUTEX_INITIALIZER;


static void _get_rtc_time(void)
{
    struct timeval tv;
    struct tm* timeinfo;
    gettimeofday(&tv, NULL);
    timeinfo = localtime((const time_t*)&tv.tv_sec);
    strftime(g_timer_buff, sizeof(g_timer_buff), "%Y/%m/%d %H:%M:%S", timeinfo);
}

void ituTimerFFDraw(uint32_t width, uint32_t height)
{
    int             i          = 0;
    ITV_UI_PROPERTY ui_prop    = {0};
    uint32_t        ui_buf_ptr = 0;
    ITUColor        color_r    = {0, 0, 0, 0};

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    if (g_timer_ff_init == false)
    {
        goto end;
    }

    ui_buf_ptr = (uint32_t)itv_get_osdbuf_anchor(0);

    if (!ui_buf_ptr)
    {
        goto end;
    }

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
    {
        if (gITUSurface[i]->addr == ui_buf_ptr)
        {
            break;
        }
    }

    if (i == MAX_UI_BUF_CNT)
    {
        printf("[TimerFF] gITUSurface addr fail ...\n");
        goto end;
    }

    ituColorFill(gITUSurface[i], 0, 0, gITUSurface[i]->width, gITUSurface[i]->height, &color_r);

    //ituFtSetFontStyle(0);
    ituFtSetFontStyle(ITU_FT_STYLE_BOLD);
    ituFtSetFontSize(height, height);
    _get_rtc_time();
    ituSetColor(&gITUSurface[i]->fgColor, 255, 255, 255, 255);
    ituFtDrawText(gITUSurface[i], 0, 0, g_timer_buff);


    ui_prop.startX       = 0;
    ui_prop.startY       = 0;
    ui_prop.width        = width;
    ui_prop.height       = height;
    ui_prop.pitch        = (width * 4);
    ui_prop.colorKeyR    = 0x00;
    ui_prop.colorKeyG    = 0x00;
    ui_prop.colorKeyB    = 0x00;
    ui_prop.EnAlphaBlend = true;
    ui_prop.constAlpha   = 0xFF;
    itv_update_osdbuf_anchor(0, &ui_prop);

end:
    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void ituTimerFFInit(uint32_t width, uint32_t height)
{
    int i = 0;
    uint32_t pitch = width * 4;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    g_ui_buff_addr[0] = itpVmemAlloc(pitch * height * MAX_UI_BUF_CNT);

    assert(g_ui_buff_addr[0]);

    itv_osd_enable(0, true);
    itv_osd_setup_base(0, 0, (uint8_t *)g_ui_buff_addr[0]);

    for (i = 1; i < MAX_UI_BUF_CNT; i++)
    {
        g_ui_buff_addr[i] = g_ui_buff_addr[i-1] + pitch * height;
        itv_osd_setup_base(0, i, (uint8_t *)g_ui_buff_addr[i]);
        #if 0
        printf("g_ui_buff_addr[%d] = %x\n", i,g_ui_buff_addr[i]);
        #endif    
    }

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
    {
        gITUSurface[i] = ituCreateSurface(width, height, pitch, ITU_ARGB8888, (uint8_t*)g_ui_buff_addr[i], ITU_STATIC);
        #if 0
        printf("ITU Serface = %x\n", gITUSurface[i]);
        #endif
    }

    g_timer_ff_init = true;

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void ituTimerFFDeInit(void)
{
    int i = 0;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    if(g_timer_ff_init == true)
    {
        itv_flush_osdbuf(0);
        itv_osd_enable(0, false);

        g_timer_ff_init = false;

        for (i = 0; i < MAX_UI_BUF_CNT; i++)
        {
            if(gITUSurface[i] != NULL)
            {
                ituDestroySurface(gITUSurface[i]);
            }
        }

        if (g_ui_buff_addr[0])
        {
            itpVmemFree(g_ui_buff_addr[0]);
            g_ui_buff_addr[0] = 0;
        }
    }

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

