// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __BD_GAP_BT_API_H__
#define __BD_GAP_BT_API_H__

#include <stdint.h>
#include "bd_err.h"
#include "bd_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// RSSI threshold
#define BD_BT_GAP_RSSI_HIGH_THRLD  -20             /*!< High RSSI threshold */
#define BD_BT_GAP_RSSI_LOW_THRLD   -45             /*!< Low RSSI threshold */

/// Class of device
typedef struct {
    uint32_t      reserved_2: 2;                    /*!< undefined */
    uint32_t      minor: 6;                         /*!< minor class */
    uint32_t      major: 5;                         /*!< major class */
    uint32_t      service: 11;                      /*!< service class */
    uint32_t      reserved_8: 8;                    /*!< undefined */
} bd_bt_cod_t;

/// class of device settings
typedef enum {
    BD_BT_SET_COD_MAJOR_MINOR     = 0x01,          /*!< overwrite major, minor class */
    BD_BT_SET_COD_SERVICE_CLASS   = 0x02,          /*!< set the bits in the input, the current bit will remain */
    BD_BT_CLR_COD_SERVICE_CLASS   = 0x04,          /*!< clear the bits in the input, others will remain */
    BD_BT_SET_COD_ALL             = 0x08,          /*!< overwrite major, minor, set the bits in service class */
    BD_BT_INIT_COD                = 0x0a,          /*!< overwrite major, minor, and service class */
} bd_bt_cod_mode_t;

/// Discoverability and Connectability mode
typedef enum {
    BD_BT_SCAN_MODE_NONE = 0,                      /*!< Neither discoverable nor connectable */
    BD_BT_SCAN_MODE_CONNECTABLE,                   /*!< Connectable but not discoverable */
    BD_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE       /*!< both discoverable and connectable */
} bd_bt_scan_mode_t;

/// Bluetooth Device Property type
typedef enum {
    BD_BT_GAP_DEV_PROP_BDNAME = 1,                 /*!< Bluetooth device name, value type is int8_t [] */
    BD_BT_GAP_DEV_PROP_COD,                        /*!< Class of Device, value type is uint32_t */
    BD_BT_GAP_DEV_PROP_RSSI,                       /*!< Received Signal strength Indication, value type is int8_t, ranging from -128 to 127 */
    BD_BT_GAP_DEV_PROP_EIR,                        /*!< Extended Inquiry Response, value type is uint8_t [] */
} bd_bt_gap_dev_prop_type_t;

/// Maximum bytes of Bluetooth device name
#define BD_BT_GAP_MAX_BDNAME_LEN             (248)

/// Maximum size of EIR Significant part
#define BD_BT_GAP_EIR_DATA_LEN               (240)

/// Bluetooth Device Property Descriptor
typedef struct {
    bd_bt_gap_dev_prop_type_t type;                /*!< device property type */
    int len;                                        /*!< device property value length */
    void *val;                                      /*!< devlice prpoerty value */
} bd_bt_gap_dev_prop_t;

/// Extended Inquiry Response data type
typedef enum {
    BD_BT_EIR_TYPE_FLAGS                    = 0x01,     /*!< Flag with information such as BR/EDR and LE support */
    BD_BT_EIR_TYPE_INCMPL_16BITS_UUID       = 0x02,     /*!< Incomplete list of 16-bit service UUIDs */
    BD_BT_EIR_TYPE_CMPL_16BITS_UUID         = 0x03,     /*!< Complete list of 16-bit service UUIDs */
    BD_BT_EIR_TYPE_INCMPL_32BITS_UUID       = 0x04,     /*!< Incomplete list of 32-bit service UUIDs */
    BD_BT_EIR_TYPE_CMPL_32BITS_UUID         = 0x05,     /*!< Complete list of 32-bit service UUIDs */
    BD_BT_EIR_TYPE_INCMPL_128BITS_UUID      = 0x06,     /*!< Incomplete list of 128-bit service UUIDs */
    BD_BT_EIR_TYPE_CMPL_128BITS_UUID        = 0x07,     /*!< Complete list of 128-bit service UUIDs */
    BD_BT_EIR_TYPE_SHORT_LOCAL_NAME         = 0x08,     /*!< Shortened Local Name */
    BD_BT_EIR_TYPE_CMPL_LOCAL_NAME          = 0x09,     /*!< Complete Local Name */
    BD_BT_EIR_TYPE_TX_POWER_LEVEL           = 0x0a,     /*!< Tx power level, value is 1 octet ranging from  -127 to 127, unit is dBm*/
    BD_BT_EIR_TYPE_MANU_SPECIFIC            = 0xff,     /*!< Manufacturer specific data */
} bd_bt_eir_type_t;

/// Major service class field of Class of Device, mutiple bits can be set
typedef enum {
    BD_BT_COD_SRVC_NONE                     =     0,    /*!< None indicates an invalid value */
    BD_BT_COD_SRVC_LMTD_DISCOVER            =   0x1,    /*!< Limited Discoverable Mode */
    BD_BT_COD_SRVC_POSITIONING              =   0x8,    /*!< Positioning (Location identification) */
    BD_BT_COD_SRVC_NETWORKING               =  0x10,    /*!< Networking, e.g. LAN, Ad hoc */
    BD_BT_COD_SRVC_RENDERING                =  0x20,    /*!< Rendering, e.g. Printing, Speakers */
    BD_BT_COD_SRVC_CAPTURING                =  0x40,    /*!< Capturing, e.g. Scanner, Microphone */
    BD_BT_COD_SRVC_OBJ_TRANSFER             =  0x80,    /*!< Object Transfer, e.g. v-Inbox, v-Folder */
    BD_BT_COD_SRVC_AUDIO                    = 0x100,    /*!< Audio, e.g. Speaker, Microphone, Headerset service */
    BD_BT_COD_SRVC_TELEPHONY                = 0x200,    /*!< Telephony, e.g. Cordless telephony, Modem, Headset service */
    BD_BT_COD_SRVC_INFORMATION              = 0x400,    /*!< Information, e.g., WEB-server, WAP-server */
} bd_bt_cod_srvc_t;

typedef enum{
    BD_BT_PIN_TYPE_VARIABLE = 0,                       /*!< Refer to BTM_PIN_TYPE_VARIABLE */
    BD_BT_PIN_TYPE_FIXED    = 1,                       /*!< Refer to BTM_PIN_TYPE_FIXED */
} bd_bt_pin_type_t;

#define BD_BT_PIN_CODE_LEN        16                   /*!< Max pin code length */
typedef uint8_t bd_bt_pin_code_t[BD_BT_PIN_CODE_LEN]; /*!< Pin Code (upto 128 bits) MSB is 0 */

/// Bits of major service class field
#define BD_BT_COD_SRVC_BIT_MASK              (0xffe000) /*!< Major service bit mask */
#define BD_BT_COD_SRVC_BIT_OFFSET            (13)       /*!< Major service bit offset */

/// Major device class field of Class of Device
typedef enum {
    BD_BT_COD_MAJOR_DEV_MISC                = 0,    /*!< Miscellaneous */
    BD_BT_COD_MAJOR_DEV_COMPUTER            = 1,    /*!< Computer */
    BD_BT_COD_MAJOR_DEV_PHONE               = 2,    /*!< Phone(cellular, cordless, pay phone, modem */
    BD_BT_COD_MAJOR_DEV_LAN_NAP             = 3,    /*!< LAN, Network Access Point */
    BD_BT_COD_MAJOR_DEV_AV                  = 4,    /*!< Audio/Video(headset, speaker, stereo, video display, VCR */
    BD_BT_COD_MAJOR_DEV_PERIPHERAL          = 5,    /*!< Peripheral(mouse, joystick, keyboard) */
    BD_BT_COD_MAJOR_DEV_IMAGING             = 6,    /*!< Imaging(printer, scanner, camera, display */
    BD_BT_COD_MAJOR_DEV_WEARABLE            = 7,    /*!< Wearable */
    BD_BT_COD_MAJOR_DEV_TOY                 = 8,    /*!< Toy */
    BD_BT_COD_MAJOR_DEV_HEALTH              = 9,    /*!< Health */
    BD_BT_COD_MAJOR_DEV_UNCATEGORIZED       = 31,   /*!< Uncategorized: device not specified */
} bd_bt_cod_major_dev_t;

/// Bits of major device class field
#define BD_BT_COD_MAJOR_DEV_BIT_MASK         (0x1f00) /*!< Major device bit mask */
#define BD_BT_COD_MAJOR_DEV_BIT_OFFSET       (8)      /*!< Major device bit offset */

/// Bits of minor device class field
#define BD_BT_COD_MINOR_DEV_BIT_MASK         (0xfc)   /*!< Minor device bit mask */
#define BD_BT_COD_MINOR_DEV_BIT_OFFSET       (2)      /*!< Minor device bit offset */

/// Bits of format type
#define BD_BT_COD_FORMAT_TYPE_BIT_MASK       (0x03)   /*!< Format type bit mask */
#define BD_BT_COD_FORMAT_TYPE_BIT_OFFSET     (0)      /*!< Format type bit offset */

/// Class of device format type 1
#define BD_BT_COD_FORMAT_TYPE_1              (0x00)

/** Bluetooth Device Discovery state */
typedef enum {
    BD_BT_GAP_DISCOVERY_STOPPED,                   /*!< device discovery stopped */
    BD_BT_GAP_DISCOVERY_STARTED,                   /*!< device discovery started */
} bd_bt_gap_discovery_state_t;

/// BT GAP callback events
typedef enum {
    BD_BT_GAP_DISC_RES_EVT = 0,                    /*!< device discovery result event */
    BD_BT_GAP_DISC_STATE_CHANGED_EVT,              /*!< discovery state changed event */
    BD_BT_GAP_RMT_SRVCS_EVT,                       /*!< get remote services event */
    BD_BT_GAP_RMT_SRVC_REC_EVT,                    /*!< get remote service record event */
    BD_BT_GAP_AUTH_CMPL_EVT,                       /*!< AUTH complete event */
    BD_BT_GAP_PIN_REQ_EVT,                         /*!< Legacy Pairing Pin code request */
    BD_BT_GAP_READ_RSSI_DELTA_EVT,                 /*!< read rssi event */
    BD_BT_GAP_EVT_MAX,
} bd_bt_gap_cb_event_t;

/** Inquiry Mode */
typedef enum {
    BD_BT_INQ_MODE_GENERAL_INQUIRY,                /*!< General inquiry mode */
    BD_BT_INQ_MODE_LIMITED_INQUIRY,                /*!< Limited inquiry mode */
} bd_bt_inq_mode_t;

/** Minimum and Maximum inquiry length*/
#define BD_BT_GAP_MIN_INQ_LEN                (0x01U)  /*!< Minimum inquiry duration, unit is 1.28s */
#define BD_BT_GAP_MAX_INQ_LEN                (0x30U)  /*!< Maximum inquiry duration, unit is 1.28s */

/// A2DP state callback parameters
typedef union {
    /**
     * @brief BD_BT_GAP_DISC_RES_EVT
     */
    struct disc_res_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        int num_prop;                          /*!< number of properties got */
        bd_bt_gap_dev_prop_t *prop;           /*!< properties discovered from the new device */
    } disc_res;                                /*!< discovery result paramter struct */

    /**
     * @brief  BD_BT_GAP_DISC_STATE_CHANGED_EVT
     */
    struct disc_state_changed_param {
        bd_bt_gap_discovery_state_t state;    /*!< discovery state */
    } disc_st_chg;                             /*!< discovery state changed parameter struct */

    /**
     * @brief BD_BT_GAP_RMT_SRVCS_EVT
     */
    struct rmt_srvcs_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bd_bt_status_t stat;                  /*!< service search status */
        int num_uuids;                         /*!< number of UUID in uuid_list */
        bd_bt_uuid_t *uuid_list;              /*!< list of service UUIDs of remote device */
    } rmt_srvcs;                               /*!< services of remote device parameter struct */

    /**
     * @brief BD_BT_GAP_RMT_SRVC_REC_EVT
     */
    struct rmt_srvc_rec_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bd_bt_status_t stat;                  /*!< service search status */
    } rmt_srvc_rec;                            /*!< specific service record from remote device parameter struct */

    /**
     * @brief BD_BT_GAP_READ_RSSI_DELTA_EVT *
     */
    struct read_rssi_delta_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bd_bt_status_t stat;                  /*!< read rssi status */
        int8_t rssi_delta;                     /*!< rssi delta value range -128 ~127, The value zero indicates that the RSSI is inside the Golden Receive Power Range, the Golden Receive Power Range is from BD_BT_GAP_RSSI_LOW_THRLD to BD_BT_GAP_RSSI_HIGH_THRLD */
    } read_rssi_delta;                         /*!< read rssi parameter struct */

    /**
     * @brief BD_BT_GAP_AUTH_CMPL_EVT
     */
    struct auth_cmpl_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bd_bt_status_t stat;                  /*!< authentication complete status */
        uint8_t device_name[BD_BT_GAP_MAX_BDNAME_LEN + 1]; /*!< device name */
    } auth_cmpl;                               /*!< authentication complete parameter struct */

    /**
     * @brief BD_BT_GAP_PIN_REQ_EVT
     */
    struct pin_req_param {
        bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bool min_16_digit;                     /*!< TRUE if the pin returned must be at least 16 digits */
    } pin_req;                                 /*!< pin request parameter struct */

} bd_bt_gap_cb_param_t;

/**
 * @brief           bluetooth GAP callback function type
 * @param           event : Event type
 * @param           param : Pointer to callback parameter
 */
typedef void (* bd_bt_gap_cb_t)(bd_bt_gap_cb_event_t event, bd_bt_gap_cb_param_t *param);

/**
 * @brief           get major service field of COD
 * @param[in]       cod: Class of Device
 * @return          major service bits
 */
inline uint32_t bd_bt_gap_get_cod_srvc(uint32_t cod)
{
    return (cod & BD_BT_COD_SRVC_BIT_MASK) >> BD_BT_COD_SRVC_BIT_OFFSET;
}

/**
 * @brief           get major device field of COD
 * @param[in]       cod: Class of Device
 * @return          major device bits
 */
inline uint32_t bd_bt_gap_get_cod_major_dev(uint32_t cod)
{
    return (cod & BD_BT_COD_MAJOR_DEV_BIT_MASK) >> BD_BT_COD_MAJOR_DEV_BIT_OFFSET;
}

/**
 * @brief           get minor service field of COD
 * @param[in]       cod: Class of Device
 * @return          minor service bits
 */
inline uint32_t bd_bt_gap_get_cod_minor_dev(uint32_t cod)
{
    return (cod & BD_BT_COD_MINOR_DEV_BIT_MASK) >> BD_BT_COD_MINOR_DEV_BIT_OFFSET;
}

/**
 * @brief           get format type of COD
 * @param[in]       cod: Class of Device
 * @return          format type
 */
inline uint32_t bd_bt_gap_get_cod_format_type(uint32_t cod)
{
    return (cod & BD_BT_COD_FORMAT_TYPE_BIT_MASK);
}

/**
 * @brief           decide the integrity of COD
 * @param[in]       cod: Class of Device
 * @return
 *                  - true if cod is valid
 *                  - false otherise
 */
inline bool bd_bt_gap_is_valid_cod(uint32_t cod)
{
    if (bd_bt_gap_get_cod_format_type(cod) == BD_BT_COD_FORMAT_TYPE_1 &&
            bd_bt_gap_get_cod_srvc(cod) != BD_BT_COD_SRVC_NONE) {
        return true;
    }

    return false;
}

/**
 * @brief           register callback function. This function should be called after bd_enable() completes successfully
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_register_callback(bd_bt_gap_cb_t callback);

/**
 * @brief           Set discoverability and connectability mode for legacy bluetooth. This function should
 *                  be called after bd_enable() completes successfully
 *
 * @param[in]       mode : one of the enums of bt_scan_mode_t
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_ARG: if argument invalid
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_set_scan_mode(bd_bt_scan_mode_t mode);

/**
 * @brief           Start device discovery. This function should be called after bd_enable() completes successfully.
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_DISC_STATE_CHANGED_EVT if discovery is started or halted.
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_DISC_RES_EVT if discovery result is got.
 *
 * @param[in]       mode - inquiry mode
 * @param[in]       inq_len - inquiry duration in 1.28 sec units, ranging from 0x01 to 0x30
 * @param[in]       num_rsps - number of inquiry responses that can be received, value 0 indicates an unlimited number of responses
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_ERR_INVALID_ARG: if invalid parameters are provided
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_start_discovery(bd_bt_inq_mode_t mode, uint8_t inq_len, uint8_t num_rsps);

/**
 * @brief           Cancel device discovery. This function should be called after bd_enable() completes successfully
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_DISC_STATE_CHANGED_EVT if discovery is stopped.
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_cancel_discovery(void);

/**
 * @brief           Start SDP to get remote services. This function should be called after bd_enable() completes successfully.
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_RMT_SRVCS_EVT after service discovery ends
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_get_remote_services(bd_addr_t remote_bda);

/**
 * @brief           Start SDP to look up the service matching uuid on the remote device. This function should be called after
 *                  bd_enable() completes successfully
 *
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_RMT_SRVC_REC_EVT after service discovery ends
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_get_remote_service_record(bd_addr_t remote_bda, bd_bt_uuid_t *uuid_t);

/**
 * @brief           This function is called to get EIR data for a specific type.
 *
 * @param[in]       eir - pointer of raw eir data to be resolved
 * @param[in]       type   - specific EIR data type
 * @param[out]      length - return the length of EIR data excluding fields of length and data type
 *
 * @return          pointer of starting position of eir data excluding eir data type, NULL if not found
 *
 */
uint8_t *bd_bt_gap_resolve_eir_data(uint8_t *eir, bd_bt_eir_type_t type, uint8_t *length);

/**
 * @brief           This function is called to set class of device.
 *                  bd_bt_gap_cb_t will is called with BD_BT_GAP_SET_COD_EVT after set COD ends
 *                  Some profile have special restrictions on class of device,
 *                  changes may cause these profile do not work
 *
 * @param[in]       cod - class of device
 * @param[in]       mode   - setting mode
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BD_ERR_INVALID_ARG: if param is invalid
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_set_cod(bd_bt_cod_t cod, bd_bt_cod_mode_t mode);

/**
 * @brief           This function is called to get class of device.
 *
 * @param[out]      cod - class of device
 *
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_FAIL: others
 */
bd_err_t bd_bt_gap_get_cod(bd_bt_cod_t *cod);

/**
 * @brief           This function is called to read RSSI delta by address after connected. The RSSI value returned by BD_BT_GAP_READ_RSSI_DELTA_EVT.
 *
 *
 * @param[in]       remote_addr - remote device address, corresponding to a certain connection handle.
 * @return
 *                  - BD_OK : Succeed
 *                  - BD_FAIL: others
 *
 */
bd_err_t bd_bt_gap_read_rssi_delta(bd_addr_t remote_addr);

/**
* @brief           Removes a device from the security database list of
*                  peer device.
*
* @param[in]       bd_addr : BD address of the peer device
*
* @return          - BD_OK : success
*                  - BD_FAIL  : failed
*
*/
bd_err_t bd_bt_gap_remove_bond_device(bd_addr_t bd_addr);

/**
* @brief           Get the device number from the security database list of peer device.
*                  It will return the device bonded number immediately.
*
* @return          - >= 0 : bonded devices number.
*                  - BD_FAIL  : failed
*
*/
int bd_bt_gap_get_bond_device_num(void);

/**
* @brief           Get the device from the security database list of peer device.
*                  It will return the device bonded information immediately.
* @param[inout]    dev_num: Indicate the dev_list array(buffer) size as input.
*                           If dev_num is large enough, it means the actual number as output.
*                           Suggest that dev_num value equal to bd_ble_get_bond_device_num().
*
* @param[out]      dev_list: an array(buffer) of `bd_addr_t` type. Use for storing the bonded devices address.
*                            The dev_list should be allocated by who call this API.
*
* @return
*                  - BD_OK : Succeed
*                  - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - BD_FAIL: others
*/
bd_err_t bd_bt_gap_get_bond_device_list(int *dev_num, bd_addr_t *dev_list);

/**
* @brief            Set pin type and default pin code for legacy pairing.
*
* @param[in]        pin_type:       Use variable or fixed pin.
*                                   If pin_type is BD_BT_PIN_TYPE_VARIABLE, pin_code and pin_code_len
*                                   will be ignored, and BD_BT_GAP_PIN_REQ_EVT will come when control
*                                   requests for pin code.
*                                   Else, will use fixed pin code and not callback to users.
* @param[in]        pin_code_len:   Length of pin_code
* @param[in]        pin_code:       Pin_code
*
* @return           - BD_OK : success
*                   - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
bd_err_t bd_bt_gap_set_pin(bd_bt_pin_type_t pin_type, uint8_t pin_code_len, bd_bt_pin_code_t pin_code);

/**
* @brief            Reply the pin_code to the peer device for legacy pairing
*                   when BD_BT_GAP_PIN_REQ_EVT is coming.
*
* @param[in]        bd_addr:        BD address of the peer
* @param[in]        accept:         Pin_code reply successful or declined.
* @param[in]        pin_code_len:   Length of pin_code
* @param[in]        pin_code:       Pin_code
*
* @return           - BD_OK : success
*                   - BD_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
bd_err_t bd_bt_gap_pin_reply(bd_addr_t bd_addr, bool b_accept, uint8_t pin_code_len, bd_bt_pin_code_t pin_code);

#ifdef __cplusplus
}
#endif

#endif /* __BD_GAP_BT_API_H__ */
