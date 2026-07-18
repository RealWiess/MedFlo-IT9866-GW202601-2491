/*
 * Copyright (c) 2022 ITE Tech. Inc.
 */

#ifndef ITE_ITDPU_H_
#define ITE_ITDPU_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ite_itdpu_crc32(const uint8_t *data, size_t len, uint32_t *crc);

int itdpu_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ITE_ITDPU_H_ */
