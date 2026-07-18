#include "otp_check.h"
#include <stdlib.h>
#include <string.h>
#include "nor/mmp_nor.h"

#define NOR_OTP_SIZE 512
#define NOR_OTP_AESK_SIZE 16
#define NOR_OTP_RSAPKH_SIZE 32
#define NOR_OTP_AESK_FLAG (0x1 << 0)
#define NOR_OTP_RSAPKH_FLAG (0x1 << 1)

void otp_check(uint8_t *aes_key, uint8_t *rsa_public_key_hash, OTP_OPERATION oip_op)
{
    uint8_t otp_flag = 0;
    uint8_t otp_check = 0;
    uint8_t otp_info_write[NOR_OTP_SIZE] = {0};
    uint8_t otp_info_read[NOR_OTP_SIZE] = {0};
    uint32_t otp_index = 0;

    if (oip_op == OTP_OPERATION_AESK) otp_flag = NOR_OTP_AESK_FLAG;
    else if (oip_op == OTP_OPERATION_RSAPKH) otp_flag = NOR_OTP_RSAPKH_FLAG;
    else if (oip_op == OTP_OPERATION_AESK_RSAPKH) otp_flag = (NOR_OTP_AESK_FLAG | NOR_OTP_RSAPKH_FLAG);
    else
    {
        printf("oip_op flag fail!\n");
        sleep(1);
        exit(0);
    }
    otp_info_write[otp_index++] = otp_flag;

    if (otp_info_write[0] & NOR_OTP_AESK_FLAG)
    {
        memcpy(otp_info_write + otp_index, aes_key, NOR_OTP_AESK_SIZE);
        otp_index += NOR_OTP_AESK_SIZE;
    }
    if (otp_info_write[0] & NOR_OTP_RSAPKH_FLAG)
    {
        memcpy(otp_info_write + otp_index, rsa_public_key_hash, NOR_OTP_RSAPKH_SIZE);
        otp_index += NOR_OTP_RSAPKH_SIZE;
    }

    otp_check = 0;
    do
    {
        if (NorWriteOTP(SPI_0, SPI_CSN_0, otp_info_write) != 0)
        {
            otp_check = 1;
            break;
        }
        if (NorReadOTP(SPI_0, SPI_CSN_0, otp_info_read) != 0)
        {
            otp_check = 2;
            break;
        }

        if (otp_info_write[0] & NOR_OTP_AESK_FLAG)
        {
            if (otp_info_read[0] != NOR_OTP_AESK_FLAG &&
                    otp_info_read[0] != (NOR_OTP_AESK_FLAG | NOR_OTP_RSAPKH_FLAG))
            {
                otp_check = 3;
                break;
            }
        }
        if (otp_info_write[0] & NOR_OTP_RSAPKH_FLAG)
        {
            if (otp_info_read[0] != NOR_OTP_RSAPKH_FLAG &&
                    otp_info_read[0] != (NOR_OTP_AESK_FLAG | NOR_OTP_RSAPKH_FLAG))
            {
                otp_check = 4;
                break;
            }
        }

        if (otp_info_write[0] == NOR_OTP_RSAPKH_FLAG)
        {
            if (otp_info_read[0] == (NOR_OTP_AESK_FLAG | NOR_OTP_RSAPKH_FLAG))
            {
                if (memcmp(otp_info_write + 1, otp_info_read + 1 + NOR_OTP_AESK_SIZE, otp_index - 1) != 0)
                {
                    otp_check = 5;
                    /*printf("different_0!\n");
                    for (otp_check = 0; otp_check < otp_index + NOR_OTP_AESK_SIZE; otp_check++)
                        printf("[%d]:%x\n", otp_check, otp_info_read[otp_check]);
                    while (1) sleep(1);*/
                }
                /*else
                {
                    printf("same_0!\n");
                    for (otp_check = 0; otp_check < otp_index + NOR_OTP_AESK_SIZE; otp_check++)
                        printf("[%d]:%x\n", otp_check, otp_info_read[otp_check]);
                    while (1) sleep(1);
                }*/
                break;
            }
        }
        if (memcmp(otp_info_write + 1, otp_info_read + 1, otp_index - 1) != 0)
        {
            otp_check = 6;
            /*printf("different_1!\n");
            for (otp_check = 0; otp_check < otp_index; otp_check++)
                printf("[%d]:%x\n", otp_check, otp_info_read[otp_check]);
            while (1) sleep(1);*/
            break;
        }
        /*else
        {
            printf("same_1!\n");
            for (otp_check = 0; otp_check < otp_index; otp_check++)
                printf("[%d]:%x\n", otp_check, otp_info_read[otp_check]);
            while (1) sleep(1);
        }*/
    } while (0);

    if (otp_check)
    {
        printf("otp_check fail_%d!\n", otp_check);
        /*for (otp_check = 0; otp_check < otp_index; otp_check++)
            printf("[%d]:%x\n", otp_check, otp_info_read[otp_check]);
        while (1) sleep(1);*/
        sleep(1);
        exit(0);
    }
    else
    {
        printf("otp_check pass!\n");
    }
}