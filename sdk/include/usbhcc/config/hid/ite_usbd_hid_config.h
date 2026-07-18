/*
* ITE USB HID Device Configuration
*/
#ifndef __ITE_USBD_HID_CONFIG__
#define __ITE_USBD_HID_CONFIG__

// #if defined(CFG_USBD_CD_HID)
typedef void ( * t_usbd_hid_ep0_get_report_rcv_cb )(uint8_t* buf, uint16_t* len);

#define USBD_HID_VENDOR_ID       0x048D
#define USBD_HID_PRODUCT_ID      0x9860

#define USBD_HID_IFC_SUBCLASS    0
#define USBD_HID_IFC_PROTOCOL    0

#define USBD_HID_USE_REPORT_ID         0
#define USBD_HID_IN_REPORT_ID_BASE     0
#define USBD_HID_OUT_REPORT_ID_BASE    0

#define USBD_HID_MAX_REPORT_LENGTH     64
#define USBD_HID_REPORT_DESC_LENGTH    33

static const uint8_t ite_usbd_hid_manufacturer_string[] = 
{
20, 3, /* length+type  */
'I', 0,'T', 0,'E', 0,' ', 0,
'T', 0,'e', 0,'c', 0,'h', 0,
'.', 0
};

static const uint8_t ite_usbd_hid_product_string[] = 
{
24, 3, /* length+type  */
'H', 0,'I', 0,'D', 0,' ', 0,
'g', 0,'e', 0,'n', 0,'e', 0,
'r', 0,'i', 0,'c', 0

};

static const uint8_t ite_usbd_hid_version_string[] = 
{
18, 3, /* length+type  */
'V', 0,'e', 0,'r', 0,' ', 0,
'1', 0,'.', 0,'0', 0,'0', 0

};

static const uint8_t ite_usbd_hid_report_descriptor[] = {/* Array of Bytes */
    0x05, 0x01,                     // usage page (undefined) 
    0x09, 0x00,                     // Usage (undefined)
    0xA1, 0x01,                     // collection (application) 
    //-----Input report-----
    0x09, 0x00,                     // USAGE (Undefined)
    0x15, 0x00,                     // LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,               // LOGICAL_MAXIMUM (255)
    0x75, 0x08,                     // REPORT_SIZE (8)
    0x95, 0x40,                     // REPORT_COUNT (64)
    0x81, 0x02,                     // INPUT (Data,Vari,Abs)
    //-----Output report-----
    0x09, 0x00,                     // USAGE (Undefined)
    0x15, 0x00,                     // logical minimum (0)
    0x26, 0xFF, 0x00,               // logical maximum (255)
    0x75, 0x08,                     // report size (8)
    0x95, 0x40,                     // report count (64)
    0x91, 0x02,                     // OUTPUT (Data,Vari,Abs)

    // 0xC0,                           // end collection
    0xC0                            // end collection
 }; 

// #endif //CFG_USBD_CD_HID
#endif //__ITE_USBD_HID_CONFIG__