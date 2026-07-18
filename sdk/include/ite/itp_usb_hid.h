#ifndef ITP_USB_HID_H
#define ITP_USB_HID_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CFG_USBH_CD_HID
#include "usbhcc/api/api_usbh_hid_generic.h"
void itp_usbh_hid_connect_cb(
    uint8_t uid);

void itp_usbh_hid_disconnect_cb(
    uint8_t uid);

void itp_usbh_hid_receive_report_cb(
    uint8_t uid, 
    uint8_t report_id, 
    uint8_t *report_data, 
    uint32_t report_size);

void itp_usbh_hid_ctl_ep_write_report(uint8_t uid, uint8_t report_id, uint8_t *report_data, uint8_t report_size);

void itp_usbh_hid_out_ep_write_report(uint8_t uid, uint8_t *report_data, uint8_t report_size);
#endif

#ifdef CFG_USBD_CD_HID
#include "usbhcc/api/api_usbd_hid_generic.h"
#include "usbhcc/config/hid/ite_usbd_hid_config.h"
bool itp_usbd_hid_is_connected();

void itp_usbd_hid_reg_read_ntf(uint8_t report_id, t_usbd_hid_ntf_fn ntf_fn);

void itp_usbd_hid_reg_ep0_get_report_rcv_cb(t_usbd_hid_ep0_get_report_rcv_cb cb);

t_usbd_hid_ep0_get_report_rcv_cb itp_usbd_hid_get_ep0_cb();

int itp_usbd_hid_read_data(uint8_t report_id, uint8_t *pbuf);

int itp_usbd_hid_write_data(uint8_t report_id, uint8_t *pbuf, int len);
#endif

#ifdef __cplusplus
}
#endif

#endif // ITP_USB_HID_H