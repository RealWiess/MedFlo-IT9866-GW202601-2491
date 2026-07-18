/** @file
 * 
 *
 * @author 
 * @version 1.0
 * @date
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */

#ifndef CAN_HANDLE_H
#define CAN_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup
 *  @{
 */

/**
 * Initializes can module.
 */
void canInit(void);

/**
 * Exits can module.
 */
void canExit(void);

/**
 * system go to sleep, power down
 */
void canSleep(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_HANDLE_H */
/** @} */