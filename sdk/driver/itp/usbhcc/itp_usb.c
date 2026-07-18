#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "itp_usb_cfg.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "ite/ite_usbex.h"
#include "usbhcc/api/api_hcc_mem.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_mst.h"
#include "usbhcc/api/api_scsi.h"
#include "usbhcc/api/api_usbh_hub.h"
#include "usbhcc/api/api_usbh_ehci.h"
#if defined(CFG_USBH_CD_PRINTER)
#include "usbhcc/api/api_usbh_printer.h"
#endif
#if defined(CFG_USBH_CD_RNDIS)
#include "usbhcc/api/api_usbh_rndis.h"
#endif
#if defined(CFG_USBH_CD_HID) || defined(CFG_USBH_CD_MOUSE_ITU)
#include "usbhcc/api/api_usbh_hid.h"
#endif

static int usb_init_ok = 0;
int mst_insert = 0;

extern int itp_usbh_init(void);
extern int itp_usbh_stop(void);
extern int itp_usbh_exit(void);

#if defined(CFG_USBH_CD_MST)
extern int itp_usbh_mst_init(void);
extern int itp_usbh_mst_stop(void);
extern int itp_usbh_mst_exit(void);
#endif

#if defined(CFG_USBH_CD_HUB)
extern int itp_usbh_hub_init(void);
extern int itp_usbh_hub_stop(void);
extern int itp_usbh_hub_exit(void);
#endif

#if defined(CFG_USBH_CD_CCID)
extern int usbh_ccid_start(void);
extern int itp_usbh_ccid_init(void);
extern int itp_usbh_ccid_stop(void);
extern int itp_usbh_ccid_exit(void);
#endif

#if defined(CFG_USBH_CD_HID)
extern int itp_usbh_hid_init(void);
#endif

#if defined(CFG_USBH_CD_MOUSE_ITU)
extern int itp_usbh_mouse_init(void);
#endif

#if defined(CFG_USBH_CD_CDCACM)
extern int usbh_cdcacm_start(void);
extern int itp_usbh_cdcacm_init(void);
extern int itp_usbh_cdcacm_stop(void);
extern int itp_usbh_cdcacm_exit(void);
#endif

#if defined(CFG_USBH_CD_CDCECM_HCC)
extern int usbh_cdcecm_start(void);
extern int itp_usbh_cdcecm_init(void);
extern int itp_usbh_cdcecm_stop(void);
extern int itp_usbh_cdcecm_exit(void);
#endif

#if defined(CFG_USBH_CD_BULK_RAW)
extern int usbh_raw_start(void);
extern int itp_usbh_raw_init(void);
extern int itp_usbh_raw_stop(void);
extern int itp_usbh_raw_exit(void);
#endif

#if defined(CFG_USBH_CD_UVC)
extern int usbh_uvc_start(void);
extern int usbh_uvc_init(void);
extern int usbh_uvc_stop(void);
extern int usbh_uvc_delete(void);
#endif

/*
* Start USB host and class drivers.
*/

int itp_usbh_chk_isMouseItu()
{
#if defined(CFG_USBH_CD_MOUSE_ITU)
    return 1;
#else
    return 0;
#endif
}

int usb_host_start(void)
{
    int rc;

#if defined(CFG_USBH_CD_MST)
    rc = usbh_mst_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_mst_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HUB)
    rc = usbh_hub_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hub_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CCID)
    rc = usbh_ccid_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_ccid_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HID) || defined(CFG_USBH_CD_MOUSE_ITU)
    rc = usbh_hid_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hid_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCACM)
    rc = usbh_cdcacm_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_cdcacm_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCECM_HCC)
    rc = usbh_cdcecm_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_cdcecm_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_PRINTER)
    /* Start Printer class driver module */
    rc = usbh_prn_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_prn_start()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_RNDIS)
    /* Start RNDIS host class driver */
    rc = usbh_rndis_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_rndis_start()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_BULK_RAW)
    rc = usbh_raw_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_raw_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_UVC)
    rc = usbh_uvc_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_uvc_start() fail! \n");
        goto end;
    }
#endif

    rc = usbh_hc_start(0);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_start(0) fail! \n");
        goto end;
    }
#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_start(1);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_start(1) fail! \n");
        goto end;
    }
#endif

end:
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

/*
* Stop USB host and class drivers.
*/
int usb_host_stop(void)
{
    int rc;

    rc = itp_usbh_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }

#if defined(CFG_USBH_CD_MST)
    rc = itp_usbh_mst_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HUB)
    rc = itp_usbh_hub_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CCID)
    rc = itp_usbh_ccid_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HID) || defined(CFG_USBH_CD_MOUSE_ITU)
    rc = usbh_hid_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCACM)
    rc = itp_usbh_cdcacm_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCECM_HCC)
    rc = itp_usbh_cdcecm_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_PRINTER)
    /* stop Printer class driver module */
    rc = usbh_prn_stop();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_prn_stop()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_RNDIS)
    /* stop RNDIS host class driver */
    rc = usbh_rndis_stop();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_rndis_stop()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_BULK_RAW)
    rc = itp_usbh_raw_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_UVC)
    rc = usbh_uvc_stop();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

end:
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

static int UsbInit(void)
{
    int rc;

    ithUsbReset();
    ithUsbInterfaceSel(ITH_USB_WRAP);

    rc = hcc_mem_init();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("hcc_mem_init() fail! \n");
        goto end;
    }

    rc = itp_usbh_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }

#if defined(CFG_USBH_CD_MST)
    rc = itp_usbh_mst_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HUB)
    rc = itp_usbh_hub_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CCID)
    rc = itp_usbh_ccid_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HID) || defined(CFG_USBH_CD_MOUSE_ITU)
    rc = usbh_hid_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#if defined(CFG_USBH_CD_HID)
    rc = itp_usbh_hid_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif
#if defined(CFG_USBH_CD_MOUSE_ITU)
    rc = itp_usbh_mouse_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif
#endif

#if defined(CFG_USBH_CD_CDCACM)
    rc = itp_usbh_cdcacm_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCECM_HCC)
    rc = itp_usbh_cdcecm_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_PRINTER)
    /* Initialize Printer class driver module */
    rc = usbh_prn_init();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_prn_init()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_RNDIS)
    /* initializes RNDIS host class driver */
    rc = usbh_rndis_init();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_rndis_init()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_BULK_RAW)
    rc = itp_usbh_raw_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_UVC)
    rc = usbh_uvc_init();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

    rc = usb_host_start();
    if (rc == USBH_SUCCESS)
    {
        usb_init_ok = 1;
    }

end:
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

static int UsbExit(void)
{
    int rc;

#if defined(CFG_USBH_CD_MST)
    rc = itp_usbh_mst_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HUB)
    rc = itp_usbh_hub_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CCID)
    rc = itp_usbh_ccid_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_HID) || defined(CFG_USBH_CD_MOUSE_ITU)
    rc = usbh_hid_delete();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCACM)
    rc = itp_usbh_cdcacm_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_CDCECM_HCC)
    rc = itp_usbh_cdcecm_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_PRINTER)
    /* delete Printer class driver module */
    rc = usbh_prn_delete();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_prn_delete()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_RNDIS)
    /* delete RNDIS host class driver */
    rc = usbh_rndis_delete();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_rndis_delete()=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_BULK_RAW)
    rc = itp_usbh_raw_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

#if defined(CFG_USBH_CD_UVC)
    rc = usbh_uvc_delete();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }
#endif

    rc = itp_usbh_exit();
    if (rc != USBH_SUCCESS)
    {
        goto end;
    }

    rc = hcc_mem_delete();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("hcc_mem_delete() fail! \n");
        goto end;
    }

end:
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc );
    }
    return rc;
}

#if defined(CFG_USBHCC_DEVICE)
extern ITHUsbModule itp_usbd_base;
#endif

static void UsbGetInfo(ITPUsbInfo *usbInfo)
{
    if (usbInfo->host) 
    {
        usbInfo->ctxt = NULL;
        usbInfo->type = 0;

        /* it's meaningless */
        if (/*(usbInfo->usbIndex == 0) && */mst_insert) 
        {
            usbInfo->ctxt = (void*)1;
            usbInfo->type = USB_DEVICE_TYPE_MSC;
        }
    }
#if defined(CFG_USBHCC_DEVICE)
    else
    {
        usbInfo->b_device = ithUsbIsDeviceMode(itp_usbd_base);
    }
#endif
}

static void UsbSleep(int idx)
{
    int rc;

    rc = usb_host_stop();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usb_host_stop() fail! rc: %d \n", rc);
    }

    return;
}

static void UsbResume(int idx)
{
    int rc;

    rc = usb_host_start();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usb_host_start() fail! rc: %d \n", rc);
    }

	return;
}

static int UsbIoctl(int file, unsigned long request, void *ptr, void *info)
{
    int res;

    switch (request)
    {
    case ITP_IOCTL_IS_AVAIL:
        return usb_init_ok;

    case ITP_IOCTL_INIT:
        res = UsbInit();
        if (res)
        {
            errno = (ITP_DEVICE_USB << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;

    case ITP_IOCTL_EXIT:
        res = UsbExit();
        if (res)
        {
            errno = (ITP_DEVICE_USB << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;

    case ITP_IOCTL_GET_INFO:
        UsbGetInfo((ITPUsbInfo *)ptr);
        break;

    case ITP_IOCTL_IS_CONNECTED:
        if ((int)ptr == 0)
        {
            return ithUsbDevicePresent(ITH_USB0);
        }
        #if (CFG_CHIP_FAMILY == 970)
        else if ((int)ptr == 1)
        {
            return ithUsbDevicePresent(ITH_USB1);
        }
        #endif
        else
        {
            return 0;
        }

    case ITP_IOCTL_SLEEP:
    case ITP_IOCTL_HIBERNATION:
        UsbSleep((int)ptr);
        break;

    case ITP_IOCTL_RESUME:
        UsbResume((int)ptr);
        break;

#if defined(CFG_USBHCC_DEVICE)
    case ITP_IOCTL_ENABLE:
        /** sample code:
         * ioctl(ITP_DEVICE_USB, ITP_IOCTL_ENABLE, (void *)ITP_USB_FORCE_DEVICE_MODE);
         */
        if (((int)ptr) == ITP_USB_FORCE_DEVICE_MODE)
        {
            ithUsbForceDeviceMode(itp_usbd_base, true);
        }
        break;

    case ITP_IOCTL_DISABLE:
        /** sample code:
         * ioctl(ITP_DEVICE_USB, ITP_IOCTL_DISABLE, (void *)ITP_USB_FORCE_DEVICE_MODE);
         */
        if (((int)ptr) == ITP_USB_FORCE_DEVICE_MODE)
        {
            ithUsbForceDeviceMode(itp_usbd_base, false);
        }
        break;
#endif

    default:
        errno = (ITP_DEVICE_USB << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsb =
{
    ":usb",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbIoctl,
    NULL
};
