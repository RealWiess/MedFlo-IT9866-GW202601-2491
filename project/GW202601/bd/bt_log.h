#ifndef BT_LOG_H
#define BT_LOG_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_LOG_COUNT 500

typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    uint8_t status_byte;
    uint8_t rolling_counter;
    uint8_t raw_data[32]; // Store raw advertising data
    uint8_t raw_len;      // Length of raw advertising data
    uint32_t timestamp;  // UTC Unix time
    bool uploaded;       // WiFi uploaded
    bool sent_to_pc;     // USB sent
} GatewayBleLog;

void bt_log_init(void);
void bt_log_add(const uint8_t *mac, int8_t rssi, uint8_t status_byte, uint8_t rolling_counter, const uint8_t *raw_ad, uint8_t raw_len);
int bt_log_get_count(void);
int bt_log_get_all_json(char *buf, int max_len);
int bt_log_get_status_json(char *buf, int max_len, bool wifi_connected, const char *wifi_ssid, const char *ip_addr, const char *sys_log);
void app_printf(const char *format, ...);

#endif /* BT_LOG_H */
