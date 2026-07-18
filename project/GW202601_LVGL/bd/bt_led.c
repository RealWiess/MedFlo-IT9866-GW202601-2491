/*
 * bt_led.c
 *
 * LED 狀態指示控制
 *   綠燈 GPIO31 — BT 狀態
 *   橘燈 GPIO32 — WiFi 狀態
 *
 * 使用獨立 thread 每 100ms 更新一次燈號，
 * 外部只需呼叫 bt_led_set_bt() / bt_led_set_wifi() 更新狀態。
 */

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "ite/ith.h"
#include "bt_led.h"

/* ================================================================ */
/*  硬體定義                                                         */
/* ================================================================ */

#define LED_GREEN_GPIO   31   /* BT 狀態  */
#define LED_ORANGE_GPIO  32   /* WiFi 狀態 */

/* Active-low：ON 時輸出 0，OFF 時輸出 1（與 phy.c LED 一致） */
#define LED_ON   0
#define LED_OFF  1

/* ================================================================ */
/*  閃爍參數（單位：100ms tick）                                     */
/* ================================================================ */

#define BLINK_FAST   2    /* 快閃：200ms  */
#define BLINK_MID    5    /* 中閃：500ms  */
#define BLINK_SLOW   10   /* 慢閃：1000ms */

/* ================================================================ */
/*  內部狀態                                                         */
/* ================================================================ */

static volatile BtLedState   s_bt_state   = BT_LED_STATE_OFF;
static volatile WifiLedState s_wifi_state = WIFI_LED_STATE_OFF;
static volatile bool         s_running    = false;
static pthread_mutex_t       s_mutex      = PTHREAD_MUTEX_INITIALIZER;

/* ================================================================ */
/*  GPIO 工具函式                                                    */
/* ================================================================ */

static void led_gpio_init(int gpio)
{
    ithGpioEnable(gpio);
    ithGpioSetOut(gpio);
    ithGpioSetMode(gpio, ITH_GPIO_MODE0);
}

static inline void led_set(int gpio, int val)
{
    /* Active-low: LED_ON=0 → Clear (LOW) 點亮; LED_OFF=1 → Set (HIGH) 熄滅 */
    if (val == LED_ON)
        ithGpioClear(gpio);
    else
        ithGpioSet(gpio);
}

/* ================================================================ */
/*  LED 控制 Thread                                                  */
/* ================================================================ */

/*
 * 根據目前的 BT/WiFi 狀態，每 100ms 決定燈號。
 * 用 tick 計數實現不同速度的閃爍。
 */
static void* led_task(void *arg)
{
    uint32_t tick = 0;

    while (s_running) {
        BtLedState   bt_st;
        WifiLedState wifi_st;

        pthread_mutex_lock(&s_mutex);
        bt_st   = s_bt_state;
        wifi_st = s_wifi_state;
        pthread_mutex_unlock(&s_mutex);

        /* ---- 綠燈（BT）---- */
        switch (bt_st) {
        case BT_LED_STATE_OFF:
            /* 滅 */
            led_set(LED_GREEN_GPIO, LED_OFF);
            break;

        case BT_LED_STATE_ADV:
            /* 慢閃 1s：亮 500ms 滅 500ms */
            led_set(LED_GREEN_GPIO,
                    (tick % BLINK_SLOW) < (BLINK_SLOW / 2) ? LED_ON : LED_OFF);
            break;

        case BT_LED_STATE_CONNECTED:
            /* 常亮 */
            led_set(LED_GREEN_GPIO, LED_ON);
            break;
        }

        /* ---- 橘燈（WiFi）---- */
        switch (wifi_st) {
        case WIFI_LED_STATE_OFF:
            /* 滅 */
            led_set(LED_ORANGE_GPIO, LED_OFF);
            break;

        case WIFI_LED_STATE_SCANNING:
            /* 快閃 200ms：亮 100ms 滅 100ms */
            led_set(LED_ORANGE_GPIO,
                    (tick % BLINK_FAST) < (BLINK_FAST / 2) ? LED_ON : LED_OFF);
            break;

        case WIFI_LED_STATE_CONNECTING:
            /* 中閃 500ms */
            led_set(LED_ORANGE_GPIO,
                    (tick % BLINK_MID) < (BLINK_MID / 2) ? LED_ON : LED_OFF);
            break;

        case WIFI_LED_STATE_CONNECTED:
            /* 常亮 */
            led_set(LED_ORANGE_GPIO, LED_ON);
            break;

        case WIFI_LED_STATE_LOST:
            /* 快閃提示斷線 */
            led_set(LED_ORANGE_GPIO,
                    (tick % BLINK_FAST) < (BLINK_FAST / 2) ? LED_ON : LED_OFF);
            break;
        }

        tick++;
        usleep(100 * 1000);  /* 100ms */
    }

    /* 離開時關燈 */
    led_set(LED_GREEN_GPIO,  LED_OFF);
    led_set(LED_ORANGE_GPIO, LED_OFF);
    return NULL;
}

/* ================================================================ */
/*  公開函式                                                         */
/* ================================================================ */

void bt_led_init(void)
{
    /* 初始化 GPIO */
    led_gpio_init(LED_GREEN_GPIO);
    led_gpio_init(LED_ORANGE_GPIO);

    /* 啟動 LED 控制 thread */
    s_running = true;

    pthread_t      tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 8 * 1024);
    pthread_create(&tid, &attr, led_task, NULL);

    printf("[bt_led] LED init OK (GREEN=GPIO%d, ORANGE=GPIO%d)\n",
           LED_GREEN_GPIO, LED_ORANGE_GPIO);
}

void bt_led_set_bt(BtLedState state)
{
    pthread_mutex_lock(&s_mutex);
    s_bt_state = state;
    pthread_mutex_unlock(&s_mutex);
    printf("[bt_led] BT state -> %d\n", state);
}

void bt_led_set_wifi(WifiLedState state)
{
    pthread_mutex_lock(&s_mutex);
    s_wifi_state = state;
    pthread_mutex_unlock(&s_mutex);
    printf("[bt_led] WiFi state -> %d\n", state);
}
