#include <sys/ioctl.h>
#include <string.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_scsi.h"
#include "usbhcc/api/api_usbh_mst.h"

/**
 * TODO: hot plugging for lun of card reader
 */

static uint8_t unit_state[SCSI_MAX_UNITS * SCSI_MAX_LUN];

extern int mst_insert;

#define ITP_SCSI_DISCONNECTED   0
#define ITP_SCSI_CONNECTED      1
#define ITP_SCSI_PRESENT        2

static int usbh_mst_cb(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    ITPCardStatus cardStatus;
    int i, state;

    (void)ithPrintf("usbh_mst_cb: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));

    if (ntf == USBH_NTF_CONNECT)
    {
        for (i = 0; i < (SCSI_MAX_UNITS * SCSI_MAX_LUN); i++) 
        {
            if (unit_state[i] == ITP_SCSI_CONNECTED) 
            {
                state = scsi_get_unit_state(i);
                (void)ithPrintf("unit[%d]: state %d \n", i, state);

                if (state & SCSI_ST_CONNECTED) 
                {
                    cardStatus.card = ITP_CARD_MSC00 << i;
                    cardStatus.inserted = 1;
                    write(ITP_DEVICE_CARD, &cardStatus, sizeof(ITPCardStatus));

                    unit_state[i] = ITP_SCSI_PRESENT;
                    mst_insert = 1;
                }
            }
        }
    }
    else if (ntf == USBH_NTF_DISCONNECT)
    {
        for (i = 0; i < (SCSI_MAX_UNITS * SCSI_MAX_LUN); i++) 
        {
            if (unit_state[i] == ITP_SCSI_PRESENT)
            {
                break;
            }
        }

        if (i == (SCSI_MAX_UNITS * SCSI_MAX_LUN))
        {
            mst_insert = 0;
        }
    }

    return 0;
}

static void usbh_scsi_cb(uint8_t unit, uint8_t state)
{
    if (state & SCSI_ST_CONNECTED) 
    {
        (void)ithPrintf("scsi unit %d %s!\n", unit, "connected");
        unit_state[unit] = ITP_SCSI_CONNECTED;
    }
    if (state & SCSI_ST_CHANGED) 
    {
        (void)ithPrintf("scsi unit %d %s!\n", unit, "changed");
    }
    if (state & SCSI_ST_WRPROTECT) 
    {
        (void)ithPrintf("scsi unit %d %s!\n", unit, "write protect");
    }
    if (state == SCSI_ST_DISCONNECTED) 
    {
        (void)ithPrintf("scsi unit %d %s!\n", unit, "disconnected");
        if (unit_state[unit] == ITP_SCSI_PRESENT) 
        {
            ITPCardStatus cardStatus;

            cardStatus.card = ITP_CARD_MSC00 << unit;
            cardStatus.inserted = 0;
            write(ITP_DEVICE_CARD, &cardStatus, sizeof(ITPCardStatus));
        }

        unit_state[unit] = ITP_SCSI_DISCONNECTED;
    }
}

int itp_usbh_mst_init(void)
{
    int rc, i;

    rc = usbh_mst_init();
    if (rc) 
    {
        ITP_USB_ERR("usbh_mst_init() fail! \n");
        goto end;
    }
    for (i = 0; i < SCSI_MAX_UNITS; i++) 
    {
        rc = usbh_mst_register_ntf(i, (USBH_NTF_CONNECT), usbh_mst_cb);
        if (rc) 
        {
            ITP_USB_ERR("usbh_mst_register_ntf(%d) fail! \n", i);
            goto end;
        }
    }
    rc = scsi_init();
    if (rc) 
    {
        ITP_USB_ERR("scsi_init() fail! \n");
        goto end;
    }
    scsi_register_cb(usbh_scsi_cb);

end:
    return rc;
}

int itp_usbh_mst_stop(void)
{
    int rc;

    rc = scsi_stop();
    if (rc) 
    {
        ITP_USB_ERR("scsi_stop() fail! \n");
        goto end;
    }

    rc = usbh_mst_stop();
    if (rc) 
    {
        ITP_USB_ERR("usbh_mst_stop() fail! \n");
        goto end;
    }

end:
    return rc;
}

int itp_usbh_mst_exit(void)
{
    int rc;

    rc = scsi_delete();
    if (rc) 
    {
        ITP_USB_ERR("scsi_delete() fail! \n");
        goto end;
    }

    rc = usbh_mst_delete();
    if (rc) 
    {
        ITP_USB_ERR("usbh_mst_delete() fail! \n");
        goto end;
    }

end:
    return rc;
}
