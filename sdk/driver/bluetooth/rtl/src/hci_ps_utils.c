/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */


#include <string.h>
#include <stdbool.h>
#include "osif_tp.h"
#include "hci_ps.h"
#include "hci_tp_dbg.h"
void hci_ps_util_hexdump(char dir, const unsigned char *buf, size_t len)
{
    static const char hexdigits[] = "0123456789abcdef";
    char str[64];
    int i;

    if (!buf || !len)
    {
        return;
    }

    str[0] = dir;

    for (i = 0; i < len; i++)
    {
        str[((i % 16) * 3) + 1] = ' ';
        str[((i % 16) * 3) + 2] = hexdigits[buf[i] >> 4];
        str[((i % 16) * 3) + 3] = hexdigits[buf[i] & 0xf];

        if ((i + 1) % 16 == 0)
        {
            str[49] = '\n';
            str[50] = '\0';
            tp_osif_printf(str);
        }
    }

    if (i % 16 > 0)
    {
        size_t j;
        for (j = (i % 16); j < 16; j++)
        {
            str[(j * 3) + 1] = '\n';
            str[(j * 3) + 2] = '\0';
        }
        tp_osif_printf(str);
    }
}


struct t_baudrate_ex
{
    uint32_t baudc;
    uint32_t speed;
} rtk_baudrate[] =
{
    {0x0000701d, 115200},
    {0x0252C00A, 230400},
    {0x00005008, 500000},
    {0x05f75004, 921600}, /* New */
    {0x00005004, 1000000},
    {0x04928002, 1500000},
    {0x00005002, 2000000},
    {0x0000B001, 2500000},
    {0x04928001, 3000000},
    {0x052A6001, 3500000},
    {0x00005001, 4000000},
    {0, 0},
};


void hci_ps_utils_baudc_to_speed(uint32_t baudc, uint32_t *speed)
{
    unsigned int i = 0;

    *speed = rtk_baudrate[i].speed;

    for (i = 0; (rtk_baudrate[i].baudc != 0); i++)
    {
        if (rtk_baudrate[i].baudc == baudc)
        {
            *speed = rtk_baudrate[i].speed;
            return;
        }
    }
    hci_tp_err("!!!!Can not find the baudc 0x%08x.\n", baudc);
    return;
}

void rtlbt_ps_util_speed_to_baudc(uint32_t *baudc, uint32_t speed)
{
    uint32_t i = 0;
    *baudc = rtk_baudrate[i].baudc; //default

    for (i = 0; rtk_baudrate[i].speed != 0; i++)
    {
        if (rtk_baudrate[i].speed == speed)
        {
            *baudc = rtk_baudrate[i].baudc;
            return;
        }
    }
    hci_tp_err("!!!Can not find the speed %u.\n", speed);
    return;
}

uint8_t *global_buf;

bool hci_tp_tx_cb(void)
{
    if (global_buf != NULL)
    {
        tp_osif_free(global_buf);
        global_buf = NULL;
    }
    else
    {
        hci_tp_err("NULL");
    }
    /*TODO free log*/
    return true;
}

bool hci_tp_adapter_send(uint8_t *p_buf, uint16_t len)
{
    global_buf = p_buf;
    hci_ps_hci_num_plus();  //count the hci number
    //hci_tp_dbg("pre_hci_send_num: %x", hci_process_info->patch_info.pre_hci_send_num);
    // util_hexdump('S', p_buf, len);
    return hci_proto_send(p_buf, len, hci_tp_tx_cb);
}

bool hci_tp_send_hci_cmd(uint16_t opcode, uint8_t *data, uint8_t len)
{
    //h4 packe malloc
    uint8_t *p;
    uint16_t idx = 0;

    p = (uint8_t *)tp_osif_malloc(HCI_CMD_HDR_LEN + len);
    if (p != NULL)
    {
        p[idx++] = HCI_CMD_PKT;
        p[idx++] = (uint8_t)(opcode & 0xff);
        p[idx++] = (uint8_t)((opcode >> 8) & 0xff);
        p[idx++] = len;
        if (len)
        {
            memcpy(&p[idx], data, len);
            idx += len;
        }
        return hci_tp_adapter_send(p, idx);
    }
    else
    {
        /*TODO log*/
        hci_tp_err("p == NULL");
        return false;
    }
}

bool hci_tp_send_hci_cmd_with_byte(uint16_t opcode, uint8_t index, uint8_t *data, uint8_t len)
{
    //h4 packe malloc
    uint8_t *p;
    uint16_t idx = 0;

    p = (uint8_t *)tp_osif_malloc(HCI_CMD_HDR_LEN + len + 1);
    if (p != NULL)
    {
        p[idx++] = HCI_CMD_PKT;
        p[idx++] = (uint8_t)(opcode & 0xff);
        p[idx++] = (uint8_t)((opcode >> 8) & 0xff);
        p[idx++] = len + 1;
        p[idx++] = index;
        if (len)
        {
            memcpy(&p[idx], data, len);
            idx += len;
        }
        return hci_tp_adapter_send(p, idx);
    }
    else
    {
        /*TODO log*/
        hci_tp_err("p == NULL");
        return false;
    }
}

//================config_utils=======
#define RTLBT_VENDOR_CONF_MAGIC  0x8723ab55
#define RTLBT_CONF_SIGN_SZ   6
#define RTLBT_CONF_ENT_HDR_SZ   3

bool rtkbt_set_config_param(uint8_t *config_addr, uint16_t cfg_len, uint16_t parm_offset,
                            uint8_t *param, uint8_t param_len)
{
    uint16_t param_all_len = 0;
    uint8_t  *p = config_addr;
    uint32_t signature;
    uint32_t i = 0;

    if (!config_addr || !param || (cfg_len < 6))
    {
        hci_tp_err("Invalid parameters.%p, %p, %x\n", config_addr, param, cfg_len);
        return 0;
    }

    param_all_len = p[5] << 8 | p[4];
    signature = p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
    p += 6;
    //hci_tp_err("=================config len: %x================\n", param_all_len);
    if (signature != RTLBT_VENDOR_CONF_MAGIC)
    {
        hci_tp_err("magic number(%x) is not %x\n", signature, RTLBT_VENDOR_CONF_MAGIC);
        return 0;
    }

    if (param_all_len != cfg_len - RTLBT_CONF_SIGN_SZ)
    {
        hci_tp_warn("config len(%x) is not right(%x), should be %x\n", param_all_len,
                    cfg_len - RTLBT_CONF_SIGN_SZ, cfg_len - RTLBT_CONF_SIGN_SZ);
        param_all_len = cfg_len - RTLBT_CONF_SIGN_SZ;
        //fix the length
        config_addr[4] = param_all_len & 0xff;
        config_addr[5] = param_all_len >> 8;
        //we can fix the length
        //return 1;
    }
    uint16_t temp_offset;
    uint8_t temp_len;
    while (i < param_all_len)
    {
        temp_offset = p[1] << 8 | p[0];
        temp_len = p[2];
        p += RTLBT_CONF_ENT_HDR_SZ;

        if (temp_offset == parm_offset)
        {
            if (param_len != temp_len)
            {
                hci_tp_err("param_len(%x) is not rightn %x\n", param_len, temp_len);
                while (true);
            }
            else
            {
                memcpy(p, param, param_len);
                return true;
            }
        }
        i += (temp_len + RTLBT_CONF_ENT_HDR_SZ);

        p += temp_len;
    }
    hci_tp_err("param_offset(%x) is not founded \n", parm_offset);
    return false;
}

bool rtkbt_get_config_param(uint8_t *config_addr, uint16_t cfg_len, uint16_t parm_offset,
                            uint8_t *param, uint8_t param_len)
{
    uint16_t param_all_len = 0;
    uint8_t  *p = config_addr;
    uint32_t signature;
    uint32_t i = 0;

    if (!config_addr || !param || (cfg_len < 6))
    {
        hci_tp_err("Invalid parameters.%p, %p, %x\n", config_addr, param, cfg_len);
        return false;
    }

    param_all_len = p[5] << 8 | p[4];
    signature = p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
    p += 6;

    if (signature != RTLBT_VENDOR_CONF_MAGIC)
    {
        hci_tp_err("magic number(%x) is not %x\n", signature, RTLBT_VENDOR_CONF_MAGIC);
        return false;
    }
    //hci_tp_err("=================config 222len: %x================\n", param_all_len);
    if (param_all_len != cfg_len - RTLBT_CONF_SIGN_SZ)
    {
        hci_tp_warn("config len(%x) is not right(%x), should be %x\n", param_all_len,
                    cfg_len - RTLBT_CONF_SIGN_SZ, cfg_len - RTLBT_CONF_SIGN_SZ);
        param_all_len = cfg_len - RTLBT_CONF_SIGN_SZ;
        //fix the length
        config_addr[4] = param_all_len & 0xff;
        config_addr[5] = param_all_len >> 8;
        //we can fix the length
        //return 1;
    }
    uint16_t temp_offset;
    uint8_t temp_len;
    while (i < param_all_len)
    {
        temp_offset = p[1] << 8 | p[0];
        temp_len = p[2];
        p += RTLBT_CONF_ENT_HDR_SZ;

        if (temp_offset == parm_offset)
        {
            if (param_len != temp_len)
            {
                hci_tp_err("param_len(%x) is not right %x\n", param_len, temp_len);
                while (true);
            }
            else
            {
                memcpy(param, p, param_len);
                return true;
            }
        }
        i += (temp_len + RTLBT_CONF_ENT_HDR_SZ);
        p += temp_len;
    }
    hci_tp_warn("param_offset(%x) is not founded \n", parm_offset);
    return false;
}

bool rtkbt_delete_last_param(uint8_t *config_addr, uint16_t *cfg_len)
{
    uint16_t param_all_len = 0;
    uint8_t  *p = config_addr;
    uint32_t signature;
    uint32_t i = 0;

    if (!config_addr || (*cfg_len < 6))
    {
        hci_tp_err("Invalid parameters.%p, %x\n", config_addr, *cfg_len);
        return false;
    }

    param_all_len = p[5] << 8 | p[4];
    signature = p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
    p += 6;

    if (signature != RTLBT_VENDOR_CONF_MAGIC)
    {
        hci_tp_err("magic number(%x) is not %x\n", signature, RTLBT_VENDOR_CONF_MAGIC);
        return false;
    }

    if (param_all_len != *cfg_len - RTLBT_CONF_SIGN_SZ)
    {
        param_all_len = *cfg_len - RTLBT_CONF_SIGN_SZ;
        //fix the length
        config_addr[4] = param_all_len & 0xff;
        config_addr[5] = param_all_len >> 8;
        //we can fix the length
        //return 1;
    }
    uint16_t temp_offset;
    uint8_t temp_len = 0U;
    while (i < param_all_len)
    {
        temp_offset = p[1] << 8 | p[0];
        temp_len = p[2];
        p += RTLBT_CONF_ENT_HDR_SZ;
        i += (temp_len + RTLBT_CONF_ENT_HDR_SZ);
        p += temp_len;
    }
    if (temp_len)
    {
        hci_tp_dbg("config offset (%x) len (%x) will be delete, original %x\n", temp_offset, temp_len,
                   param_all_len);
        //param_all_len = p[5] << 8 | p[4];
        param_all_len -= (temp_len + 3);
        *cfg_len -= (temp_len + 3); //change the length
        //fix the length
        config_addr[4] = param_all_len & 0xff;
        config_addr[5] = param_all_len >> 8;
        //we can fix the length
        //return 1;
    }
    return false;
}
