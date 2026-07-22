#ifndef WIFI_APP_H
#define WIFI_APP_H

#include <stdbool.h>

/* Start WiFi connection in background */
void wifi_app_start(void);

/* Status for HMI display */
bool wifi_app_is_connected(void);
const char* wifi_app_get_ssid(void);
const char* wifi_app_get_ip(void);

#endif
