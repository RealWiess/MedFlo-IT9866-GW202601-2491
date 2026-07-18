#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "itp_usb_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_hid.h"
#include "usbhcc/api/api_usbh_hid_generic.h"

#define MAX_HID_UNITS MAX_GENERIC_UNITS

typedef struct hid_out_report {
    uint8_t uid;
	uint8_t	report_id;
	uint8_t	*report_data;
    uint8_t report_size;
}HID_OUT_REPORT;

__attribute__((weak))
void itp_usbh_hid_connect_cb(
    uint8_t uid)
{}

__attribute__((weak))
void itp_usbh_hid_disconnect_cb(
    uint8_t uid)
{}

__attribute__((weak))
void itp_usbh_hid_receive_report_cb(
    uint8_t uid,
    uint8_t report_id,
    uint8_t *report_data,
    uint32_t report_size)
{}

static void *_hidh_ctl_ep_write_thread_func(void *arg)
{
    pthread_detach(pthread_self());
    int rc;
    HID_OUT_REPORT *out = (HID_OUT_REPORT *)arg;
    rc = usbh_hid_generic_write_report(out->uid, out->report_id, out->report_data, out->report_size);
    free(out->report_data);
    free(out);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hid_ctl_ep_write_report() fail! \n");
    }
    pthread_exit(NULL);
}

static void *_hidh_out_ep_write_thread_func(void *arg)
{
    pthread_detach(pthread_self());
    int rc;
    HID_OUT_REPORT *out = (HID_OUT_REPORT *)arg;
    rc = usbh_hid_generic_it_ep_write_report(out->uid, out->report_data, out->report_size);
    free(out->report_data);
    free(out);
    if (rc != USBH_SUCCESS)
    {
        ITP_USB_ERR("usbh_hid_out_ep_write_report() fail! \n");
    }
    pthread_exit(NULL);
}

void itp_usbh_hid_ctl_ep_write_report(uint8_t uid, uint8_t report_id, uint8_t *report_data, uint8_t report_size)
{
    pthread_t pid;
    int rc;
    HID_OUT_REPORT *out = (HID_OUT_REPORT *)malloc(sizeof(HID_OUT_REPORT));
    out->uid = uid;
    out->report_id = report_id;
    out->report_data = (uint8_t *)malloc(report_size);
    memcpy(out->report_data, report_data, report_size);
    out->report_size = report_size;

    rc = pthread_create(&pid, NULL, _hidh_ctl_ep_write_thread_func, (void *)out);
    if (rc != 0)
    {
        ITP_USB_ERR("itp_usbh_hid_ctl_ep_write_report() fail! \n");
    }
}

void itp_usbh_hid_out_ep_write_report(uint8_t uid, uint8_t *report_data, uint8_t report_size)
{
    pthread_t pid;
    int rc;
    HID_OUT_REPORT *out = (HID_OUT_REPORT *)malloc(sizeof(HID_OUT_REPORT));
    out->uid = uid;
    out->report_data = (uint8_t *)malloc(report_size);
    memcpy(out->report_data, report_data, report_size);
    out->report_size = report_size;

    rc = pthread_create(&pid, NULL, _hidh_out_ep_write_thread_func, (void *)out);
    if (rc != 0)
    {
        ITP_USB_ERR("itp_usbh_hid_out_ep_write_report() fail! \n");
    }
}

static int usbh_hid_cb(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
#if 0
    ITP_USB_INFO("usbh_hid_cb: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));
#endif
    if(ntf == USBH_NTF_CONNECT)
        itp_usbh_hid_connect_cb(uid);
    else
        itp_usbh_hid_disconnect_cb(uid);

    return 0;
}

int itp_usbh_hid_init(void)
{
    int rc;

    for(int i = 0; i < MAX_HID_UNITS; i++) {
        rc = usbh_hid_generic_register_ntf( i, USBH_NTF_CONNECT, usbh_hid_cb );
        if (rc != USBH_SUCCESS)
        {
            ITP_USB_ERR("usbh_hid_register_ntf() fail! \n");
            goto end;
        }
    }
    usbh_hid_generic_register_receive_ntf(itp_usbh_hid_receive_report_cb);

end:
    return rc;
}
