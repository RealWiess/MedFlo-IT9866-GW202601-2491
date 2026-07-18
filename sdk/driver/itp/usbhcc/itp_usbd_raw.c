#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_raw.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

#define CH_CNT            1

struct usbd_raw_ctxt {
    int ref;
    /* If 'len' is equal to buffer size zero-length-packet (ZLP) is not necessary */
    uint8_t skip_zlp;
};

static struct usbd_raw_ctxt g_ch[CH_CNT] = { 0 };

int itp_usbd_cd_raw_init(void)
{
    int rc;

    rc = usbd_raw_init();
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_init() fail! \n");
        goto end;
    }
    rc = usbd_raw_start();
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_start() fail! \n");
        goto end;
    }
#if !defined(CFG_USBD_COMPOSITE)
    rc = usbd_start((usbd_config_t *)&device_cfg_bulk);
    if (rc) 
    {
        ITP_USB_ERR("usbd_start() fail! \n");
        goto end;
    }
#endif

end:
    return rc;
}

int itp_usbd_cd_raw_exit(void)
{
    int rc;

    rc = usbd_raw_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_stop() fail! \n");
        goto end;
    }

    rc = usbd_raw_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}

static int UsbdRawRead(int file, char *ptr, int len, void *info)
{
    int rc;
    uint8_t ch, b_block = 1;
    uint32_t actual_len = 0;

    ch = (uint8_t)file;

again:
    rc = usbd_raw_read(ch, (uint8_t *)ptr, (uint32_t)len);
    if (rc == USBD_RAW_SUCCESS) 
    {
        rc = usbd_raw_read_state(ch, b_block, &actual_len);
        if (rc) 
        {
            ITP_USB_ERR("usbd_raw_read_state(ch%d) fail! rc %d \n", ch, rc);
            goto err;
        }
    }
    else if (rc == USBD_RAW_ERROR) 
    {
        ITP_USB_ERR("usbd_raw_read(ch%d) fail! rc %d \n", ch, rc);
        if (usbd_raw_present(ch) == USBD_RAW_SUCCESS) 
        {
            (void)usleep(1000);
            goto again;
        }
    }

    ITP_USB_DBG("UsbdRawRead(ch%" PRIu8 "): %" PRIu32 "/%d \n", ch, actual_len, len);

    return (int)actual_len;

err:
    return -rc;
}

static int UsbdRawWrite(int file, char *ptr, int len, void *info)
{
    int rc;
    uint8_t ch, b_block = 1;

    ch = (uint8_t)file;

    if (usbd_raw_present(ch) != USBD_RAW_SUCCESS)
    {
        return -1;
    }

    rc = usbd_raw_write(ch, (uint8_t *)ptr, (uint32_t)len, g_ch[ch].skip_zlp);
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_write(ch%d) fail! rc %d \n", ch, rc);
        goto end;
    }

    rc = usbd_raw_write_state(ch, b_block);
    if (rc) 
    {
        ITP_USB_ERR("usbd_raw_write_state(ch%d) fail! rc %d \n", ch, rc);
        goto end;
    }

end:
    ITP_USB_DBG("UsbdRawWrite: %d \n", len);

    return (rc != 0) ? 0 : len;
}

static int UsbdRawOpen(const char* name, int flags, int mode, void* info)
{
    uint8_t ch = (uint8_t)atoi(name);

    g_ch[ch].ref++;

    ITP_USB_INFO("[UsbdRawOpen] %s, ch:%d \n", name, ch);
    return ch;
}

static int UsbdRawClose(int file, void* info)
{
    uint8_t ch = (uint8_t)file;

    g_ch[ch].ref--;

    return 0;
}

static int UsbdRawIoctl(int file, unsigned long request, void* ptr, void* info)
{
    uint8_t ch = (uint8_t)file;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        break;

    case ITP_IOCTL_IS_CONNECTED:
        if (usbd_raw_present(ch) == USBD_RAW_SUCCESS)
        {
            return 1;
        }
        else
        {
            return 0;
        }

    case ITP_IOCTL_ENABLE:
        /** sample code:
        * ioctl(fd, ITP_IOCTL_ENABLE, (void *)ITP_USB_SKIP_ZLP);
        */
        if (((int)ptr) == ITP_USB_SKIP_ZLP)
        {
            g_ch[ch].skip_zlp = 1;
        }
        break;

    case ITP_IOCTL_DISABLE:
        /** sample code:
        * ioctl(fd, ITP_IOCTL_DISABLE, (void *)ITP_USB_SKIP_ZLP);
        */
        if (((int)ptr) == ITP_USB_SKIP_ZLP)
        {
            g_ch[ch].skip_zlp = 0;
        }
        break;


    default:
        errno = (ITP_DEVICE_USBDACM << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}


const ITPDevice itpDeviceUsbdRaw =
{
    ":usbd_raw",
    UsbdRawOpen,
    UsbdRawClose,
    UsbdRawRead,
    UsbdRawWrite,
    itpLseekDefault,
    UsbdRawIoctl,
    NULL
};
