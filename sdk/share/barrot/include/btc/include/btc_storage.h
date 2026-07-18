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

#ifndef __BTC_STORAGE_H__
#define __BTC_STORAGE_H__

#include <stdint.h>
#include "include/common/bt_defs.h"
#include "include/stack/include/bt_types.h"
#include "include/api/bd_gap_bt_api.h"


#define BTC_STORAGE_DEV_CLASS_STR       "DevClass"
#define BTC_STORAGE_LINK_KEY_STR        "LinkKey"    /* same as the ble */
#define BTC_STORAGE_LINK_KEY_TYPE_STR   "LinkKeyType"
#define BTC_STORAGE_PIN_LENGTH_STR      "PinLength"
#define BTC_STORAGE_REMOTE_NAME         "RemoteName"

#define BTC_STORAGE_LATEST_SEC_STR      "LatestBondDevice"
#define BTC_STORAGE_LATEST_BD_STR       "BdAddr"

#define MAX_BONDED_DEVICE 255

typedef struct {
    char name[BD_NAME_LEN + 1];
    char addr[64];
} Bonded_Device;

typedef struct {
    Bonded_Device* ptBondedDevice[MAX_BONDED_DEVICE];
    int device_count;
} Bonded_Device_List;


/*******************************************************************************
**
** Function         btc_storage_add_bonded_device
**
** Description      BTC storage API - Adds the newly bonded device to NVRAM
**                  along with the link-key, Key type and Pin key length
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_add_bonded_device(bt_bdaddr_t *remote_bd_addr,
        LINK_KEY link_key,
        uint8_t key_type,
        uint8_t pin_length,
        BD_NAME remote_name);

/*******************************************************************************
**
** Function         btc_storage_remove_bonded_device
**
** Description      BTC storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_remove_bonded_device(bt_bdaddr_t *remote_bd_addr);

/*******************************************************************************
**
** Function         btc_storage_remove_bonded_device
**
** Description      BTC storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_load_bonded_devices(void);

/*******************************************************************************
**
** Function         btc_storage_get_num_bt_bond_devices
**
** Description      BTC storage API - get the num of the bonded device from NVRAM
**
** Returns          the num of the bonded device
**
*******************************************************************************/
int btc_storage_get_num_bt_bond_devices(void);

/*******************************************************************************
**
** Function         btc_storage_get_bonded_bt_devices_list
**
** Description      BTC storage API - get the list of the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_get_bonded_bt_devices_list(bt_bdaddr_t *bond_dev, int dev_num);

/*******************************************************************************
**
** Function         btc_storage_get_latest_bonded_devices
**
** Description      BTC storage API - retrieve the lastest bonded device bd address
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_get_latest_bonded_devices(bt_bdaddr_t *bond_dev);

/*******************************************************************************
**
** Function         btc_storage_add_latest_bonded_devices
**
** Description      BTC storage API - add the lastest bonded device bd address
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_add_latest_bonded_devices(bt_bdaddr_t *bond_dev);

/*******************************************************************************
**
** Function         btc_storage_disable_save_bonded_device
**
** Description      BTC storage API - add the lastest bonded device bd address
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_disable_save_bonded_device(void);

/*******************************************************************************
**
** Function         btc_storage_disable_save_bonded_device
**
** Description      BTC storage API - add the lastest bonded device bd address
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_enable_save_bonded_device(void);

/*******************************************************************************
**
** Function         btc_storage_get_latest_bonded_device_name
**
** Description      BTC storage API - retrieve the lastest bonded device bd name
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_get_latest_bonded_device_name(char *device_name);

/*******************************************************************************
**
** Function         btc_storage_get_bonded_device_list
**
** Description      BTC storage API - retrieve the bonded device list
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_get_bonded_device_list(Bonded_Device_List *list);

#endif /* BTC_STORAGE_H */
