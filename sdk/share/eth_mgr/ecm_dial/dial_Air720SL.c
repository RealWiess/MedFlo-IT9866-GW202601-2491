#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "ite/itp.h"
#include "ecmMgr.h"


#define RX_BUF_LEN      (256+128)
#define RX_BUF_LEN_1    512

static pthread_t rx_task;
static char rxbuf[RX_BUF_LEN];
static char rxbuf_1[RX_BUF_LEN_1];
static int rxbuf_1_len;
static int dial_done;
static bool ready = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void* receive_task(void* arg)
{
    int len;

    while (ready == false) usleep(1000);

    for (;;) {
        len = read(ITP_DEVICE_USBOPTION, rxbuf, RX_BUF_LEN);
        pthread_mutex_lock(&mutex);
        if (len > 0) {
            if (len >= RX_BUF_LEN)
                printf("receive_task: len %d >= RX_BUF_LEN %d \n", len, RX_BUF_LEN);
            rxbuf[len] = '\0';
            printf("%s\n", rxbuf);

            if (!dial_done) {
                memcpy((void*)(rxbuf_1 + rxbuf_1_len), rxbuf, len);
                rxbuf_1_len += len;
                if (rxbuf_1_len > RX_BUF_LEN_1)
                    printf("receive_task: rxbuf_1 buffer full! ==> TODO!!! \n");
            }
        }
        pthread_mutex_unlock(&mutex);

        if (len < 0) {
            //printf("receive_task: --....%d \n", len);
            while (!ioctl(ITP_DEVICE_USBOPTION, ITP_IOCTL_IS_CONNECTED, NULL))
                sleep(1);
           // printf("receive_task: ++....\n");
        }
    }

    return NULL;
}


static pthread_t tx_task;
extern int dhcp_ready ;

static void* atcmd_task(void* arg)
{
    char *string;
    int retry, rc = 0;
    uint8_t *ok;

    while (ready == false) usleep(1000);

    string = "ATE1\r";
    retry = 20;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(2);

        ok = strstr(rxbuf_1, "OK");
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -1;
        goto end;
    }

    string = "AT+CPIN?\r";
    retry = 12;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(2);

        ok = strstr(rxbuf_1, "READY");
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -2;
        goto end;
    }

    string = "AT+CREG?\r";
    retry = 12;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(3);

        ok = strstr(rxbuf_1, "OK");  // note: should check value here...
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -4;
        goto end;
    }

    string = "AT+CEREG?\r";
    retry = 12;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(3);

        ok = strstr(rxbuf_1, "OK");  // note: should check value here...
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -4;
        goto end;
    }

do_again:
    string = "AT+CSQ\r";
    retry = 12;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(3);

        ok = strstr(rxbuf_1, "OK");  // note: should check value here...
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -3;
        goto end;
    }

    string = "AT+CGDCONT?\r";
    retry = 12;
    do {
        pthread_mutex_lock(&mutex);
        memset((void*)rxbuf_1, 0, RX_BUF_LEN_1);
        rxbuf_1_len = 0;
        pthread_mutex_unlock(&mutex);

        if (rc = write(ITP_DEVICE_USBOPTION, string, strlen(string)) < 0)
            goto end;

        sleep(3);

        ok = strstr(rxbuf_1, "OK");  // note: should check value here...
    } while ((ok == NULL) && (--retry));
    if (retry <= 0 || !ok) {
        rc = -3;
        goto end;
    }

    usleep(3 * 1000 * 1000);
    if (dhcp_ready) { while (1) sleep(1); }
    goto do_again;

end:
    printf("atcmd_task error...\n");
    usleep(3 * 1000 * 1000);
    goto do_again;

    return NULL;
}

int ecm_dial(void)
{
    dial_done = 0;
    if (!rx_task)
        pthread_create(&rx_task, NULL, receive_task, NULL);

    /* Air720 is auto dial, just creat task for AT cmd test. */
    if (!tx_task)
        pthread_create(&tx_task, NULL, atcmd_task, NULL);

    /* wait usb serial ready */
    printf("wait usb option driver ready!\n");
    while (!ioctl(ITP_DEVICE_USBOPTION, ITP_IOCTL_IS_CONNECTED, NULL)) {
        sleep(1);
        putchar('*');
        fflush(stdout);
    }
    printf("usb option driver ready!\n");

    ioctl(ITP_DEVICE_USBOPTION, ITP_IOCTL_SET_MODE, (void*)ITP_USB_OPTION_IO_MODE);

    ready = true;

    return 0;
}

int dial_check(void)
{
    return 0;
}