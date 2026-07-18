#include <sys/ioctl.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbh_ccid.h"
#include "usbhcc/api/api_pcsc_lite.h"


int itp_usbh_ccid_init(void)
{
    int rc;

    rc = usbh_ccid_init();
    if (rc) 
    {
        ITP_USB_ERR("usbh_ccid_init() fail! \n");
        goto end;
    }

    rc = pcsc_init();
    if (rc) 
    {
        ITP_USB_ERR("pcsc_init() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_ccid_stop(void)
{
    int rc;

    rc = usbh_ccid_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbh_ccid_stop() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_ccid_exit(void)
{
    int rc;

    rc = usbh_ccid_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbh_ccid_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}
