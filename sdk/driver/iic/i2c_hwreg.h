/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * I2C Registers
 *
 * @author Sammy Chen
 * @version 0.1
 */
#ifndef I2C_HWREG_H
#define I2C_HWREG_H
//#include "host\ahb.h"

#include "ith/ith_defs.h"

/*Memory Base is only for CPU direct mmapping*/
#define REG_I2C_CONTROL                    (0x00U)
#define REG_I2C_STATUS                     (0x04U)
#define REG_I2C_CLOCK_DIV                  (0x08U)
#define REG_I2C_DATA                       (0x0CU)
#define REG_I2C_SLAVE_ADDR                 (0x10U)
#define REG_I2C_ICR                        (0x1CU)
#define REG_I2C_GLITCH                     (0x14U)
#define REG_I2C_BUS_MONITOR                (0x18U)
#define REG_I2C_DATA_RCV_STATE             (0x20U)
#define REG_I2C_REV_NUMBER                 (0x30U)
#define REG_I2C_FEATURE                    (0x34U)

/*ICR Register*/
#define REG_BIT_INTR_TRIG_START            (((uint32_t)1U) << 14U)
#define REG_BIT_INTR_TRIG_ARBITRATION_LOSS (((uint32_t)1U) << 13U)
#define REG_BIT_INTR_TRIG_SLAVE_SELECT     (((uint32_t)1U) << 12U)
#define REG_BIT_INTR_TRIG_STOP             (((uint32_t)1U) << 11U)
#define REG_BIT_INTR_TRIG_BUS_ERROR        (((uint32_t)1U) << 10U)
#define REG_BIT_INTR_TRIG_DATA_RECEIVE     (((uint32_t)1U) << 9U)
#define REG_BIT_INTR_TRIG_DATA_TRANSFER    (((uint32_t)1U) << 8U)

/*Control Register*/
#define REG_BIT_CONTL_CHECK_DOWN_EN        (((uint32_t)1U) << 19U)
#define REG_BIT_CONTL_FIFO_EN              (((uint32_t)1U) << 8U)
#define REG_BIT_CONTL_TRANSFER_BYTE        (((uint32_t)1U) << 7U)
#define REG_BIT_CONTL_NACK                 (((uint32_t)1U) << 6U)
#define REG_BIT_CONTL_STOP                 (((uint32_t)1U) << 5U)
#define REG_BIT_CONTL_START                (((uint32_t)1U) << 4U)
#define REG_BIT_CONTL_GC                   (((uint32_t)1U) << 3U)
#define REG_BIT_CONTL_CLK_ENABLE           (((uint32_t)1U) << 2U)
#define REG_BIT_CONTL_I2C_ENABLE           (((uint32_t)1U) << 1U)
#define REG_BIT_CONTL_I2C_RESET            (((uint32_t)1U) << 0U)

#define REG_BIT_INTR_TRIG                  REG_BIT_INTR_TRIG_ARBITRATION_LOSS | REG_BIT_INTR_TRIG_SLAVE_SELECT | \
                                           REG_BIT_INTR_TRIG_BUS_ERROR | REG_BIT_INTR_TRIG_DATA_RECEIVE | \
                                           REG_BIT_INTR_TRIG_DATA_TRANSFER

#define REG_BIT_INTR_ALL                   REG_BIT_INTR_TRIG_START | \
                                           REG_BIT_INTR_TRIG_ARBITRATION_LOSS | \
                                           REG_BIT_INTR_TRIG_SLAVE_SELECT | \
                                           REG_BIT_INTR_TRIG_STOP | \
                                           REG_BIT_INTR_TRIG_BUS_ERROR | \
                                           REG_BIT_INTR_TRIG_DATA_RECEIVE | \
                                           REG_BIT_INTR_TRIG_DATA_TRANSFER

/*Status Register*/
#define REG_BIT_STATUS_DATA_END_CLEAR      (((uint32_t)1U) << 29U)
#define REG_BIT_STATUS_CHECK_CLEAR         (((uint32_t)1U) << 28U)
#define REG_BIT_STATUS_START               (((uint32_t)1U) << 11U)
#define REG_BIT_STATUS_ARBITRATION_LOSS    (((uint32_t)1U) << 10U)
#define REG_BIT_STATUS_SLAVE_SELECT        (((uint32_t)1U) << 8U)
#define REG_BIT_STATUS_STOP                (((uint32_t)1U) << 7U)
#define REG_BIT_STATUS_BUS_ERROR           (((uint32_t)1U) << 6U)
#define REG_BIT_STATUS_DATA_RECEIVE_DONE   (((uint32_t)1U) << 5U)
#define REG_BIT_STATUS_DATA_TRANSFER_DONE  (((uint32_t)1U) << 4U)
#define REG_BIT_STATUS_NON_ACK             (((uint32_t)1U) << 1U)
#define REG_BIT_STATUS_RECEIVE_MODE        (((uint32_t)1U) << 0U)

/*Data Register*/
#define REG_BIT_READ_ENABLE                (((uint32_t)1U) << 0U)

/*Clcok Division Register*/
#define REG_MASK_CLK_DIV_COUNT             (0x3FFFFU)

/*Hold Time & Glitch Suppression Setting Register*/
#define REG_MASK_GSR                       (0x1C00U)
#define REG_SHIFT_GSR                      (10U)
#define REG_MASK_TSR                       (0x3FFU)

#endif
