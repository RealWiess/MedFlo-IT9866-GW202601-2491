/**
 * @file         test_suite.h
 *
 * @brief       This file contains only prototypes for test functions
 */

#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include "obd2stack.h"

//1043 TYPE, GPIO
#define  TJA1043_STB_N   16
#define  TJA1043_EN      18

/* CAN Interface */
//int canfd_test();
uint8_t UdsCanWriteMsg(uint16_t canChannel, CAN_Tx *req);
void UdsCanInit(uint32_t canChannel);
void UdsCanDeInit(uint32_t canChannel);
#endif /* TEST_SUITE_H */
