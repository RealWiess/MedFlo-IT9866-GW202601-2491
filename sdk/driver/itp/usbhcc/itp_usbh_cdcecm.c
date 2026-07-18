#include <sys/ioctl.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_cdcecm.h"
#include "usbhcc/config/config_usbh_cdcecm.h"

struct usbh_cdcecm_ctxt {
    uint8_t state;
    uint8_t reserved[3];
};

static struct usbh_cdcecm_ctxt ecm_ctxt[USBH_CDCECM_MAX_UNITS] = { 0 };

extern int usbh_cdcecm_tx_cb(t_usbh_unit_id uid, t_usbh_ntf ntf);
extern int usbh_cdcecm_rx_cb(t_usbh_unit_id uid, t_usbh_ntf ntf);

static int cdcecm_ntf_fn(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    (void)ithPrintf("[ECM]: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));

    /* TODO: multi-uint */
    if (ntf == USBH_NTF_CONNECT) 
    {
        #if defined(CFG_USBH_CD_CDCECM)
        ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_NETIF_ADD, NULL);
        #endif

        #if defined(CFG_USBH_CD_CDCECM_EX)
        ioctl(ITP_DEVICE_ECM_EX, ITP_IOCTL_NETIF_ADD, NULL);
        #endif
    }
    else if (ntf == USBH_NTF_DISCONNECT) 
    {
        #if defined(CFG_USBH_CD_CDCECM)
        ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_NETIF_REMOVE, NULL);
        #endif

        #if defined(CFG_USBH_CD_CDCECM_EX)
        ioctl(ITP_DEVICE_ECM_EX, ITP_IOCTL_NETIF_REMOVE, NULL);
        #endif
    }

    ecm_ctxt[uid].state = ntf;

    return 0;
}

int itp_usbh_cdcecm_init(void)
{
    int rc, i;

    rc = usbh_cdcecm_init();
    if (rc) 
    {
        ITP_USB_ERR("usbh_cdcecm_init() fail! \n");
        goto end;
    }
    for (i = 0; i < USBH_CDCECM_MAX_UNITS; i++) 
    {
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CONNECT, cdcecm_ntf_fn);
        if (rc) 
        {
            ITP_USB_ERR("usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CONNECT \n", i);
            goto end;
        }
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CDCECM_TX, usbh_cdcecm_tx_cb);
        if (rc) 
        {
            ITP_USB_ERR("usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CDCECM_TX \n", i);
            goto end;
        }
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CDCECM_RX, usbh_cdcecm_rx_cb);
        if (rc) 
        {
            ITP_USB_ERR("usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CDCECM_RX \n", i);
            goto end;
        }
    }

end:
    return rc;
}

int itp_usbh_cdcecm_stop(void)
{
    int rc;

    rc = usbh_cdcecm_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbh_cdcecm_stop() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_cdcecm_exit(void)
{
    int rc;

    rc = usbh_cdcecm_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbh_cdcecm_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}
