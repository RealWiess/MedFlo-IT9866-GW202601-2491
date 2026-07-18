#ifndef STL_FA626_SRC__STL_CRC_H_
#define STL_FA626_SRC__STL_CRC_H_

#include <stdint.h>

uint32_t stl_get_crc32_(uint32_t init_crc_val, uint8_t *buf, uint32_t buf_size);

#endif