#include "stl_rom.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <ite/ith.h>
#include <ite/itp.h>
#include "stl_crc_.h"

#define BLOCK_SIZE (64 * 1024)
//#define DEBUG

typedef struct {
    uint32_t start_address;
    uint32_t size;
    uint32_t crc;
} STL_ROM_REGION;

typedef enum {
    STL_ROM_TEST_STATE_UNINIT = 0,
    STL_ROM_TEST_STATE_INIT,
    STL_ROM_TEST_STATE_READ_NEXT_BLOCK,
    STL_ROM_TEST_STATE_CRC_1K,
    STL_ROM_TEST_STATE_ERROR,
} STL_ROM_TEST_STATE;

static uint8_t            nor_data_[BLOCK_SIZE];
static STL_ROM_REGION     test_region_[] =
{
    {                  0x0},
#ifdef CFG_BOOTLOADER_ENABLE
    {CFG_UPGRADE_IMAGE_POS}
#endif
};
static uint32_t           test_region_index_;
static STL_ROM_TEST_STATE test_state_;
static uint32_t           current_test_address_;
static uint32_t           current_crc_;

#define TEST_REGION_COUNT (sizeof(test_region_) / sizeof(test_region_[0]))

// Format of ROM Code for NOR flash.
// Syntax                |    No. of Bytes  | Descr.
// ----------------------|------------------|------------------------
// NOR () {              |                  |
//     Header            | 8                | "SMEDIA01"/"SMEDIA02"
//     Offset            | 4                | Absolute postion of Script_len.
//                       |                  | (big endian value, maximun 504)
//     Header_len        | 4                | Total header length. (big endian)
//     if (compressed) { |                  |
//         C_DATA_LEN    | 4                | UCL Compressed NOR_DATA length
//     }                 |                  |
//     DATA              | 4*N              | Reserved Area
//     Script_len        | 4                | Number of MMIO setting. (big endian)
//     INIT_SCR()        |                  | Initial Script
//     Nor_CRC32         | 4                | 0xffffffff will skip the CRC check
//     Nor_Len           | 4                | Length of RAW NOR_DATA (big endian)
//     NOR_DATA          | N                |
// }
stl_status stl_get_image_size_(const uint8_t *buf, uint32_t *size)
{
#define _STL_HEADER_LEN_OFFSET (12)
#define _STL_C_DATA_LEN_OFFSET (16)

    *size = 0;
    if (buf != NULL)
    {
        const uint8_t *nor_data;

        uint32_t      header_len = *((uint32_t *)(buf + _STL_HEADER_LEN_OFFSET));
        header_len = itpBetoh32(header_len);
        if (header_len > 0x1000)
            return stl_error;

        nor_data   = buf + header_len;
        if (strncmp(nor_data, "SMAZ", 4) == 0)
        {
            uint32_t c_data_len = *((uint32_t *)(buf + _STL_C_DATA_LEN_OFFSET));
            c_data_len = itpBetoh32(c_data_len);
            *size      = c_data_len + header_len;
        }
        else
        {
            uint32_t data_len = *((uint32_t *)(nor_data - 4));
            data_len = itpBetoh32(data_len);
            *size    = data_len + header_len;
        }

        if (*size > CFG_UPGRADE_NOR_IMAGE_SIZE)
            return stl_error;

        if (*size < 100)
            return stl_error;
    }
    return stl_success;
}

stl_status stl_rom_read_64kbytes_(uint8_t *buf, uint32_t pos)
{
    stl_status status      = stl_error;
    uint32_t   block_size  = 0;
    uint32_t   block_count = 0;

    int        fd          = open(":nor", O_RDONLY, 0);
    if (fd == -1)
    {
        // "open device error\n"
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
        goto end;
    }
    if (0 != ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &block_size))
    {
        // "get block size error\n"
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
        goto end;
    }
    if (0 == block_size)
    {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
        goto end;
    }
    pos = pos / block_size;
    if (pos != lseek(fd, pos, SEEK_SET))
    {
        // "seek to rom position %d error\n", pos
#ifdef DEBUG
        printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
        goto end;
    }
    block_count = BLOCK_SIZE / block_size;
    if (read(fd, nor_data_, block_count) != block_count)
    {
        // "read norflash error\n"
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
        goto end;
    }

    status      = stl_success;
end:
    if (fd != -1)
        close(fd);

    return status;
}

stl_status stl_rom_get_nor_crc_(
    uint32_t *ret_crc,
    uint32_t nor_address,
    uint32_t size)
{
    stl_status status  = stl_success;
    uint32_t   pos;
    uint32_t   end_pos = nor_address + size;

    *ret_crc = 0xffffffff;
    for (pos = nor_address; pos < end_pos; pos += BLOCK_SIZE)
    {
        status = stl_rom_read_64kbytes_(nor_data_, pos);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
            goto end;
        }
        *ret_crc = stl_get_crc32_(*ret_crc, nor_data_, BLOCK_SIZE);
#ifdef DEBUG
        printf("1: addr(0x%08X) crc(0x%08X)\n", pos, *ret_crc);
#endif
    }

end:
    return status;
}

stl_status stl_rom_test_init(void)
{
    uint32_t   img_size;
    stl_status status = stl_error;
    int32_t    i;

    for (i = 0; i < TEST_REGION_COUNT; i++)
    {
        status = stl_rom_read_64kbytes_(nor_data_, test_region_[i].start_address);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
            goto fail;
        }
        status = stl_get_image_size_(nor_data_, &img_size);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
            goto fail;
        }
        test_region_[i].size = ITH_ALIGN_UP(img_size, BLOCK_SIZE);

        status = stl_rom_get_nor_crc_(
            &(test_region_[i].crc),
            test_region_[i].start_address,
            test_region_[i].size);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
            goto fail;
        }
    }

    test_region_index_ = 0;
    test_state_        = STL_ROM_TEST_STATE_INIT;

    return stl_success;

fail:
    return status;
}

stl_status stl_rom_runtime_test(void)
{
    stl_status status = stl_success;

    switch (test_state_)
    {
    case STL_ROM_TEST_STATE_CRC_1K:
        // Calculate the CRC value of 1K size data
        current_crc_ = stl_get_crc32_(
            current_crc_,
            &nor_data_[current_test_address_ % BLOCK_SIZE],
            1024);

#ifdef DEBUG
        printf("3: addr(0x%08X) crc(0x%08X)\n", current_test_address_, current_crc_);
#endif
        current_test_address_ += 1024;
        if (current_test_address_ == test_region_[test_region_index_].start_address
            + test_region_[test_region_index_].size)
        {
#ifdef DEBUG
            printf("2: addr(0x%08X) crc(0x%08X)\n",
                   current_test_address_ - BLOCK_SIZE,
                   current_crc_);
#endif

            if (test_region_[test_region_index_].crc != current_crc_)
            {
#ifdef DEBUG
                printf("CRC NOT MATCH(init(0x%08X) - curr(0x%08X))\n",
                       test_region_[test_region_index_].crc, current_crc_);
#endif
                test_state_ = STL_ROM_TEST_STATE_ERROR;
                return stl_error;
            }

            test_region_index_++;
            // cppcheck-suppress moduloofone
            test_region_index_    = test_region_index_ % TEST_REGION_COUNT;
            current_test_address_ = test_region_[test_region_index_].start_address;
            current_crc_          = 0xffffffff;

            test_state_           = STL_ROM_TEST_STATE_READ_NEXT_BLOCK;
        }
        else if (current_test_address_ % BLOCK_SIZE == 0)
        {
            test_state_ = STL_ROM_TEST_STATE_READ_NEXT_BLOCK;

#ifdef DEBUG
            printf("2: addr(0x%08X) crc(0x%08X)\n",
                   current_test_address_ - BLOCK_SIZE,
                   current_crc_);
#endif
        }
        break;

    case STL_ROM_TEST_STATE_READ_NEXT_BLOCK:
        status = stl_rom_read_64kbytes_(
            nor_data_,
            current_test_address_);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("[NOR] read data fail! addr=0x%08X\n", current_test_address_);
#endif
            test_state_ = STL_ROM_TEST_STATE_ERROR;
            return stl_error;
        }
        test_state_ = STL_ROM_TEST_STATE_CRC_1K;
        break;

    case STL_ROM_TEST_STATE_ERROR:
        return stl_error;

    case STL_ROM_TEST_STATE_INIT:
        status = stl_rom_read_64kbytes_(
            nor_data_,
            test_region_[test_region_index_].start_address);
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("[NOR] read data fail! addr=0x%08X\n", test_region_[test_region_index_].start_address);
#endif
            test_state_ = STL_ROM_TEST_STATE_ERROR;
            return stl_error;
        }

        current_test_address_ = test_region_[test_region_index_].start_address;
        current_crc_          = 0xffffffff;
        test_state_           = STL_ROM_TEST_STATE_CRC_1K;
        break;

    case STL_ROM_TEST_STATE_UNINIT:
        status                = stl_rom_test_init();
        if (status != stl_success)
        {
#ifdef DEBUG
            printf("%s(%d)\n", __FUNCTION__, __LINE__);
#endif
            test_state_ = STL_ROM_TEST_STATE_ERROR;
            return stl_error;
        }
        break;
    }

    return status;
}