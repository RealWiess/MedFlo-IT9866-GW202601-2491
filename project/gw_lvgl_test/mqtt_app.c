/*
 * mqtt_app.c — BISECT TEST: TCP + write() only
 */
#include "mqtt_app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "ite/itp.h"
#include "wifiMgr.h"

#define MQTT_HOST "mqtt.go6.tw"
#define MQTT_PORT 1883

static void *mqtt_task(void *arg)
{
    (void)arg;
    printf("[MQTT] waiting 10s...\n");
    sleep(10);

    uint8_t mac[6]={0};
    uint8_t *pmac = WifiMgr_Get_WIFI_MAC();
    if (pmac) memcpy(mac, pmac, 6);

    struct hostent *he = gethostbyname(MQTT_HOST);
    if (!he) { printf("[MQTT] DNS FAIL\n"); return NULL; }
    printf("[MQTT] DNS OK\n");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv = {2,0}; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_port = htons(MQTT_PORT);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("[MQTT] TCP fail\n"); close(sock); return NULL;
    }
    printf("[MQTT] TCP OK\n");

    /* Bisect test: write() — does it trigger the same crash as send()? */
    const char *test = "TEST";
    int n = write(sock, test, 4);
    printf("[MQTT] write rc=%d\n", n);
    close(sock);
    return NULL;
}

void mqtt_publish(const char *topic, const char *payload) { (void)topic; (void)payload; }
bool mqtt_app_is_connected(void) { return false; }

void mqtt_app_init(void)
{
    pthread_t tid; pthread_attr_t attr;
    pthread_attr_init(&attr); pthread_attr_setstacksize(&attr, 16384);
    pthread_create(&tid, &attr, mqtt_task, NULL);
    pthread_attr_destroy(&attr);
    printf("[MQTT] init done\n");
}
