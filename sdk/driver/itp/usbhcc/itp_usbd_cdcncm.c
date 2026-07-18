#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_cdcncm.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

extern int  usbd_cdcncm_tx_cb (uint8_t ntf);
extern int  usbd_cdcncm_rx_cb (uint8_t ntf);

static int usbd_cdcncm_ntf_fn (uint8_t ntf)
{
    if (ntf == USBD_CDCNCM_NTF_CONNECT)
    {
        (void)ioctl(ITP_DEVICE_USBDNCM, ITP_IOCTL_NETIF_ADD, NULL);
    }
    else if (ntf == USBD_CDCNCM_NTF_DISCONNECT)
    {
        (void)ioctl(ITP_DEVICE_USBDNCM, ITP_IOCTL_NETIF_REMOVE, NULL);
    }
    else
    {
        /* no action */
    }

    return 0;
}

int itp_usbd_cd_cdcncm_init (void)
{
    t_usbd_cdcncm_ret rc;

    rc = usbd_cdcncm_init();
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_init() fail! \n");
        goto end;
    }
    rc = usbd_cdcncm_start();
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_start() fail! \n");
        goto end;
    }
    rc = usbd_cdcncm_register_ntf(USBD_CDCNCM_NTF_CONNECT, usbd_cdcncm_ntf_fn);
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_register_ntf() fail! USBD_CDCNCM_NTF_CONNECT \n");
        goto end;
    }
    rc = usbd_cdcncm_register_ntf(USBD_CDCNCM_NTF_DISCONNECT, usbd_cdcncm_ntf_fn);
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_register_ntf() fail! USBD_CDCNCM_NTF_DISCONNECT \n");
        goto end;
    }
    rc = usbd_cdcncm_register_ntf(USBD_CDCNCM_NTF_RX, usbd_cdcncm_rx_cb);
    if (rc != USBD_CDCNCM_SUCCESS)
    {
        ITP_USB_ERR("usbd_cdcncm_register_ntf() fail! USBD_CDCNCM_NTF_RX \n");
        goto end;
    }
    rc = usbd_cdcncm_register_ntf(USBD_CDCNCM_NTF_TX, usbd_cdcncm_tx_cb);
    if (rc != USBD_CDCNCM_SUCCESS)
    {
        ITP_USB_ERR("usbd_cdcncm_register_ntf() fail! USBD_CDCNCM_NTF_TX \n");
        goto end;
    }

	  usbd_cdcncm_set_link_status(1u); /* preset to link up */

    rc = usbd_start((usbd_config_t *)&device_cfg_cdc_ncm);
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_start() fail! \n");
        goto end;
    }

end:
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

int itp_usbd_cd_cdcncm_exit (void)
{
    t_usbd_cdcncm_ret rc;

    rc = usbd_cdcncm_stop();
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_stop() fail! \n");
        goto end;
    }

    rc = usbd_cdcncm_delete();
    if (rc != USBD_CDCNCM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcncm_delete() fail! \n");
        goto end;
    }

end:
    if (rc != USBD_CDCNCM_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

