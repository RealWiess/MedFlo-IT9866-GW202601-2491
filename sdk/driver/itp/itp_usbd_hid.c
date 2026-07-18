/**
 * @file itp_usbd_hid.c
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief
 * @version 0.1
 * @date 2019-03-14
 *
 * @copyright Copyright (c) 2019 ITE Tech. Inc. All Rights Reserved.
 *
 */
#include "ite/itp.h"
#include <linux/usb/g_hid.h>

#define	ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */

extern int iteHiddInitialize(void);
extern int iteHiddTerminate(void);
extern int iteUdcRestart(void);
extern int iteUdcResetPlug(void);
extern int iteUdcSetRmWakup(void);
extern int iteUsbCompositeOverwrite(struct usb_composite_overwrite *covr);

static uint8_t gUsbdHidConnected = 0;
pthread_mutex_t gUsbdHidMutex  = PTHREAD_MUTEX_INITIALIZER;

static int UsbdHidRead(int file, char *ptr, int len, void* info)
{
    if(gUsbdHidConnected)
    	return hidg_read(ptr, len);
    return 0;
}

static int UsbdHidWrite(int file, char *ptr, int len, void* info)
{
    if(gUsbdHidConnected) {
        int ret = hidg_write(ptr, len);
    #if defined(CFG_USBD_HID_MOUSE) || defined(CFG_USBD_HID_KBD)
        if(ret == -ESHUTDOWN) {
            if(iteUdcSetRmWakup()) {
                usleep(1000*1000*1);
                ret = hidg_write(ptr, len);
            }
        }
    #endif
        return ret;
    }
    return 0;
}

static int UsbdHidIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
    case ITP_IOCTL_INIT:
        iteHiddInitialize();
        break;

    case ITP_IOCTL_USBD_HIDG_REFRESH_CONFIG:
        iteUdcRestart();
        break;

    case ITP_IOCTL_USBD_HIDG_SET_CONFIG:
        return hidg_set_config((HIDG_FUNC_CONFIG *) ptr);

    case ITP_IOCTL_USBD_HIDG_READ:
        if(gUsbdHidConnected)
            return hidg_read_by_ID((HIDG_FUNC_CMD_DATA*) ptr);
        break;

    case ITP_IOCTL_USBD_HIDG_WRITE:
        if(gUsbdHidConnected) {
            int ret = hidg_write_by_ID((HIDG_FUNC_CMD_DATA*) ptr);
    #if defined(CFG_USBD_HID_MOUSE) || defined(CFG_USBD_HID_KBD)
            if(ret == -ESHUTDOWN) {
                if(iteUdcSetRmWakup()) {
                    usleep(1000*1000*1);
                    ret = hidg_write_by_ID((HIDG_FUNC_CMD_DATA*) ptr);
                }
            }
    #endif
            return ret;
        }
        break;

    case ITP_IOCTL_USBD_HIDG_OVERWRITE_VIDPID:
        iteUsbCompositeOverwrite((struct usb_composite_overwrite*) ptr);
        break;        

    case ITP_IOCTL_USBD_HIDG_RESETPLUG:
        iteUdcResetPlug();
        break;

    case ITP_IOCTL_ENABLE:
        gUsbdHidConnected = 1;
        break;

    case ITP_IOCTL_DISABLE:
        gUsbdHidConnected = 0;
        break;

    case ITP_IOCTL_IS_CONNECTED:
        return gUsbdHidConnected;

    case ITP_IOCTL_EXIT:
        iteHiddTerminate();
        break;
    }
    return 0;
}

const ITPDevice itpDeviceUsbdHid =
{
    ":usbd hid",
    itpOpenDefault,
    itpCloseDefault,
    UsbdHidRead,
    UsbdHidWrite,
    itpLseekDefault,
    UsbdHidIoctl,
    NULL
};
