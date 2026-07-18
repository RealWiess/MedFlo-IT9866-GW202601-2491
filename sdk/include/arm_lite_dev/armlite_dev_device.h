/** @file
 * ARMLite device library.
 *
 * @author Kevin Chen
 * @version 1.0
 * @date 2019/10/01
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup ARMLite device library
 *  @{
 */
#ifndef ARMLITE_DEVICE_H
#define ARMLITE_DEVICE_H

#include "ite/itp.h"
#include "ite/ite_risc.h"
#ifndef _MSC_VER
#include "mmio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ARMLITE_UNKNOWN_DEVICE        0
#define ARMLITE_OPUS_CODEC            1
#define ARMLITE_HUD_BG_FAST           2
#define ARMLITE_CANBUS                3
#define ARMLITE_LCD_CRC_CAL           4

#define ITP_IOCTL_ARMLITE_SWITCH_ENG  ITP_IOCTL_CUSTOM_CTL_ID0

#define REQUEST_CMD_REG               USER_DEFINE_REG30
#define RESPONSE_CMD_REG              USER_DEFINE_REG31

#define ARMLITE_COMMAND_REG_READ      ithReadRegA
#define ARMLITE_COMMAND_REG_WRITE     ithWriteRegA

#ifdef __cplusplus
}
#endif

#endif // ARMLITE_DEVICE_H
/** @} */ // end of ARMLITE_DEVICE_H
