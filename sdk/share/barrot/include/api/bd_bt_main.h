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

#ifndef __BD_BT_MAIN_H__
#define __BD_BT_MAIN_H__

#include "bd_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth stack status type, to indicate whether the bluetooth stack is ready
 */
typedef enum {
    BD_STATUS_UNINITIALIZED   = 0,        /*!< Bluetooth not initialized */
    BD_STATUS_INITIALIZED,                /*!< Bluetooth initialized but not enabled */
    BD_STATUS_ENABLED                     /*!< Bluetooth initialized and enabled */
} bd_status_t;

#define BD_ENABLE_SAVE_BONDED_DEVICE    1
#define BD_DISABLE_SAVE_BONDED_DEVICE   0

/**
 * @brief     Get bluetooth stack status
 *
 * @return    Bluetooth stack status
 *
 */
bd_status_t bd_get_status(void);
    
/**
 * @brief     Enable bluetooth, must after bd_init()
 *
 * @return
 *            - BD_OK : Succeed
 *            - Other  : Failed
 */
bd_err_t bd_enable(void);

/**
 * @brief     Disable bluetooth, must prior to bd_deinit()
 *
 * @return
 *            - BD_OK : Succeed
 *            - Other  : Failed
 */
bd_err_t bd_disable(void);

/**
 * @brief     Init and alloc the resource for bluetooth, must be prior to every bluetooth stuff
 *
 * @return
 *            - BD_OK : Succeed
 *            - Other  : Failed
 */
 bd_err_t bd_init(char* pConfigPath, int bSaveBondedDevice);

/**
 * @brief     Deinit and free the resource for bluetooth, must be after every bluetooth stuff
 *
 * @return
 *            - BD_OK : Succeed
 *            - Other  : Failed
 */
bd_err_t bd_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __BD_BT_MAIN_H__ */
