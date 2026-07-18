#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbh_raw.h"

#define UID_CNT            1

struct usbh_raw_ctxt {
    int ref;
    volatile int   connect;
    /* If 'len' is equal to buffer size zero-length-packet (ZLP) is not necessary */
    uint8_t skip_zlp;
};

static struct usbh_raw_ctxt g_uid[UID_CNT] = { 0 };

struct raw_interface {
    uint8_t class;
    uint8_t sclass;
    uint8_t protocol;
};

static struct raw_interface ifc_list[] =
{
    { 0xFE, 0x02, 0x08 },  /* ITE */
    { 0x08, 0xFF, 0xFF },  /* Hikvision module */
    {}
};

int raw_ifc_supported(uint8_t class, uint8_t sclass, uint8_t protocol)
{
    const struct raw_interface *ifc = ifc_list;

    for (; ifc->class || ifc->sclass || ifc->protocol; ifc++) 
    {
        if (ifc->class != class)
        {
            continue;
        }
        if (ifc->sclass != sclass)
        {
            continue;
        }
        if (ifc->protocol != protocol)
        {
            continue;
        }

        return 1;
    }

    return 0;

}

static int usbh_raw_cb(t_usbh_unit_id uid, t_usbh_ntf ntf_type)
{
    if (uid >= UID_CNT) 
    {
        ITP_USB_ERR("usbh_raw_cb() uid error! (%d) \n", uid);
        return -1;
    }

    switch (ntf_type) 
    {
    case USBH_NTF_CONNECT:
        (void)printf("[USBH][RAW] uid %d connect \n", uid);
        g_uid[uid].connect = 1;
        break;

    case USBH_NTF_DISCONNECT:
        (void)printf("[USBH][RAW] uid %d disconnect \n", uid);
        g_uid[uid].connect = 0;
        break;

    default:
        break;
    }

    return 0;
}

int itp_usbh_raw_init(void)
{
    int rc;
    uint8_t i;

    rc = usbh_raw_init();
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_init() fail! \n");
        goto end;
    }
    for (i = 0; i < UID_CNT; i++) 
    {
        rc = usbh_raw_register_ntf(i, USBH_NTF_CONNECT, usbh_raw_cb);
        if (rc) 
        {
            ITP_USB_ERR("usbh_raw_register_ntf() fail! \n");
            goto end;
        }
    }
    rc = usbh_raw_start();
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_start() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_raw_stop(void)
{
    int rc;

    rc = usbh_raw_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_stop() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_raw_exit(void)
{
    int rc;

    rc = usbh_raw_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}

static int UsbhRawRead(int file, char *ptr, int len, void *info)
{
    int rc;
    uint8_t uid, ch = 0, b_skip_zlp = 1;
    uint8_t b_block = 0;
    uint32_t actual_len = 0;

    uid = (uint8_t)file;

    rc = usbh_raw_read(uid, ch, (uint8_t *)ptr, (uint32_t)len, b_skip_zlp);
    if (rc == USBH_RAW_SUCCESS) 
    {
        while ((rc = usbh_raw_read_state(uid, ch, b_block, &actual_len)) == USBH_RAW_BUSY);
        if (rc) 
        {
            ITP_USB_ERR("usbh_raw_read_state(uid%" PRIu8 ") fail! rc %d \n", uid, rc);
            goto err;
        }
    }
    else 
    {
        ITP_USB_ERR("usbh_raw_read(uid%d) fail! rc %d \n", uid, rc);
        goto err;
    }

    ITP_USB_DBG("UsbhRawRead(uid%" PRIu8 "): %" PRIu32 "/%d \n", uid, actual_len, len);

    return (int)actual_len;

err:
    return -rc;
}

static int UsbhRawWrite(int file, char *ptr, int len, void *info)
{
    int rc;
    uint8_t uid, ch = 0;
    uint8_t b_block = 0;

    uid = (uint8_t)file;

    if (usbh_raw_present(uid) != TRUE)
    {
        return -1;
    }

    rc = usbh_raw_write(uid, ch, (uint8_t *)ptr, (uint32_t)len, g_uid[uid].skip_zlp);
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_write(uid%d) fail! rc %d \n", uid, rc);
        goto end;
    }

    rc = usbh_raw_write_state(uid, ch, b_block);
    while (rc == USBH_RAW_BUSY)
    {
        rc = usbh_raw_write_state(uid, ch, b_block);
    }
    if (rc) 
    {
        ITP_USB_ERR("usbh_raw_write_state(uid%d) fail! rc %d \n", uid, rc);
        goto end;
    }

end:
    ITP_USB_DBG("UsbhRawWrite: %d \n", len);

    return (rc != 0) ? 0 : len;
}

static int UsbhRawOpen(const char* name, int flags, int mode, void* info)
{
    uint8_t uid = (uint8_t)atoi(name);

    g_uid[uid].ref++;

    ITP_USB_INFO("[UsbhRawOpen] uid:%d \n", uid);
    return uid;
}

static int UsbhRawClose(int file, void* info)
{
    uint8_t uid = (uint8_t)file;

    g_uid[uid].ref--;

    return 0;
}

static int UsbhRawIoctl(int file, unsigned long request, void* ptr, void* info)
{
    uint8_t uid = (uint8_t)file;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        break;

    case ITP_IOCTL_IS_CONNECTED:
        return g_uid[uid].connect;

    case ITP_IOCTL_ENABLE:
        /** sample code:
        * ioctl(fd, ITP_IOCTL_ENABLE, (void *)ITP_USB_SKIP_ZLP);
        */
        if (((int)ptr) == ITP_USB_SKIP_ZLP)
        {
            g_uid[uid].skip_zlp = 1;
        }
        break;

    case ITP_IOCTL_DISABLE:
        /** sample code:
        * ioctl(fd, ITP_IOCTL_DISABLE, (void *)ITP_USB_SKIP_ZLP);
        */
        if (((int)ptr) == ITP_USB_SKIP_ZLP)
        {
            g_uid[uid].skip_zlp = 0;
        }
        break;

    default:
        errno = (ITP_DEVICE_USBHRAW << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}


const ITPDevice itpDeviceUsbhRaw =
{
    ":usbh_raw",
    UsbhRawOpen,
    UsbhRawClose,
    UsbhRawRead,
    UsbhRawWrite,
    itpLseekDefault,
    UsbhRawIoctl,
    NULL
};
