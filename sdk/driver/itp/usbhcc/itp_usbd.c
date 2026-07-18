#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/config/config_usbd_config.h"
#if defined(CFG_USBD_CD_PRINTER)
#include "usbhcc/api/api_usbd_prn.h"
#endif

#if defined(CFG_USBD_CD_CDCACM) || defined(HAVE_USBD_CD_CDCACM)
extern int itp_usbd_cd_cdcacm_init(void);
extern int itp_usbd_cd_cdcacm_exit(void);
#endif

#if defined(CFG_USBD_CD_CDCNCM)
extern int itp_usbd_cd_cdcncm_init(void);
extern int itp_usbd_cd_cdcncm_exit(void);
#endif

#if defined(CFG_USBD_CD_CDCECM)
extern int itp_usbd_cd_cdcecm_init(void);
extern int itp_usbd_cd_cdcecm_exit(void);
#endif

#if defined(CFG_USBD_CD_MST)
extern int itp_usbd_cd_mst_init(void);
extern int itp_usbd_cd_mst_exit(void);
#endif

#if defined(CFG_USBD_CD_BULK_RAW) || defined(HAVE_USBD_CD_BULK_RAW)
extern int itp_usbd_cd_raw_init(void);
extern int itp_usbd_cd_raw_exit(void);
#endif

#if defined(CFG_USBD_CD_HID)
extern int itp_usbd_hid_init(void);
extern int itp_usbd_hid_exit(void);
#endif


extern ITHUsbModule itp_usbd_base;
extern uint8_t itp_usbd_idx;



static int usb_device_init(void)
{
    int rc;

    rc = usbd_init();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_init() fail! \n");
        goto end;
    }

#if defined(CFG_USBD_CD_CDCACM) || defined(HAVE_USBD_CD_CDCACM)
    rc = itp_usbd_cd_cdcacm_init();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("itp_usbd_cd_cdcacm_init() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_CDCNCM)
    rc = itp_usbd_cd_cdcncm_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("itp_usbd_cd_cdcncm_init() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_CDCECM)
    rc = itp_usbd_cd_cdcecm_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("itp_usbd_cd_cdcecm_init() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_MST)
    rc = itp_usbd_cd_mst_init();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("itp_usbd_cd_mst_init() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_BULK_RAW) || defined(HAVE_USBD_CD_BULK_RAW)
    rc = itp_usbd_cd_raw_init();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("itp_usbd_cd_raw_init() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_HID)
    rc = itp_usbd_hid_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usb_device_cd_hid_init() fail! \n");
        goto end;
    }

	rc = usbd_start((usbd_config_t *)&device_cfg_hid_gen);
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_start() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_PRINTER)
    /* Init Printer class driver module */
    rc = usbd_prn_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_prn_init()=%d\n", rc);
        goto end;
    }

    /* Start Printer class driver module */
    rc = usbd_prn_start();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_prn_start()=%d\n", rc);
        goto end;
    }

    rc = usbd_start((usbd_config_t *)&device_cfg_printer);
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_start(&device_cfg_printer)=%d\n", rc);
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_CDCACM_RAW)
    rc = usbd_start((usbd_config_t *)&device_cfg_cdc_acm_raw);
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_start(&device_cfg_cdc_acm_raw) fail! rc:%d \n", rc);
        goto end;
    }
#endif

end:
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

static int usb_device_exit(void)
{
    int rc;

    rc = usbd_stop();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_stop() fail! \n");
        goto end;
    }

    rc = usbd_delete();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_delete() fail! \n");
        goto end;
    }

#if defined(CFG_USBD_CD_CDCACM) || defined(HAVE_USBD_CD_CDCACM)
    rc = itp_usbd_cd_cdcacm_exit();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usb_device_cd_cdcacm_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_CDCNCM)
    rc = itp_usbd_cd_cdcncm_exit();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("itp_usbd_cd_cdcncm_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_CDCECM)
    rc = itp_usbd_cd_cdcecm_exit();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("itp_usbd_cd_cdcecm_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_MST)
    rc = itp_usbd_cd_mst_exit();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("itp_usbd_cd_mst_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_BULK_RAW) || defined(HAVE_USBD_CD_BULK_RAW)
    rc = itp_usbd_cd_raw_exit();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("itp_usbd_cd_raw_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_HID)
    rc = itp_usbd_hid_exit();
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usb_device_cd_hid_exit() fail! \n");
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_PRINTER)
    /* Stop Printer class driver module */
    rc = usbd_prn_stop();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_prn_stop()=%d\n", rc);
        goto end;
    }

    /* Delete Printer class driver module */
    rc = usbd_prn_delete();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_prn_delete()=%d\n", rc);
        goto end;
    }
#endif

end:
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

static pthread_t detect_task;
static sem_t detect_sem;
static int usbd_exit;
volatile bool usbd_connect = false;


#if defined(CFG_DBG_USBHCC)
static void OriStandardIo(void)
{
#if defined(CFG_DBG_PRINTBUF)
	// init print buffer device
	itpRegisterDevice(ITP_DEVICE_STD, &itpDevicePrintBuf);
	ioctl(ITP_DEVICE_PRINTBUF, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART0)
	// init uart device
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart0);
	ioctl(ITP_DEVICE_UART0, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART1)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart1);
	ioctl(ITP_DEVICE_UART1, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART2)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart2);
	ioctl(ITP_DEVICE_UART2, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART3)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart3);
	ioctl(ITP_DEVICE_UART3, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART4)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart4);
	ioctl(ITP_DEVICE_UART4, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART5)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart5);
	ioctl(ITP_DEVICE_UART5, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_SWUART_CODEC)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceSwUartCodecDbg);
	ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_INIT, NULL);
#endif // defined(CFG_DBG_PRINTBUF)
}
#endif

static void* UsbDeviceDetectHandler(void* arg)
{
#if defined(CFG_DBG_USBHCC)
    int acm_dbg = 0;
#endif
    int rc;

    while (1) 
    {
        if (ithUsbIsDeviceMode(itp_usbd_base) == true) 
        {
            if (usbd_connect == false) 
            {
                rc = usbh_hc_stop(itp_usbd_idx);
                if (rc != USBH_SUCCESS) 
                {
                    ITP_USB_ERR("usbh_hc_stop(%d) fail! \n", itp_usbd_idx);
                    goto end;
                }

                (void)printf("enter device mode! \n");
                rc = usb_device_init();
                if (rc != USBD_SUCCESS)
                {
                    ITP_USB_ERR("usb_device_init() fail!\n");
                }

                usbd_vbus(1, 0);
                usbd_connect = true;
            }
        }
        else 
        {
            if (usbd_connect == true) 
            {
                #if defined(CFG_DBG_USBHCC)
                if (acm_dbg == 1) 
                {
                    OriStandardIo();
                    (void)printf("\n original STD I/O.... \n");
                    acm_dbg = 0;
                    ioctl(ITP_DEVICE_USBDCONSOLE, ITP_IOCTL_EXIT, NULL);
                }
                #endif

                (void)printf("leave device mode! \n");
                usbd_connect = false;

                usbd_vbus(0, 0);
                usb_device_exit();

                rc = usbh_hc_start(itp_usbd_idx);
                if (rc != USBH_SUCCESS)
                {
                    ITP_USB_ERR("usbh_hc_start(%d) fail! \n", itp_usbd_idx);
                }
            }
        }

        itpSemWaitTimeout(&detect_sem, 30);
        #if defined(CFG_DBG_USBHCC)
        if (acm_dbg == 0) 
        {
            if (ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_IS_CONNECTED, NULL)) 
            {
                (void)printf("\n use usb console.... \n");
                itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUsbdConsole);
                itpRegisterDevice(ITP_DEVICE_USBDCONSOLE, &itpDeviceUsbdConsole);
                ioctl(ITP_DEVICE_USBDCONSOLE, ITP_IOCTL_INIT, NULL);
                acm_dbg = 1;
            }
        }
        #endif

        if (usbd_exit == 1)
        {
            goto end;
        }
    }

end:
    return NULL;
}

static int UsbdInit(void)
{
    int rc;
    pthread_attr_t attr;

    usbd_exit = 0;
    sem_init(&detect_sem, 0, 0);

    pthread_attr_init(&attr);
    rc = pthread_create(&detect_task, &attr, UsbDeviceDetectHandler, NULL);
    if (rc != 0)
    {
        ITP_USB_ERR(" create usb device detect thread fail! 0x%08X \n", rc);
    }

    return rc;
}

static int UsbdExit(void)
{
    int rc;

    rc = usb_device_exit();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usb_device_exit() fail! \n");
    }

    usbd_exit = 1;
    sem_post(&detect_sem);
    pthread_join(detect_task, NULL);
    sem_destroy(&detect_sem);

    return 0;
}

static int UsbdIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int res;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        res = UsbdInit();
        if (res != 0)
        {
            errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;

    case ITP_IOCTL_EXIT:
        res = UsbdExit();
        break;

    default:
        errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbd =
{
    ":usbd",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdIoctl,
    NULL
};

