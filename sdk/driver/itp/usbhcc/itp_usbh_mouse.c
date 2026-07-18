#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_hid.h"
#include "usbhcc/api/api_usbh_hid_mouse.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"

#define QUEUE_LEN 256

static QueueHandle_t queue;
static ITPMouseEvent ev;
static ITPMouseEvent last_ev;
static int usb_mouse_insert = 0;

static int usbh_hid_mouse_connntf(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    (void)uid;

    if (ntf == USBH_NTF_CONNECT)
    {
        usb_mouse_insert = 1;
    }
    else
    {
        usb_mouse_insert = 0;
    }

    return 0;
}

static int usbh_hid_mouse_rxntf(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    t_mouse_report  report;
    (void)ntf;

    if ( usbh_hid_mouse_get_report(uid, &report) == USBH_SUCCESS )
    {
        ev.flags = report.buttons;
        ev.x = report.dx;
        ev.y = report.dy;
        ev.wheel = report.dwheel;
        if(last_ev.flags & 0x0F)
        {
            for(int i=0; i<4; i++)
            {
                if((last_ev.flags & (1<<i)) && !(ev.flags & (1<<i)))
                    ev.flags |= (1<<(i+4));
            }
        }

        if(queue != NULL)
        {
            if(!ithCpuInInterrupt())
            {
                if(!(xQueueSend(queue, &ev, 0)))
                    ithPrintf(" Mouse: queue full! \n");
            }
            else
            {
                if(!(xQueueSendFromISR(queue, &ev, 0)))
                    ithPrintf(" Mouse: queue full! \n");
            }
        }
#if 0
        ithPrintf("mouse: (%d, %d, %d) %s%s%s%s %s%s%s%s \r\n",
                    ev.x, ev.y, ev.wheel,
                    (ev.flags & ITP_MOUSE_LBTN_DOWN) ? "L-D " : "",
                    (ev.flags & ITP_MOUSE_RBTN_DOWN) ? "R-D " : "",
                    (ev.flags & ITP_MOUSE_MBTN_DOWN) ? "M-D " : "",
                    (ev.flags & ITP_MOUSE_SBTN_DOWN) ? "S-D " : "",
                    (ev.flags & ITP_MOUSE_LBTN_UP) ? "L-U " : "",
                    (ev.flags & ITP_MOUSE_RBTN_UP) ? "R-U " : "",
                    (ev.flags & ITP_MOUSE_MBTN_UP) ? "M-U " : "",
                    (ev.flags & ITP_MOUSE_SBTN_UP) ? "S-U " : "");
#endif
        memcpy((void*)&last_ev, (void*)&ev, sizeof(ITPMouseEvent));
    }

    return 0;
}

int itp_usbh_mouse_init()
{
    int rc;

    rc = usbh_hid_mouse_register_ntf(0, USBH_NTF_CONNECT, usbh_hid_mouse_connntf);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hid_mouse_register_ntf() USBH_NTF_CONNECT fail! \n");
        goto end;
    }
    
    rc = usbh_hid_mouse_register_ntf(0, USBH_NTF_HID_MOUSE_RX, usbh_hid_mouse_rxntf);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hid_mouse_register_ntf() USBH_NTF_CONNECT fail! \n");
        goto end;
    }
    
    itpRegisterDevice(ITP_DEVICE_USBMOUSE, &itpDeviceUsbMouse);
    
    queue = xQueueCreate(QUEUE_LEN, (unsigned portBASE_TYPE) sizeof(ITPMouseEvent));
    if(queue == NULL)
    {
        ITP_USB_ERR(" Mouse: create queue fail! \n");
    }
    
end:
    return rc;
}

static int UsbMouseRead(int file, char *ptr, int len, void* info)
{
    if(queue == NULL)
    {
        ITP_USB_ERR(" Mouse: queue not created??? \n");
        errno = (ITP_DEVICE_USBMOUSE << ITP_DEVICE_ERRNO_BIT) | 2;
        return -1;
    }

    if (xQueueReceive(queue, ptr, 0))
        return sizeof (ITPMouseEvent);

    return 0;
}

static int UsbMouseIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
    case ITP_IOCTL_IS_AVAIL:
        return usb_mouse_insert;

    default:
        errno = (ITP_DEVICE_USBMOUSE << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbMouse =
{
    ":usbmouse",
    itpOpenDefault,
    itpCloseDefault,
    UsbMouseRead,
    itpWriteDefault,
    itpLseekDefault,
    UsbMouseIoctl,
    NULL
};
