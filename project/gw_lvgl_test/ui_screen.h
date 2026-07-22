#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include "lvgl.h"

/* Show splash, then auto-transition to HMI after ~3 seconds */
void ui_screen_init(void);

/* Status updates (called from main loop) */
void ui_screen_update_wifi(const char *ssid, const char *ip, bool connected);
void ui_screen_update_ble(int device_count, bool ok);
void ui_screen_set_error(bool error);

#endif /* UI_SCREEN_H */
