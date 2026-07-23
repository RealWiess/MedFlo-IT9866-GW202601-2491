#ifndef MQTT_APP_H
#define MQTT_APP_H

#include <stdbool.h>

void mqtt_app_init(void);
bool mqtt_app_is_connected(void);
void mqtt_publish(const char *topic, const char *payload);

#endif
