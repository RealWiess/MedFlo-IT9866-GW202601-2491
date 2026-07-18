#include <assert.h>
#include <pthread.h>
#include <time.h>

#include "ite/itv.h"
#include "ite/itu.h"

#define MAX_UI_BUF_CNT 3
#define timer_ff_w     1024 // 1088//1024//1280
#define timer_ff_wp    timer_ff_w * 4
#define timer_ff_h     64
#define timer_ff_fs    24

static uint32_t        g_ui_buff_addr[MAX_UI_BUF_CNT];
static ITUSurface *    gITUSurface[MAX_UI_BUF_CNT];
static char            date_0[40];
static char            date_1[40];
static char            gps_0[40];
static char            gps_1[40];
static char            speed_0[20];
static char            direction_0[20];
static bool            g_timer_ff_init;
static pthread_mutex_t TIMER_FF_MUTEX      = PTHREAD_MUTEX_INITIALIZER;
static uint32_t        gps_last_time       = 0;
static uint32_t        speed_last_time     = 0;
static uint32_t        direction_last_time = 0;

static void            _get_rtc_time (char * date_0, char * date_1)
{
    time_t     current_time    = 0;
    static int count_0         = 0;
    static int count_1         = 0;
    static int count_2         = 0;
    uint32_t   current_time_ms = 0;

    struct tm  s_time          = {0};

    time(&current_time);
    current_time_ms = xTaskGetTickCount();

    if (gmtime_r(&current_time, &s_time))
    {
        sprintf(date_0, "%04d% / %02d / %02d", (1900 + s_time.tm_year), (1 + s_time.tm_mon), s_time.tm_mday);
        sprintf(date_1, "%02d% : %02d : %02d", s_time.tm_hour, s_time.tm_min, s_time.tm_sec);
    }

    if (current_time_ms < gps_last_time)
    {
        gps_last_time = current_time_ms;
    }
    if (current_time_ms < speed_last_time)
    {
        speed_last_time = current_time_ms;
    }
    if (current_time_ms < direction_last_time)
    {
        direction_last_time = current_time_ms;
    }

    if (current_time_ms - gps_last_time > 300)
    {
        gps_last_time = current_time_ms;
        if (count_0 % 3 == 0)
        {
            strlcpy(gps_0, "E123.987654", sizeof(gps_0));
            strlcpy(gps_1, "N23.123456", sizeof(gps_1));
        }
        else if (count_0 % 3 == 1)
        {
            strlcpy(gps_0, "E167.277268", sizeof(gps_0));
            strlcpy(gps_1, "N60.323456", sizeof(gps_1));
        }
        else if (count_0 % 3 == 2)
        {
            strlcpy(gps_0, "E158.156311", sizeof(gps_0));
            strlcpy(gps_1, "N37.297412", sizeof(gps_1));
            count_0 = -1;
        }
        count_0++;
        // (void)printf("1:*%llu*\n", gps_last_time);
    }

    if (current_time_ms - speed_last_time > 400)
    {
        speed_last_time = current_time_ms;
        if (count_1 % 3 == 0)
        {
            snprintf(speed_0, sizeof(speed_0), "%8s", "100km/h");
        }
        else if (count_1 % 3 == 1)
        {
            snprintf(speed_0, sizeof(speed_0), "%8s", "115km/h");
        }
        else if (count_1 % 3 == 2)
        {
            snprintf(speed_0, sizeof(speed_0), "%8s", "106km/h");
            count_1 = -1;
        }
        count_1++;
        // (void)printf("2:*%llu*\n", speed_last_time);
    }

    if (current_time_ms - direction_last_time > 500)
    {
        direction_last_time = current_time_ms;
        if (count_2 % 3 == 0)
        {
            strlcpy(direction_0, "NE", sizeof(direction_0));
        }
        else if (count_2 % 3 == 1)
        {
            strlcpy(direction_0, "SW", sizeof(direction_0));
        }
        else if (count_2 % 3 == 2)
        {
            strlcpy(direction_0, "NW", sizeof(direction_0));
            count_2 = -1;
        }
        count_2++;
        // (void)printf("3:*%llu*\n", direction_last_time);
    }
}

void timer_ff_drawing (void)
{
    int             i          = 0;
    ITV_UI_PROPERTY ui_prop    = {0};
    uint32_t        ui_buf_ptr = 0;
    ITUColor        color_r    = {0, 255, 0, 0};

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    if (!g_timer_ff_init)
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
        (void)printf("[TimerFF] gITUSurface addr fail ...\n");
        goto end;
    }

    ituColorFill(gITUSurface[i], 0, 0, gITUSurface[i]->width, gITUSurface[i]->height, &color_r);
    ituFtSetFontStyle(0);
    ituFtSetFontSize(timer_ff_fs, timer_ff_fs);
    _get_rtc_time(date_0, date_1);
    ituFtDrawText(gITUSurface[i], 0, 0, gps_0);
    ituFtDrawText(gITUSurface[i], 208, 0, gps_1);
    ituFtDrawText(gITUSurface[i], 400, 0, direction_0);
    ituFtDrawText(gITUSurface[i], 448, 0, speed_0);
    ituFtDrawText(gITUSurface[i], 624, 0, date_0);
    ituFtDrawText(gITUSurface[i], 832, 0, date_1);

    // gfxwaitEngineIdle();

    ui_prop.startX       = 0;
    ui_prop.startY       = 0;
    ui_prop.width        = timer_ff_w;
    ui_prop.height       = timer_ff_h;
    ui_prop.pitch        = timer_ff_wp;
    ui_prop.colorKeyR    = 0xFF;
    ui_prop.colorKeyG    = 0xFF;
    ui_prop.colorKeyB    = 0xFF;
    ui_prop.EnAlphaBlend = true;
    ui_prop.constAlpha   = 0xFF;
    itv_update_osdbuf_anchor(0, &ui_prop);

end:
    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void timer_ff_init (void)
{
    int i = 0;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    g_ui_buff_addr[0] = itpVmemAlloc(timer_ff_wp * timer_ff_h * MAX_UI_BUF_CNT);

    assert(g_ui_buff_addr[0]);

    itv_osd_enable(0, true);
    itv_osd_setup_base(0, 0, (uint8_t *)g_ui_buff_addr[0]);
    for (i = 1; i < MAX_UI_BUF_CNT; i++)
    {
        g_ui_buff_addr[i] = g_ui_buff_addr[i - 1] + timer_ff_wp * timer_ff_h;
        itv_osd_setup_base(0, i, (uint8_t *)g_ui_buff_addr[i]);
    }

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
    {
        gITUSurface[i] =
            ituCreateSurface(timer_ff_w, timer_ff_h, timer_ff_wp, ITU_ARGB8888, g_ui_buff_addr[i], ITU_STATIC);
    }

    gps_last_time       = xTaskGetTickCount();
    speed_last_time     = xTaskGetTickCount();
    direction_last_time = xTaskGetTickCount();
    strlcpy(gps_0, "E118.223356", sizeof(gps_0));
    strlcpy(gps_1, "N21.203831", sizeof(gps_1));
    sprintf(speed_0, "%8s", "112km/h");
    strlcpy(direction_0, "SE", sizeof(direction_0));

    g_timer_ff_init = true;

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void timer_ff_deinit (void)
{
    int i = 0;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    itv_flush_osdbuf(0);
    itv_osd_enable(0, false);

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
    {
        ituDestroySurface(gITUSurface[i]);
    }

    if (g_ui_buff_addr[0])
    {
        itpVmemFree(g_ui_buff_addr[0]);
        g_ui_buff_addr[0] = 0;
    }

    g_timer_ff_init = false;

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}
