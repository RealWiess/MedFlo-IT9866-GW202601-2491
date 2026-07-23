#ifndef BLE_INIT_H
#define BLE_INIT_H

#include <stdbool.h>
#include <stdint.h>

/* ── MedFlo device info ── */
typedef struct {
    char mac[18];        /* "XX:XX:XX:XX:XX:XX" */
    char name[33];       /* device name */
    int8_t rssi;         /* signal strength */
    uint8_t status;      /* status flags */
    uint8_t counter;     /* rolling counter */
} medflo_dev_t;

/* ── API ── */
void ble_init(void);
int  ble_get_device_count(void);
const medflo_dev_t *ble_get_devices(void);

#endif
