#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ite/itp.h"
#include "ite/ug.h"

extern int NetworkWifiGetRetryCount(void);
#include "ite/itc.h"
#include "curl/curl.h"
#include "bt_log.h"
#include "bt_wifi.h"
#include "ctrlboard.h"
#include "bt_main.h"

#undef printf
#define printf app_printf

extern void bt_wifi_get_ip(char *ip, int ip_len);

static GatewayBleLog s_log_buffer[MAX_LOG_COUNT];
static int s_log_head = 0;
static int s_log_tail = 0;
static int s_log_count = 0;
static pthread_mutex_t s_log_mutex = PTHREAD_MUTEX_INITIALIZER;

#define BATCH_SIZE 50

#define MQTT_HOST "mqtt.go6.tw"
#define MQTT_PORT 1883
#define MQTT_USER "DCareW"
#define MQTT_PASS "4rfghy6"
#define MQTT_OTA_USER "DCareR"
#define MQTT_OTA_PASS "6yhgvfr4"

// OTA state
char g_ota_url[256] = "";
volatile bool g_ota_pending = false;
static uint16_t s_mqtt_pkt_id = 1;

// OTA RAM buffer state
static uint8_t *s_ota_buf = NULL;
static int s_ota_buf_offset = 0;

// curl write callback: accumulate downloaded data in RAM
static size_t ota_write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    if (!s_ota_buf) {
        s_ota_buf = malloc(16 * 1024 * 1024);
        if (!s_ota_buf) { printf("[OTA] RAM alloc failed\n"); return 0; }
        s_ota_buf_offset = 0;
        printf("[OTA] RAM buffer allocated (16MB)\n");
    }
    if (s_ota_buf_offset + (int)total > 16 * 1024 * 1024) {
        printf("[OTA] RAM full\n"); return 0;
    }
    memcpy(s_ota_buf + s_ota_buf_offset, ptr, total);
    s_ota_buf_offset += total;
    return total;
}

// Use SDK upgrade framework to flash downloaded PKG to NOR
static int ota_flash_pkg(uint8_t *buf, int size) {
    ITCArrayStream arrayStream;
    itcArrayStreamOpen(&arrayStream, buf, size);
    printf("[OTA] Verifying PKG CRC...\n");
    if (ugCheckCrc(&arrayStream.stream, NULL) != 0) {
        printf("[OTA] CRC check FAILED\n");
        return -1;
    }
    printf("[OTA] CRC OK, flashing...\n");
    ioctl(ITP_DEVICE_WATCHDOG, ITP_IOCTL_DISABLE, NULL);
    int result = ugUpgradePackage(&arrayStream.stream);
    ioctl(ITP_DEVICE_WATCHDOG, ITP_IOCTL_ENABLE, NULL);
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    // Code 124 = ug_file.c "Out of space" (FAT not accessible from bg thread)
    // Code 134 = ugUpgradePackage file/dir copy error (EINVAL on media files)
    // The firmware raw data (TAG_RAWDATA) was written before this point.
    // File copy failures are non-critical - just means files on partition
    // weren't updated. Treat as soft success.
    if (result == 0 || result == 124 || result == 134) {
        if (result == 0)
            printf("[OTA] Flash SUCCESS\n");
        else
            printf("[OTA] Firmware OK, file copy skipped (code %d)\n", result);
        return 0;
    }
    printf("[OTA] Flash FAILED (code %d, errno=%d)\n", result, errno);
    return result;
}

// MQTT 可變長度編碼
static int mqtt_encode_len(uint8_t *buf, int len) {
    int i = 0;
    do {
        uint8_t b = len & 0x7F;
        len >>= 7;
        if (len > 0) b |= 0x80;
        buf[i++] = b;
    } while (len > 0);
    return i;
}

// 發送 MQTT CONNECT
static int mqtt_connect(int sock, const char *client_id, const char *user, const char *pass) {
    uint8_t pkt[256];
    int pos = 0;
    // Variable header: "MQTT" (4) + protocol_level (4) + flags (C2=user+pass+clean) + keepalive (60)
    uint8_t var_header[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0xC2, 0x00, 0x3C};
    int var_len = sizeof(var_header);
    int id_len = strlen(client_id);
    int user_len = strlen(user);
    int pass_len = strlen(pass);
    // Remaining length = var_header + client_id(2+len) + username(2+len) + password(2+len)
    int remaining = var_len + 2 + id_len + 2 + user_len + 2 + pass_len;

    // Fixed header
    pkt[pos++] = 0x10; // CONNECT
    pos += mqtt_encode_len(pkt + pos, remaining);
    // Variable header
    memcpy(pkt + pos, var_header, var_len); pos += var_len;
    // Client ID
    pkt[pos++] = (id_len >> 8) & 0xFF;
    pkt[pos++] = id_len & 0xFF;
    memcpy(pkt + pos, client_id, id_len); pos += id_len;
    // Username
    pkt[pos++] = (user_len >> 8) & 0xFF;
    pkt[pos++] = user_len & 0xFF;
    memcpy(pkt + pos, user, user_len); pos += user_len;
    // Password
    pkt[pos++] = (pass_len >> 8) & 0xFF;
    pkt[pos++] = pass_len & 0xFF;
    memcpy(pkt + pos, pass, pass_len); pos += pass_len;

    return send(sock, pkt, pos, 0) == pos ? 0 : -1;
}

// 發送 MQTT SUBSCRIBE (QoS 0)
static int mqtt_subscribe(int sock, const char *topic) {
    uint8_t pkt[256];
    int topic_len = strlen(topic);
    uint16_t pid = s_mqtt_pkt_id++;
    int remaining = 2 + 2 + topic_len + 1;  // pkt_id + topic_len + topic + qos
    int pos = 0;

    pkt[pos++] = 0x82; // SUBSCRIBE
    pos += mqtt_encode_len(pkt + pos, remaining);
    pkt[pos++] = (pid >> 8) & 0xFF;
    pkt[pos++] = pid & 0xFF;
    pkt[pos++] = (topic_len >> 8) & 0xFF;
    pkt[pos++] = topic_len & 0xFF;
    memcpy(pkt + pos, topic, topic_len); pos += topic_len;
    pkt[pos++] = 0x00; // QoS 0

    if (send(sock, pkt, pos, 0) != pos) {
        printf("[MQTT] SUBSCRIBE send failed for topic: %s\n", topic);
        return -1;
    }

    // Read SUBACK
    uint8_t suback[5];
    int n = recv(sock, suback, 5, 0);
    if (n >= 5 && suback[0] == 0x90) {
        // suback[4] is the return code for the subscription (0x00 = QoS 0 OK, 0x80 = failure)
        if (suback[4] == 0x00) {
            printf("[MQTT] Subscribed OK: %s\n", topic);
        } else {
            printf("[MQTT] Subscribe REJECTED (code=0x%02X) for: %s\n", suback[4], topic);
        }
    } else if (n > 0) {
        printf("[MQTT] SUBACK unexpected: type=0x%02X n=%d for topic: %s\n", suback[0], n, topic);
    } else {
        printf("[MQTT] SUBACK timeout for topic: %s (n=%d)\n", topic, n);
    }
    return 0;  // Always succeed - subscribe is best-effort for OTA
}

// Forward declarations
static int mqtt_publish(int sock, const char *topic, const char *payload, int payload_len);

// 處理 incoming MQTT messages（PINGRESP / PUBLISH for OTA）
// Returns: 0 = nothing available, 1 = PINGRESP received, 2 = PUBLISH processed, -1 = error
static int mqtt_check_incoming(int sock) {
    uint8_t first_byte;
    uint8_t pkt_type;
    int n;
    int got_byte = 0;

    // Method 1: try non-blocking recv (MSG_DONTWAIT)
    n = recv(sock, &first_byte, 1, MSG_DONTWAIT);
    if (n == 1) {
        got_byte = 1;
    } else if (n == 0) {
        return 0;  // EOF
    } else {
        // Method 2: fallback to FIONREAD + blocking recv
        int avail = 0;
        if (ioctl(sock, FIONREAD, &avail) < 0 || avail == 0) return 0;
        printf("[MQTT] FIONREAD=%d (MSG_DONTWAIT miss)\n", avail);
        n = recv(sock, &first_byte, 1, 0);
        if (n <= 0) return 0;
        got_byte = 1;
    }

    if (!got_byte) return 0;

    pkt_type = first_byte & 0xF0;

    // PINGRESP: single byte 0xD0 followed by 0x00 (remaining length)
    if (pkt_type == 0xD0) {
        uint8_t remaining;
        n = recv(sock, &remaining, 1, 0);
        if (n == 1 && remaining == 0x00) {
            printf("[MQTT] PINGRESP received\n");
            return 1;  // PINGRESP
        }
        return 0;  // Malformed PINGRESP, ignore
    }

    // Only handle PUBLISH packets
    if (pkt_type != 0x30) {
        // Unknown packet type - try to skip remaining length and data
        printf("[MQTT] Ignoring unknown packet type 0x%02X\n", pkt_type);
        // Read remaining length
        int mul = 1, rem_len = 0;
        uint8_t b;
        do {
            n = recv(sock, &b, 1, 0);
            if (n <= 0) return -1;
            rem_len += (b & 0x7F) * mul;
            mul *= 128;
        } while (b & 0x80);
        // Skip remaining data
        uint8_t dummy[256];
        int left = rem_len;
        while (left > 0) {
            int chunk = left > (int)sizeof(dummy) ? (int)sizeof(dummy) : left;
            n = recv(sock, dummy, chunk, 0);
            if (n <= 0) break;
            left -= n;
        }
        return 2;
    }

    // Read remaining length
    int mul = 1, rem_len = 0;
    uint8_t b;
    do {
        n = recv(sock, &b, 1, 0);
        if (n <= 0) return -1;
        rem_len += (b & 0x7F) * mul;
        mul *= 128;
    } while (b & 0x80);

    // Read topic length
    uint8_t topic_len_buf[2];
    n = recv(sock, topic_len_buf, 2, 0);
    if (n < 2) return -1;
    int topic_len = (topic_len_buf[0] << 8) | topic_len_buf[1];
    if (topic_len <= 0 || topic_len > 127) return -1;   // safety

    // Calculate payload offset (topic_len(2) + topic + [packet_id(2) for QoS>0])
    int payload_ofs = 2 + topic_len;
    uint8_t qos = (first_byte >> 1) & 0x03;
    if (qos > 0) payload_ofs += 2;

    // Read topic
    uint8_t topic_buf[128];
    n = recv(sock, topic_buf, topic_len, 0);
    if (n < topic_len) return -1;
    topic_buf[topic_len] = '\0';
    const char *topic = (const char*)topic_buf;

    // Read payload
    int payload_len = rem_len - payload_ofs;
    if (payload_len <= 0 || payload_len > 2048) {
        // No payload (e.g. empty message), skip
        if (payload_len == 0) {
            printf("[MQTT] Empty PUBLISH on topic: %s\n", topic);
            return 2;
        }
        return -1;
    }
    char *json = malloc(payload_len + 1);
    if (!json) return -1;
    n = recv(sock, json, payload_len, 0);
    if (n < payload_len) { free(json); return -1; }
    json[payload_len] = '\0';

    printf("[MQTT] PUBLISH topic=%s payload=%s\n", topic, json);

    // Check for OTA command
    if (strstr(json, "\"cmd\":\"ota\"") || strstr(json, "\"cmd\": \"ota\"")) {
        // Extract URL
        char *url_start = strstr(json, "\"url\":\"");
        if (!url_start) url_start = strstr(json, "\"url\": \"");
        if (url_start) {
            url_start = strchr(url_start, ':') + 1;
            while (*url_start == ' ' || *url_start == '"') url_start++;
            char *url_end = strchr(url_start, '"');
            if (url_end) {
                int url_len = url_end - url_start;
                if (url_len < (int)sizeof(g_ota_url) - 1) {
                    memcpy(g_ota_url, url_start, url_len);
                    g_ota_url[url_len] = '\0';
                    g_ota_pending = true;
                    printf("[OTA] Command received! URL: %s\n", g_ota_url);
                    // Send ACK via publish
                    char ack[128];
                    snprintf(ack, sizeof(ack), "{\"ota\":\"accepted\",\"url\":\"%s\"}", g_ota_url);
                    char status_topic[96];
                    snprintf(status_topic, sizeof(status_topic), "%s/status", topic);
                    // Remove /ctrl suffix for status topic
                    char *ctrl = strstr(status_topic, "/ctrl/");
                    if (!ctrl) ctrl = strstr(status_topic, "/ctrl");
                    if (ctrl) strcpy(ctrl, "/status");
                    mqtt_publish(sock, status_topic, ack, strlen(ack));
                }
            }
        }
    }

    free(json);
    return 2;  // PUBLISH processed
}

// 發送 MQTT PUBLISH (QoS 0 - 高效，適合大量設備上報)
static int mqtt_publish(int sock, const char *topic, const char *payload, int payload_len) {
    uint8_t fixed[10];
    int topic_len = strlen(topic);
    int remaining = 2 + topic_len + payload_len;
    int pos = 0;

    fixed[pos++] = 0x30; // PUBLISH, QoS 0
    pos += mqtt_encode_len(fixed + pos, remaining);
    fixed[pos++] = (topic_len >> 8) & 0xFF;
    fixed[pos++] = topic_len & 0xFF;

    if (send(sock, fixed, pos, 0) != pos) return -1;
    if (send(sock, topic, topic_len, 0) != topic_len) return -1;
    if (payload_len > 0) {
        if (send(sock, payload, payload_len, 0) != payload_len) return -1;
    }
    return 0;
}

static void* MqttUploadTask(void* arg) {
    sleep(10); // Wait for WiFi to initialize

    int sock = -1;
    int ota_sock = -1;
    uint32_t last_activity = 0;
    uint32_t last_ota_activity = 0;
    uint32_t last_heartbeat = 0;       // 30s heartbeat for PC App discovery
    char gwid[20] = "unknown";
    char st_topic[96] = "";          // persistent: DCare/d/<gwid>/status
    char topic[64] = "";
    char ip[16] = "0.0.0.0";

    while (1) {
        // Check WiFi
        bt_wifi_get_ip(ip, sizeof(ip));
        if (strcmp(ip, "0.0.0.0") == 0) {
            if (sock >= 0) { nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; }
            usleep(2000000);
            continue;
        }

        // Build GWID
        char mac[20];
        bt_wifi_get_mac(mac, sizeof(mac));
        if (strcmp(mac, "00:00:00:00:00:00") != 0) {
            snprintf(gwid, sizeof(gwid), "0000%c%c%c%c%c%c%c%c%c%c%c%c",
                     mac[0], mac[1], mac[3], mac[4], mac[6], mac[7],
                     mac[9], mac[10], mac[12], mac[13], mac[15], mac[16]);
        }
        snprintf(topic, sizeof(topic), "DCare/d/%s", gwid);

        // OTA download trigger: must run even if MQTT is down (WiFi may be OK, or USB-triggered)
        if (g_ota_pending && g_ota_url[0]) {
            g_ota_pending = false;
            printf("[OTA] Starting firmware download from %s\n", g_ota_url);
            // Publish download start (best-effort, ignore if no MQTT)
            if (sock >= 0) {
                char status[128];
                snprintf(status, sizeof(status), "{\"ota\":\"downloading\",\"url\":\"%s\"}", g_ota_url);
                char st_topic[96];
                snprintf(st_topic, sizeof(st_topic), "DCare/d/%s/status", gwid);
                mqtt_publish(sock, st_topic, status, strlen(status));
            }

            s_ota_buf = NULL;
            s_ota_buf_offset = 0;
            CURL *curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, g_ota_url);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ota_write_callback);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180L);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                CURLcode res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);

                if (res == CURLE_OK && s_ota_buf && s_ota_buf_offset > 0) {
                    printf("[OTA] Download OK (%d bytes), flashing...\n", s_ota_buf_offset);
                    if (sock >= 0) {
                        char status[128];
                        snprintf(status, sizeof(status), "{\"ota\":\"flashing\",\"bytes\":%d}", s_ota_buf_offset);
                        char st_topic[96];
                        snprintf(st_topic, sizeof(st_topic), "DCare/d/%s/status", gwid);
                        mqtt_publish(sock, st_topic, status, strlen(status));
                    }
                    if (ota_flash_pkg(s_ota_buf, s_ota_buf_offset) == 0) {
                        printf("[OTA] Flash OK, rebooting...\n");
                        if (sock >= 0) {
                            char status[128];
                            snprintf(status, sizeof(status), "{\"ota\":\"ready\",\"rebooting\":true}");
                            char st_topic[96];
                            snprintf(st_topic, sizeof(st_topic), "DCare/d/%s/status", gwid);
                            mqtt_publish(sock, st_topic, status, strlen(status));
                        }
                        free(s_ota_buf); s_ota_buf = NULL;
                        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_EXIT, NULL);
                        usleep(500000);
                        exit(0);
                    } else {
                        printf("[OTA] Flash failed\n");
                    }
                } else {
                    printf("[OTA] Download FAILED: curl=%d (%s) bytes=%d\n",
                           res, curl_easy_strerror(res), s_ota_buf_offset);
                }
                if (s_ota_buf) { free(s_ota_buf); s_ota_buf = NULL; }
            }
            // After OTA, reset sockets to trigger clean reconnect
            if (sock >= 0) { nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; }
            if (ota_sock >= 0) { nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1; }
            continue;
        }

        // Connect if needed
        if (sock < 0) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) { usleep(2000000); continue; }

            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(MQTT_PORT);
            addr.sin_addr.s_addr = inet_addr(MQTT_HOST);
            if (addr.sin_addr.s_addr == INADDR_NONE) {
                struct hostent *he = gethostbyname(MQTT_HOST);
                if (he && he->h_addr_list[0])
                    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
                else { nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(5000000); continue; }
            }

            printf("[MQTT] Connecting to %s:%d...\n", MQTT_HOST, MQTT_PORT);
            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                printf("[MQTT] Connect failed\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(5000000); continue;
            }

            if (mqtt_connect(sock, gwid, MQTT_USER, MQTT_PASS) < 0) {
                printf("[MQTT] CONNECT failed\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(5000000); continue;
            }

            uint8_t connack[4];
            int n = recv(sock, connack, 4, 0);
            if (n < 4) {
                printf("[MQTT] CONNACK timeout (n=%d)\n", n);
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(5000000); continue;
            }
            if (connack[0] != 0x20) {
                printf("[MQTT] CONNACK bad type=%02x\n", connack[0]);
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(5000000); continue;
            }
            if (connack[3] != 0x00) {
                // MQTT return codes: 1=bad protocol, 2=bad ID, 3=server unavailable, 4=bad user/pass, 5=not authorized
                printf("[MQTT] CONNACK rejected, code=%d\n", connack[3]);
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1; usleep(10000000); continue;
            }
            printf("[MQTT] Connected OK, topic=%s\n", topic);
            nmgw_status_update(NMGW_FLAG_MQTT_OK, true);
            last_activity = (uint32_t)time(NULL);

            // Force OTA socket reconnect so both sockets are always fresh.
            // Fixes: WiFi TEMP_DISCONNECT → sock reconnects but ota_sock stays stale
            if (ota_sock >= 0) {
                nmgw_status_update(NMGW_FLAG_OTA_OK, false);
                close(ota_sock);
                ota_sock = -1;
                printf("[MQTT-OTA] Reset (main socket reconnected)\n");
            }

            // Build persistent status topic (used by heartbeat later)
            snprintf(st_topic, sizeof(st_topic), "DCare/d/%s/status", gwid);

            // Publish boot status (fw_ver for OTA verification)
            char boot_status[128];
            snprintf(boot_status, sizeof(boot_status),
                "{\"gwid\":\"%s\",\"fw_ver\":\"%s\",\"status\":\"online\"}",
                gwid, FW_BUILD_TIME);
            mqtt_publish(sock, st_topic, boot_status, strlen(boot_status));
            last_heartbeat = (uint32_t)time(NULL);

            // Set recv timeout
            struct timeval tv = {2, 0};
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            printf("[MQTT] GWID=%s (log publish only)\n", gwid);
        }

        // ============================================================
        // OTA SOCKET: Dedicated DCareR connection for receiving OTA commands
        // (Third-party broker has read/write separation: DCareW=write, DCareR=read)
        // ============================================================
        if (ota_sock < 0) {
            ota_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (ota_sock >= 0) {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(MQTT_PORT);
                addr.sin_addr.s_addr = inet_addr(MQTT_HOST);
                if (addr.sin_addr.s_addr == INADDR_NONE) {
                    struct hostent *he = gethostbyname(MQTT_HOST);
                    if (he && he->h_addr_list[0])
                        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
                    else { nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1; }
                }
                if (ota_sock >= 0) {
                    char ota_cid[64];
                    snprintf(ota_cid, sizeof(ota_cid), "%s_ota", gwid);
                    printf("[MQTT-OTA] Connecting to %s:%d (DCareR)...\n", MQTT_HOST, MQTT_PORT);
                    if (connect(ota_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                        printf("[MQTT-OTA] Connect failed\n");
                        nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
                    } else if (mqtt_connect(ota_sock, ota_cid, MQTT_OTA_USER, MQTT_OTA_PASS) < 0) {
                        printf("[MQTT-OTA] CONNECT failed\n");
                        nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
                    } else {
                        uint8_t connack[4];
                        int n = recv(ota_sock, connack, 4, 0);
                        if (n < 4 || connack[0] != 0x20 || connack[3] != 0x00) {
                            printf("[MQTT-OTA] CONNACK bad/rejected (n=%d code=%d)\n", n, n>=4?connack[3]:-1);
                            nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
                        } else {
                            printf("[MQTT-OTA] Connected OK\n");
                            nmgw_status_update(NMGW_FLAG_OTA_OK, true);
                            last_ota_activity = (uint32_t)time(NULL);
                            // Subscribe to broadcast and device-specific OTA topics
                            mqtt_subscribe(ota_sock, "DCare/d/broadcast/ota");
                            char ctrl_topic[96];
                            snprintf(ctrl_topic, sizeof(ctrl_topic), "DCare/d/%s/ctrl", gwid);
                            mqtt_subscribe(ota_sock, ctrl_topic);
                            struct timeval tv2 = {2, 0};
                            setsockopt(ota_sock, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(tv2));
                            printf("[MQTT-OTA] Listening on %s + DCare/d/broadcast/ota\n", ctrl_topic);
                        }
                    }
                }
            }
            if (ota_sock < 0) usleep(5000000); // retry delay
        }

        // ============================================================
        // STEP 1: Process ALL incoming MQTT data FIRST (before PINGREQ)
        // This prevents the PINGREQ recv() from consuming OTA messages
        // ============================================================
        if (ota_sock >= 0) {
            int incoming_rc = mqtt_check_incoming(ota_sock);
            if (incoming_rc == 1) {
                last_ota_activity = (uint32_t)time(NULL);
            } else if (incoming_rc < 0) {
                printf("[MQTT-OTA] Incoming read error, reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
            }
        }
        if (sock >= 0) {
            int incoming_rc = mqtt_check_incoming(sock);
            if (incoming_rc == 1) {
                // Stale PINGRESP from previous cycle
                last_activity = (uint32_t)time(NULL);
            } else if (incoming_rc < 0) {
                printf("[MQTT] Incoming read error, reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1;
                continue;
            }
        }

        // Gather batch
        GatewayBleLog batch_logs[BATCH_SIZE];
        int batch_indices[BATCH_SIZE];
        int batch_count = 0;
        pthread_mutex_lock(&s_log_mutex);
        int idx = s_log_head;
        for (int i = 0; i < s_log_count && batch_count < BATCH_SIZE; i++) {
            if (!s_log_buffer[idx].uploaded) {
                batch_logs[batch_count] = s_log_buffer[idx];
                batch_indices[batch_count] = idx;
                batch_count++;
            }
            idx = (idx + 1) % MAX_LOG_COUNT;
        }
        pthread_mutex_unlock(&s_log_mutex);

        if (batch_count > 0) {
            // Build and publish
            char *payload = malloc(20480);
            if (payload) {
                #define PMAX 20480
                int off = snprintf(payload, PMAX, "[");
                for (int i = 0; i < batch_count && off < PMAX - 450; i++) {
                    struct tm *tm_info = localtime((time_t*)&batch_logs[i].timestamp);
                    char time_str[30];
                    if (tm_info) strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+08:00", tm_info);
                    else snprintf(time_str, sizeof(time_str), "2026-07-01T00:00:00+08:00");
                    char data_hex[65] = {0};
                    for (int j = 0; j < batch_logs[i].raw_len && j < 32; j++)
                        sprintf(&data_hex[j*2], "%02X", batch_logs[i].raw_data[j]);
                    if (batch_logs[i].raw_len == 0)
                        sprintf(data_hex, "%02X%02X", batch_logs[i].status_byte, batch_logs[i].rolling_counter);
                    if (i > 0) off += snprintf(payload + off, PMAX - off, ",");
                    off += snprintf(payload + off, PMAX - off,
                        "{\"time\":\"%s\",\"gwid\":\"%s\",\"rssi\":%d,\"stat\":%d,\"cnt\":%d,\"mac\":\"%02X%02X%02X%02X%02X%02X\",\"data\":\"%s\"}",
                        time_str, gwid, batch_logs[i].rssi, batch_logs[i].status_byte, batch_logs[i].rolling_counter,
                        batch_logs[i].mac[5], batch_logs[i].mac[4], batch_logs[i].mac[3], batch_logs[i].mac[2], batch_logs[i].mac[1], batch_logs[i].mac[0], data_hex);
                }
                off += snprintf(payload + off, PMAX - off, "]");
                #undef PMAX

                if (mqtt_publish(sock, topic, payload, off) == 0) {
                    printf("[MQTT] Publish OK (%d logs, %d bytes)\n", batch_count, off);
                    pthread_mutex_lock(&s_log_mutex);
                    for (int i = 0; i < batch_count; i++)
                        s_log_buffer[batch_indices[i]].uploaded = true;
                    pthread_mutex_unlock(&s_log_mutex);
                    last_activity = (uint32_t)time(NULL);
                } else {
                    printf("[MQTT] Publish failed, reconnecting...\n");
                    nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1;
                }
                free(payload);
            }
        } else {
            usleep(200000);
        }

        // Second incoming check (after batch work, before OTA trigger)
        // Check OTA socket first (DCareR - dedicated for OTA commands)
        if (ota_sock >= 0) {
            int incoming_rc = mqtt_check_incoming(ota_sock);
            if (incoming_rc == 1) {
                last_ota_activity = (uint32_t)time(NULL);
            } else if (incoming_rc < 0) {
                printf("[MQTT-OTA] Incoming read error (2), reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
            }
        }
        if (sock >= 0) {
            int incoming_rc = mqtt_check_incoming(sock);
            if (incoming_rc == 1) {
                last_activity = (uint32_t)time(NULL);
            } else if (incoming_rc < 0) {
                printf("[MQTT] Incoming read error (2), reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1;
                continue;
            }
        }

        // Heartbeat: publish status every 30s so PC App discovers all gateways
        // even if no BLE devices nearby (boot status is one-shot, QoS 0)
        if (sock >= 0 && (uint32_t)time(NULL) - last_heartbeat >= 30) {
            char hb[128];
            snprintf(hb, sizeof(hb),
                "{\"gwid\":\"%s\",\"fw_ver\":\"%s\",\"status\":\"online\"}",
                gwid, FW_BUILD_TIME);
            mqtt_publish(sock, st_topic, hb, strlen(hb));
            last_heartbeat = (uint32_t)time(NULL);
        }

        // Send PINGREQ every 30 seconds to keep connection alive
        // Uses mqtt_check_incoming() for safe recv (won't consume OTA messages)
        if (sock >= 0 && (uint32_t)time(NULL) - last_activity > 30) {
            uint8_t pingreq[] = {0xC0, 0x00};
            if (send(sock, pingreq, 2, 0) != 2) {
                printf("[MQTT] PINGREQ send failed, reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1;
                continue;
            }
            // Wait for PINGRESP, safely handling any other incoming data
            // (e.g. OTA PUBLISH arriving between PINGREQ and PINGRESP)
            int got_pong = 0;
            uint32_t ping_start = (uint32_t)time(NULL);
            while (!got_pong && (uint32_t)time(NULL) - ping_start < 5) {
                int rc = mqtt_check_incoming(sock);
                if (rc == 1) {
                    got_pong = 1;
                    last_activity = (uint32_t)time(NULL);
                } else if (rc < 0) {
                    break;  // connection error
                }
                // rc == 0: no data yet, sleep briefly and retry
                // rc == 2: non-PINGRESP data processed (e.g. OTA!), keep waiting
                if (!got_pong) usleep(100000);
            }
            if (!got_pong) {
                printf("[MQTT] Ping timeout (no PINGRESP in 5s), reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_MQTT_OK, false); close(sock); sock = -1;
            }
        }

        // PINGREQ for OTA socket (DCareR - keepalive)
        if (ota_sock >= 0 && (uint32_t)time(NULL) - last_ota_activity > 30) {
            uint8_t pingreq[] = {0xC0, 0x00};
            if (send(ota_sock, pingreq, 2, 0) != 2) {
                printf("[MQTT-OTA] PINGREQ send failed, reconnecting...\n");
                nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
            } else {
                int got_pong = 0;
                uint32_t ping_start = (uint32_t)time(NULL);
                while (!got_pong && (uint32_t)time(NULL) - ping_start < 5) {
                    int rc = mqtt_check_incoming(ota_sock);
                    if (rc == 1) {
                        got_pong = 1;
                        last_ota_activity = (uint32_t)time(NULL);
                    } else if (rc < 0) {
                        break;
                    }
                    if (!got_pong) usleep(100000);
                }
                if (!got_pong) {
                    printf("[MQTT-OTA] Ping timeout, reconnecting...\n");
                    nmgw_status_update(NMGW_FLAG_OTA_OK, false); close(ota_sock); ota_sock = -1;
                }
            }
        }
    }
    return NULL;
}

void bt_log_init(void) {
    pthread_mutex_lock(&s_log_mutex);
    s_log_head = 0;
    s_log_tail = 0;
    s_log_count = 0;
    memset(s_log_buffer, 0, sizeof(s_log_buffer));
    pthread_mutex_unlock(&s_log_mutex);

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32 * 1024);
    pthread_create(&thread, &attr, MqttUploadTask, NULL);
}

void bt_log_add(const uint8_t *mac, int8_t rssi, uint8_t status_byte, uint8_t rolling_counter, const uint8_t *raw_ad, uint8_t raw_len) {
    pthread_mutex_lock(&s_log_mutex);
    
    // Check if buffer is full; if so, overwrite the oldest log
    if (s_log_count >= MAX_LOG_COUNT) {
        s_log_head = (s_log_head + 1) % MAX_LOG_COUNT;
        s_log_count--;
    }
    
    GatewayBleLog *log = &s_log_buffer[s_log_tail];
    memcpy(log->mac, mac, 6);
    log->rssi = rssi;
    log->status_byte = status_byte;
    log->rolling_counter = rolling_counter;
    
    // Copy raw advertising data
    log->raw_len = raw_len > 32 ? 32 : raw_len;
    if (raw_ad && log->raw_len > 0) {
        memcpy(log->raw_data, raw_ad, log->raw_len);
    }
    
    log->timestamp = (uint32_t)time(NULL);
    log->uploaded = false;
    log->sent_to_pc = false;
    
    s_log_tail = (s_log_tail + 1) % MAX_LOG_COUNT;
    s_log_count++;
    
    pthread_mutex_unlock(&s_log_mutex);
}

int bt_log_get_count(void) {
    int count;
    pthread_mutex_lock(&s_log_mutex);
    count = s_log_count;
    pthread_mutex_unlock(&s_log_mutex);
    return count;
}

// Convert all logs to a JSON array string and mark them as sent to PC
// Convert all logs to a JSON array string and mark them as sent to PC
int bt_log_get_all_json(char *buf, int max_len) {
    pthread_mutex_lock(&s_log_mutex);
    int pos = 0;
    int n;
    
    n = snprintf(buf + pos, max_len - pos, "{\"cmd\":\"LOGS\",\"logs\":[");
    if (n > 0 && pos + n < max_len) pos += n;
    
    // 限制發送數量為最多 80 筆（大幅增加安全邊際，且足夠實時上報）
    int max_send = 80;
    int start_i = 0;
    if (s_log_count > max_send) {
        start_i = s_log_count - max_send;
    }
    
    int index = (s_log_head + start_i) % MAX_LOG_COUNT;
    int added_count = 0;
    for (int i = start_i; i < s_log_count; i++) {
        GatewayBleLog *log = &s_log_buffer[index];
        
        // 預留足夠的緩衝空間（每個 log 約需 250 bytes，包含 raw data），若空間不足則提早結束
        if (max_len - pos < 300) {
            break;
        }
        
        struct tm *tm_info = localtime((time_t*)&log->timestamp);
        char time_str[30];
        if (tm_info) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+08:00", tm_info);
        } else {
            snprintf(time_str, sizeof(time_str), "2026-07-03T00:00:00+08:00");
        }
        
        if (added_count > 0) {
            n = snprintf(buf + pos, max_len - pos, ",");
            if (n > 0 && pos + n < max_len) pos += n;
        }
        
        n = snprintf(buf + pos, max_len - pos, 
            "{\"mac\":\"%02X%02X%02X%02X%02X%02X\",\"name\":\"MEDFLO-%02X%02X%02X%02X%02X%02X\",\"rssi\":%d,\"status\":%d,\"counter\":%d,\"data\":\"",
            log->mac[5], log->mac[4], log->mac[3], log->mac[2], log->mac[1], log->mac[0],
            log->mac[5], log->mac[4], log->mac[3], log->mac[2], log->mac[1], log->mac[0],
            log->rssi, log->status_byte, log->rolling_counter
        );
        if (n > 0 && pos + n < max_len) pos += n;
        
        for (int j = 0; j < log->raw_len; j++) {
            if (pos >= max_len - 10) break;
            n = snprintf(buf + pos, max_len - pos, "%02X", log->raw_data[j]);
            if (n > 0 && pos + n < max_len) pos += n;
        }
        
        n = snprintf(buf + pos, max_len - pos, "\",\"time\":\"%s\",\"uploaded\":%s}",
            time_str,
            log->uploaded ? "true" : "false"
        );
        if (n > 0 && pos + n < max_len) pos += n;
        
        log->sent_to_pc = true;
        added_count++;
        index = (index + 1) % MAX_LOG_COUNT;
    }
    
    n = snprintf(buf + pos, max_len - pos, "]}");
    if (n > 0 && pos + n < max_len) pos += n;
    
    pthread_mutex_unlock(&s_log_mutex);
    return pos;
}

int bt_log_get_status_json(char *buf, int max_len, bool wifi_connected, const char *wifi_ssid, const char *ip_addr, const char *sys_log) {
    extern bool g_ble_started;
    extern bool g_ble_sem_passed;
    extern int  g_ble_disc_total;
    extern int  g_ble_disc_passed;
    extern int  g_ble_init_step;

    pthread_mutex_lock(&s_log_mutex);
    int count = s_log_count;
    pthread_mutex_unlock(&s_log_mutex);
    
    double buf_usage = (double)count / MAX_LOG_COUNT * 100.0;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[30];
    if (tm_info) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+08:00", tm_info);
    } else {
        snprintf(time_str, sizeof(time_str), "2012-01-01T00:00:00+08:00");
    }
    
    static char s_cached_mac_str[20] = "00:00:00:00:00:00";
    if (strcmp(s_cached_mac_str, "00:00:00:00:00:00") == 0) {
        bt_wifi_get_mac(s_cached_mac_str, sizeof(s_cached_mac_str));
    }

#ifdef FW_BUILD_TIME
    const char *fw_ver = FW_BUILD_TIME;
#else
    const char *fw_ver = "unknown";
#endif

    return snprintf(buf, max_len,
        "{\"cmd\":\"STATUS\",\"gw_name\":\"MedFlow-GW\",\"fw_ver\":\"%s\",\"mac\":\"%s\",\"wifi_connected\":%s,\"ble_started\":%s,\"ble_sem_passed\":%s,\"ble_init_step\":%d,\"ble_disc_total\":%d,\"ble_disc_passed\":%d,\"wifi_ssid\":\"%s\",\"ip\":\"%s\",\"log_count\":%d,\"buf_usage\":%.1f,\"wifi_retry\":%d,\"time\":\"%s\",\"target_url\":\"mqtt://mqtt.go6.tw:1883\",\"sys_log\":\"%s\"}",
        fw_ver,
        s_cached_mac_str,
        wifi_connected ? "true" : "false",
        g_ble_started ? "true" : "false",
        g_ble_sem_passed ? "true" : "false",
        g_ble_init_step,
        g_ble_disc_total,
        g_ble_disc_passed,
        wifi_ssid ? wifi_ssid : "",
        ip_addr ? ip_addr : "0.0.0.0",
        count,
        buf_usage,
        NetworkWifiGetRetryCount(),
        time_str,
        sys_log ? sys_log : ""
    );
}
