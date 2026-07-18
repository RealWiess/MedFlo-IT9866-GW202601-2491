#include <stdio.h>
#include "usbhcc/api/api_usbd.h"


/* call from usbh_hc_init() */
int usb_get_device_cfg(int *usb_idx, unsigned int *id_pin)
{
    int rc = -1;

#if defined(CFG_USBHCC_DEVICE)
    #if defined(CFG_USBD_USB0)
    (*usb_idx) = 0;
    #endif
    #if defined(CFG_USBD_USB1)
    (*usb_idx) = 1;
    #endif
    (*id_pin) = CFG_GPIO_USB_ID_PIN;
    rc = 0;
#endif

    return rc;
}

/*
** Tell USB device driver if the board is currently running USB powered
** or not.
**
*/
int usbd_is_self_powered(void)
{
    /* allways self powered */
    return 1;
}

void usbd_conn_state(usbd_conn_state_t new_state)
{
    switch (new_state)
    {
    case usbdcst_invalid:
        (void)printf("usbd state: usbdcst_invalid \n");
        /*  */
        break;

    case usbdcst_offline:
        (void)printf("usbd state: cable not connected, stack stopped \n");
        /* cable not connected, stack stopped */
        break;

    case usbdcst_ready:
        (void)printf("usbd state: cabe not connected, stack started \n");
        /* cabe not connected, stack started */
        break;

    case usbdcst_stopped:
        (void)printf("usbd state: cable connected, stack stopped; bus powered device may draw 500uA from USB \n");
        /* cable connected, stack stopped; bus powered device may draw 500uA from USB */
        break;

    case usbdcst_not_cfg:
        (void)printf("usbd state: online, not configured; bus powered device may draw 10mA from USB \n");
        /* online, not configured; bus powered device may draw 10mA from USB */
        break;

    case usbdcst_cfg:
        (void)printf("usbd state: configured \n");
        /* configured; bus powered device may the maximum amount of current
        specifyed in the configuration descriptor */
        break;

    case usbdcst_suspend:
        (void)printf("usbd state: suspended; bus powered device may draw 500uA from USB \n");
        /* suspended; bus powered device may draw 500uA from USB */
        break;

    case usbdcst_suspend_hic:
        (void)printf("usbd state: suspended; bus powered device may draw 2.5 mA from USB \n");
        /* suspended; bus powered device may draw 2.5 mA from USB */
        break;
    } /* switch */

} /* usbd_conn_state */

/* avoid print some log when unplug */
int usbd_is_dbg(void)
{
#if defined(CFG_DBG_USBHCC)
    return 1;
#else
    return 0;
#endif
}


