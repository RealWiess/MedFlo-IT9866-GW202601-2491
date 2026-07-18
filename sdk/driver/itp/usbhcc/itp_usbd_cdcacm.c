#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_cdcacm.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

#define LINE_CNT            1
#define ACM_WBUF_SIZE       (128*1024)

struct usbd_cdcacm_ctxt {
    uint8_t *acm_wbuf;
    volatile int   connect;
};

static struct usbd_cdcacm_ctxt g_line[LINE_CNT];

static void usbd_cdcacm_cb(const uint8_t line, const t_usbd_cdcacm_ntf_type ntf_type)
{
    if (line > LINE_CNT) 
    {
        ITP_USB_ERR("usbd_cdcacm_cb() line index error! (%d) \n", line);
        return;
    }

    switch (ntf_type) 
    {
    case USBD_CDCACM_NTF_CONNECT:
        (void)printf("line %d connect \n", line);
        g_line[line].connect = 1;
        break;

    case USBD_CDCACM_NTF_DISCONNECT:
        (void)printf("line %d disconnect \n", line);
        g_line[line].connect = 0;
        break;

    default:
        /* no action */
        break;
    }

    return;
}

int itp_usbd_cd_cdcacm_init(void)
{
    int i, rc;

    for (i = 0; i < LINE_CNT; i++) 
    {
        g_line[i].acm_wbuf = (uint8_t*)itpVmemAlloc(ACM_WBUF_SIZE);
        if (!g_line[i].acm_wbuf) 
        {
            ITP_USB_ERR("alloc ACM write buffer fail! \n");
            rc = -1;
            goto end;
        }
    }

    rc = usbd_cdcacm_init();
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_init() fail! \n");
        goto end;
    }
    rc = usbd_cdcacm_start();
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_start() fail! \n");
        goto end;
    }
    rc = usbd_cdcacm_reg_ntf_fn(USBD_CDCACM_NTF_CONNECT, usbd_cdcacm_cb);
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_reg_ntf_fn() fail! USBD_CDCACM_NTF_CONNECT \n");
        goto end;
    }
    rc = usbd_cdcacm_reg_ntf_fn(USBD_CDCACM_NTF_DISCONNECT, usbd_cdcacm_cb);
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_reg_ntf_fn() fail! USBD_CDCACM_NTF_DISCONNECT \n");
        goto end;
    }
    #if defined(CFG_USBD_CD_CDCACM_RAW)
    rc = usbd_cdcacm_set_rx_mode (0, USBD_CDCACM_RXMODE_DIRECT);
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_set_rx_mode() fail! USBD_CDCACM_RXMODE_DIRECT \n");
        goto end;
    }
    #endif

    #if !defined(CFG_USBD_COMPOSITE)
    rc = usbd_start((usbd_config_t *)&device_cfg_cdc_acm);
    if (rc) 
    {
        ITP_USB_ERR("usbd_start() fail! \n");
        goto end;
    }
    #endif

end:
    if (rc) 
    {
        for (i = 0; i < LINE_CNT; i++) 
        {
            if (g_line[i].acm_wbuf) 
            {
                itpVmemFree((uint32_t)g_line[i].acm_wbuf);
                g_line[i].acm_wbuf = NULL;
            }
        }

        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

int itp_usbd_cd_cdcacm_exit(void)
{
    int i, rc;

    rc = usbd_cdcacm_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_stop() fail! \n");
        goto end;
    }

    rc = usbd_cdcacm_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbd_cdcacm_delete() fail! \n");
        goto end;
    }

end:
    for (i = 0; i < LINE_CNT; i++) 
    {
        if (g_line[i].acm_wbuf) 
        {
            itpVmemFree((uint32_t)g_line[i].acm_wbuf);
            g_line[i].acm_wbuf = NULL;
        }
    }
    if (rc)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

static int UsbdAcmRead(int file, char *ptr, int len, void *info)
{
    int rc;
    uint8_t line = 0;
    uint32_t actual_len = 0;

    if (!g_line[line].connect)
    {
        return 0;
    }

    rc = usbd_cdcacm_receive(line, (uint8_t * const)ptr, (uint32_t)len, (uint32_t * const)&actual_len);
    while (rc == USBD_CDCACM_BUSY) 
    {
        (void)usleep(1000);
        rc = usbd_cdcacm_receive(line, (uint8_t * const)ptr, (uint32_t)len, &actual_len);
    }
    if (rc)
    {
        ITP_USB_ERR("usbd_cdcacm_receive() fail! rc %d \n",rc);
    }

    ITP_USB_DBG("UsbdAcmRead: %" PRIu32 "/%d \n", actual_len, len);

    return (int)actual_len;
}

static int UsbdAcmWrite(int file, char *ptr, int len, void *info)
{
    int rc = 0;
    uint8_t line = 0;

#if defined(CFG_DBG_USBHCC)
    extern ITHUsbModule itp_usbd_base;
    if (ithUsbIsDeviceMode(itp_usbd_base) == false)
    {
        goto exit;
    }
#endif

    if (!g_line[line].connect)
    {
        goto exit;
    }

    if (!g_line[line].acm_wbuf)
    {
       goto exit;
    }

    if (len > ACM_WBUF_SIZE) 
    {
        ITP_USB_ERR("acm write len(%d) > %d \n", len, ACM_WBUF_SIZE);
        goto exit;
    }

    /* Check last TX finished */
    while (usbd_cdcacm_send_status(line) == USBD_CDCACM_BUSY)
    {
        (void)usleep(1000);
    }

    memcpy((void*)g_line[line].acm_wbuf, (void*)ptr, (size_t)len);
    rc = usbd_cdcacm_send(line, g_line[line].acm_wbuf, (uint32_t)len);
    if (rc)
    {
        ITP_USB_ERR("usbd_cdcacm_send() fail! rc %d \n", rc);
    }

    ITP_USB_DBG("UsbdAcmWrite: %d \n", len);

    return (rc != 0) ? 0 : len;

exit:
    return rc;
}

extern ITHUsbModule itp_usbd_base;

static int UsbdAcmIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int line = 0;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        break;

    case ITP_IOCTL_ENABLE:
        break;

    case ITP_IOCTL_DISABLE:
        break;

    case ITP_IOCTL_IS_CONNECTED:
        /* currently only support one line */
        return g_line[line].connect;

    default:
        errno = (ITP_DEVICE_USBDACM << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}


const ITPDevice itpDeviceUsbdAcm =
{
    ":usbd acm",
    itpOpenDefault,
    itpCloseDefault,
    UsbdAcmRead,
    UsbdAcmWrite,
    itpLseekDefault,
    UsbdAcmIoctl,
    NULL
};


#if defined(CFG_DBG_USBHCC)

#define LOG_BUF_SIZE    (CFG_DBG_USBHCC_BUFSIZE)

static pthread_t usbd_console_task;
static sem_t console_sem;
static sem_t task_sem;
static volatile int exit;
static char log_buf[LOG_BUF_SIZE];  /* for keep isr log */
static volatile int console_idx_w;
static int console_idx_r;

static int UsbdAcmConsolePutchar(int c);

static void* UsbdAcmConsoleHandler(void* arg)
{
    sem_post(&task_sem);

    while (exit == 0)
    {
        sem_wait(&console_sem);

        if (exit == 0)
        {
            int len;
            int cur_idx_w = console_idx_w;

            if (console_idx_r > cur_idx_w)
            {
                len = LOG_BUF_SIZE - console_idx_r;
                write(ITP_DEVICE_USBDACM, (log_buf + console_idx_r), len);
                console_idx_r = 0;
            }

            if (console_idx_r < cur_idx_w)
            {
                len = cur_idx_w - console_idx_r;

                write(ITP_DEVICE_USBDACM, (log_buf + console_idx_r), len);
                console_idx_r += len;
                if (console_idx_r >= LOG_BUF_SIZE)
                {
                    console_idx_r = 0;
                }
            }
        }

    }

    return NULL;
}

static int UsbdAcmConsoleInit(void)
{
    int rc;
    pthread_attr_t attr;
    struct sched_param param;
    
    exit = 0;
    console_idx_w = 0;
    console_idx_r = 0;
    sem_init(&task_sem, 0, 0);
    sem_init(&console_sem, 0, 0);

    pthread_attr_init(&attr);
    param.sched_priority = 4;
    pthread_attr_setschedparam(&attr, &param);
    rc = pthread_create(&usbd_console_task, &attr, UsbdAcmConsoleHandler, NULL);
    if (rc != 0)
    {
        ITP_USB_ERR(" create usb device console thread fail! 0x%08X \n", rc);
        goto end;
    }

    ithPutcharFunc = UsbdAcmConsolePutchar;
    sem_wait(&task_sem);

end:
    return rc;
}

static int UsbdAcmConsoleExit(void)
{
    int rc;

    exit = 1;
    sem_post(&console_sem);
    pthread_join(usbd_console_task, NULL);
    sem_destroy(&console_sem);
    sem_destroy(&task_sem);

    return 0;
}

static int UsbdAcmConsolePutchar(int c)
{
    char cc = (char)c;

    ithEnterCritical();

    if (console_idx_w >= LOG_BUF_SIZE)
    {
        console_idx_w = 0;
    }

    log_buf[console_idx_w++] = (char)c;
    sem_post(&console_sem);

    ithExitCritical();

    return c;
}

static int UsbdAcmConsoleWrite(int file, char *ptr, int len, void *info)
{
    int rc, i;

    ithEnterCritical();

    for (i = 0; i < len; i++)
    {
        if (console_idx_w >= LOG_BUF_SIZE)
        {
            console_idx_w = 0;
        }
        log_buf[console_idx_w++] = ptr[i];
    }
    sem_post(&console_sem);
    rc = len;

    ithExitCritical();

    return rc;
}

static int UsbdAcmConsoleIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int rc;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        rc = UsbdAcmConsoleInit();
        break;

    case ITP_IOCTL_EXIT:
        rc = UsbdAcmConsoleExit();
        break;

    default:
        errno = (ITP_DEVICE_USBDCONSOLE << ITP_DEVICE_ERRNO_BIT) | 1;
        rc = -1;
        break;
    }

    return rc;
}


const ITPDevice itpDeviceUsbdConsole =
{
    ":usbd console",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    UsbdAcmConsoleWrite,
    itpLseekDefault,
    UsbdAcmConsoleIoctl,
    NULL
};

#endif
