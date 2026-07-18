/** @file
 * ALT CPU device library.
 *
 * @author Steven Hsiao
 * @version 1.0
 * @date 2018
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup ALT CPU Utility API declaration
 *  @{
 */
#ifndef ALT_CPU_UTILITY_API_H
#define ALT_CPU_UTILITY_API_H

#ifdef __cplusplus
extern "C" {
#endif

//GPIO
void setGpioDir(unsigned long gpioPin, unsigned long bIn);

void setGpioMode(unsigned long gpioPin, unsigned long mode);

unsigned long getGpioValue(unsigned long gpioPin, unsigned long bIn);

void setGpioValue(unsigned long gpioPin, unsigned long bHigh);

//TickTimer
unsigned long getCurTimer(unsigned int timer);

unsigned long getTimer(unsigned int timer);

void resetTimer(unsigned int timer);

void stopTimer(unsigned int timer);

void startTimer(unsigned int timer);

unsigned long getDuration(unsigned int timer, unsigned long previousTick);

unsigned long getDurationInUs(unsigned int timer, unsigned long previousTick, unsigned long tickPerUs);

#ifdef __cplusplus
}
#endif

#endif // ALT_CPU_UTILITY_API_H
/** @} */ // end of ALT_CPU_UTILITY_API_H
