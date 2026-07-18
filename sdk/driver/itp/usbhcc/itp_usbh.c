#include <sys/ioctl.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_ehci.h"

#if defined(CFG_USBHCC_DEVICE)
#if defined(CFG_USBD_USB0)
ITHUsbModule itp_usbd_base = ITH_USB0;
uint8_t itp_usbd_idx = 0;
#endif
#if defined(CFG_USBD_USB1)
ITHUsbModule itp_usbd_base = ITH_USB1;
uint8_t itp_usbd_idx = 1;
#endif
#endif

#if defined(CFG_USBH_CD_HUB) && defined(CFG_USBH_CD_HID)
#define EHCI_NO_OF_FAILED_DEVICES   4u

/*
** Device ID info used for workaround for failed devices
*/
typedef struct
{
  uint32_t   vid;                /* Vendor ID */
  uint32_t   pid;                /* Product ID */
  uint16_t   report_size;        /* Report size */
} t_device_id_info;


/* This is a workaround for the Faraday USB IP bug.
   Interrupt  */
const t_device_id_info failed_devices[EHCI_NO_OF_FAILED_DEVICES] =
{
    {0x046D, 0xC018, 4}
  , {0x18F8, 0x0F99, 6}
  , {0xe0ff, 0x0005, 6}
  , {0x0B05, 0x181B, 9}
};
#endif


static void usbh_enum_fail_cb(t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid)
{
    ITP_USB_ERR("%s() vid: 0x%04X  pid: 0x%04X \n", __func__,vid, pid);
}

#if defined(CFG_USBH_CD_HUB) && defined(CFG_USBH_CD_HID)
/*
 * EP info change notification.
 */
void ep_info_change_cb ( t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid
                       , t_usbh_ep_hub_inf * ep_hub_inf, t_usbh_ep_inf  * ep_inf )
{
  uint32_t i;
  
  if ( ( ep_hub_inf->hub_speed == USBH_HIGH_SPEED ) && ( ep_hub_inf->dev_speed != USBH_HIGH_SPEED ) )
  {
    for( i = 0u ; i < EHCI_NO_OF_FAILED_DEVICES ; i++ )
    {
      /* If this device fails for short packets */
      if ( ( failed_devices[i].vid == vid )
          && ( failed_devices[i].pid == pid ) )
      {
        /* Change packet size for report size sent by device. */
        ep_inf->psize = failed_devices[i].report_size;
        
        break;
      }
    }
  }

} /* ep_info_change_cb */
#endif

int itp_usbh_init(void)
{
    int rc;

    /* initialize USB host stack. */
    rc = usbh_init();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_init() fail! \n");
        goto end;
    }
    /* initialize USB host controller. */
    rc = usbh_hc_init(0, usbh_ehci_hc, 0);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_init(0,0) fail! \n");
        goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_init(1, usbh_ehci_hc, 1);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_init(1,1) fail! \n");
        goto end;
    }
#endif
    rc = usbh_register_enum_failed_cb(usbh_enum_fail_cb);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_register_enum_failed_cb() fail! \n");
        goto end;
    }
#if defined(CFG_USBH_CD_HUB) && defined(CFG_USBH_CD_HID)
    rc = usbh_register_ep_info_change_cb( ep_info_change_cb );
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_register_ep_info_change_cb() fail! \n");
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

int itp_usbh_stop(void)
{
    int rc;

    rc = usbh_stop();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_stop() fail! \n");
        goto end;
    }

    rc = usbh_hc_stop(0);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_stop(0) fail! \n");
        goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_stop(1);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_stop(1) fail! \n");
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

int itp_usbh_exit(void)
{
    int rc;

    rc = usbh_delete();
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_delete() fail! \n");
        goto end;
    }

    rc = usbh_hc_delete(0);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_delete(0) fail! \n");
        goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_delete(1);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hc_delete(1) fail! \n");
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
