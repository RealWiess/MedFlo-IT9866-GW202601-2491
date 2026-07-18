#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_hid.h"
#include "usbhcc/api/api_usbd_hid_generic.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"
#include "usbhcc/config/hid/ite_usbd_hid_config.h"

static bool gConnected = false;
static uint8_t *gHid_wbuf = NULL;
static t_usbd_hid_ep0_get_report_rcv_cb gEp0cb = NULL;

bool itp_usbd_hid_is_connected()
{
    return gConnected;
}

void itp_usbd_hid_reg_read_ntf(uint8_t report_id, t_usbd_hid_ntf_fn ntf_fn)
{
    int rc;
    rc = usbd_ghid_register_ntf(0, report_id, USBD_GHID_OUT_CB, ntf_fn, 0);
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("itp_usbd_hid_reg_read_ntf() fail! \n");
    }
}

void itp_usbd_hid_reg_ep0_get_report_rcv_cb(t_usbd_hid_ep0_get_report_rcv_cb cb)
{
    gEp0cb = cb;
}

t_usbd_hid_ep0_get_report_rcv_cb itp_usbd_hid_get_ep0_cb()
{
    return gEp0cb;
}

int itp_usbd_hid_read_data(uint8_t report_id, uint8_t *pbuf)
{
    int rc;
    uint16_t read_len = 0;
    if(gConnected == false)
    {
        return 0;
    }

    rc = usbd_ghid_read_data(0, report_id, (uint8_t *)pbuf, (uint16_t *)&read_len);
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_ghid_read_data() fail! rc %d \n",rc);
    }
#if 0
    ITP_USB_DBG("UsbdHidRead: %d/%d \n", read_len, len);
#endif
    return read_len;
}

int itp_usbd_hid_write_data(uint8_t report_id, uint8_t *pbuf, int len)
{
    int rc;
    if(gConnected == false || gHid_wbuf == NULL)
    {
        return 0;
    }

    if (len > USBD_HID_MAX_REPORT_LENGTH)
    {
        ITP_USB_ERR("hid write len(%d) > %d \n", len, USBD_HID_MAX_REPORT_LENGTH);
        return 0;
    }

    memcpy((void*)gHid_wbuf, (void*)pbuf, len);
    rc = usbd_ghid_write_data(0, report_id, gHid_wbuf);

    if (rc != USBD_SUCCESS && rc != USBD_HID_ERR_NOT_READY)
    {
        ITP_USB_ERR("usbd_ghid_write_data() fail! rc %d \n", rc);
        return 0;
    }
#if 0
    ITP_USB_DBG("UsbdHidWrite: %d \n", len);
#endif

    return len;
}

int itp_usbd_hid_init(void)
{
    int rc;

    rc = usbd_hid_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_hid_init() fail! \n");
        goto end;
    }
    rc = usbd_ghid_init();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_ghid_init() fail! \n");
        goto end;
    }
    rc = usbd_ghid_start();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_ghid_start() fail! \n");
        goto end;
    }
    rc = usbd_hid_start();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_hid_start() fail! \n");
        goto end;
    }

    gHid_wbuf = malloc(USBD_HID_MAX_REPORT_LENGTH);
    if (gHid_wbuf == NULL)
    {
        ITP_USB_ERR("alloc HID write buffer fail! \n");
        rc = -1;
        goto end;
    }
    gConnected = true;

end:
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}

int itp_usbd_hid_exit(void)
{
    int rc;

    gConnected = false;
    if (gHid_wbuf != NULL)
    {
        free(gHid_wbuf);
        gHid_wbuf = NULL;
    }
    gEp0cb = NULL;

    rc = usbd_ghid_stop();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_ghid_stop() fail! \n");
        goto end;
    }
    rc = usbd_ghid_delete();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_ghid_delete() fail! \n");
        goto end;
    }
    rc = usbd_hid_stop();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_hid_stop() fail! \n");
        goto end;
    }
    rc = usbd_hid_delete();
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("usbd_hid_delete() fail! \n");
        goto end;
    }

end:
    if (rc != USBD_SUCCESS)
    {
        ITP_USB_ERR("%s() rc = %d \n", __func__, rc);
    }
    return rc;
}
