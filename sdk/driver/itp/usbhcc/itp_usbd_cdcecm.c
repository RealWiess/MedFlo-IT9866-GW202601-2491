#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_cdcecm.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

extern int  usbd_cdcecm_tx_cb (t_usbd_cdcecm_ntf ntf);
extern int  usbd_cdcecm_rx_cb (t_usbd_cdcecm_ntf ntf);

static int usbd_cdcecm_ntf_fn (t_usbd_cdcecm_ntf ntf)
{
    if (ntf == USBD_CDCECM_NTF_CONNECT)
    {
        (void)ioctl(ITP_DEVICE_USBDECM, ITP_IOCTL_NETIF_ADD, NULL);
    }
    else if (ntf == USBD_CDCECM_NTF_DISCONNECT)
    {
        (void)ioctl(ITP_DEVICE_USBDECM, ITP_IOCTL_NETIF_REMOVE, NULL);
    }
    else
    {
        /* no action */
    }

    return 0;
}

int itp_usbd_cd_cdcecm_init (void)
{
    t_usbd_cdcecm_ret rc;

    rc = usbd_cdcecm_init();
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_init() fail! \n");
        goto end;
    }
    rc = usbd_cdcecm_start();
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_start() fail! \n");
        goto end;
    }
    rc = usbd_cdcecm_register_ntf(USBD_CDCECM_NTF_CONNECT, &usbd_cdcecm_ntf_fn);
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_register_ntf() fail! USBD_CDCECM_NTF_CONNECT \n");
        goto end;
    }
    rc = usbd_cdcecm_register_ntf(USBD_CDCECM_NTF_DISCONNECT, &usbd_cdcecm_ntf_fn);
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_register_ntf() fail! USBD_CDCECM_NTF_DISCONNECT \n");
        goto end;
    }
    rc = usbd_cdcecm_register_ntf(USBD_CDCECM_NTF_RX, &usbd_cdcecm_rx_cb);
    if (rc != USBD_CDCECM_SUCCESS)
    {
        ITP_USB_ERR("usbd_cdcecm_register_ntf() fail! USBD_CDCECM_NTF_RX \n");
        goto end;
    }
    rc = usbd_cdcecm_register_ntf(USBD_CDCECM_NTF_TX, &usbd_cdcecm_tx_cb);
    if (rc != USBD_CDCECM_SUCCESS)
    {
        ITP_USB_ERR("usbd_cdcecm_register_ntf() fail! USBD_CDCECM_NTF_TX \n");
        goto end;
    }

    rc = usbd_start((usbd_config_t *)&device_cfg_cdc_ecm);
    if (rc != USBD_SUCCESS) 
    {
        ITP_USB_ERR("usbd_start() fail! \n");
        goto end;
    }

end:
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

int itp_usbd_cd_cdcecm_exit (void)
{
    t_usbd_cdcecm_ret rc;

    rc = usbd_cdcecm_stop();
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_stop() fail! \n");
        goto end;
    }

    rc = usbd_cdcecm_delete();
    if (rc != USBD_CDCECM_SUCCESS) 
    {
        ITP_USB_ERR("usbd_cdcecm_delete() fail! \n");
        goto end;
    }

end:
    if (rc != USBD_CDCECM_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

