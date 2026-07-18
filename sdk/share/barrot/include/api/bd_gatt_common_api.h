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

#include <stdint.h>
#include <stdbool.h>

#include "bd_err.h"
#include "bd_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum Transmission Unit used in GATT
#define BD_GATT_DEF_BLE_MTU_SIZE   23U   /* relate to GATT_DEF_BLE_MTU_SIZE in stack/gatt_api.h */

// Maximum Transmission Unit allowed in GATT
#define BD_GATT_MAX_MTU_SIZE       517U  /* relate to GATT_MAX_MTU_SIZE in stack/gatt_api.h */

/**
 * @brief           This function is called to set local MTU,
 *                  the function is called before BLE connection.
 *
 * @param[in]       mtu: the size of MTU.
 *
 * @return
 *                  - BD_OK: success
 *                  - other: failed
 *
 */
extern bd_err_t bd_ble_gatt_set_local_mtu (uint16_t mtu);

#ifdef __cplusplus
}
#endif
