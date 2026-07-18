#include <sys/ioctl.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_hub.h"


static int usbh_hub_cb(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
   (void)printf("usbh_hub_cb: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));

    (void)printf("hub present %d \n", usbh_hub_present(uid));

    #if 0
    (void)printf("port handle: %p \n", usbh_hub_get_port_hdl(uid));
    #endif
    return 0;
}

int itp_usbh_hub_init(void)
{
    int rc;

    rc = usbh_hub_init();
    if (rc) 
    {
        ITP_USB_ERR("usbh_hub_init() fail! \n");
        goto end;
    }
    rc = usbh_hub_register_ntf(0, (USBH_NTF_CONNECT), usbh_hub_cb);
    if (rc) 
    {
        ITP_USB_ERR("usbh_hub_register_ntf() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_hub_stop(void)
{
    int rc;

    rc = usbh_hub_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbh_hub_stop() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_hub_exit(void)
{
    int rc;

    rc = usbh_hub_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbh_hub_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}
