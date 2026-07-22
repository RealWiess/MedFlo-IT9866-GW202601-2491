/*
 * ble_init.c — Minimal NimBLE init to power-on RTL8821CS chip for WiFi
 */
#include <stdio.h>
#include <unistd.h>
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "ite/itp.h"

static void on_reset(int reason)
{
    printf("[BLE] reset (reason=%d)\n", reason);
}

static void on_sync(void)
{
    printf("[BLE] sync done — chip initialized\n");
}

static void* ble_task(void *arg)
{
    (void)arg;

    printf("[BLE] starting NimBLE to init RTL8821CS...\n");

    /* Minimal host config */
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb  = on_sync;

    /* Init NimBLE port — this powers on RTL8821CS */
    nimble_port_init();
    printf("[BLE] nimble_port_init done\n");

    /* Let the controller initialize */
    sleep(3);
    printf("[BLE] chip should be ready now\n");

    return NULL;
}

void ble_init(void)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&tid, &attr, ble_task, NULL);
    pthread_attr_destroy(&attr);
    printf("[BLE] thread started\n");
}
