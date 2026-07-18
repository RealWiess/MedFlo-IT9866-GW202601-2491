/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "hci_ps.h"

#include "bt_uart.h"
#include "hci_board_if.h"
#include "osif_tp.h"
#include "rtlbt_common.h"
#include "hci_tp_dbg.h"
#include "hci_uart.h"

//=============================================patch=config======
#define PATCH_FRAGMENT_MAX_SIZE         252
typedef struct
{
    uint8_t             *fw_buf;
    uint16_t            fw_len;
    uint8_t             *config_buf;
    uint16_t            config_len;
    uint32_t            buf_free_flag;

    uint16_t            pre_hci_send_num;

    uint8_t             end_index;
    uint8_t             last_pkt;
    uint8_t             addi_pkt;
    uint8_t             total_index;
    uint8_t             curr_index;
    uint16_t            data_sent;
    uint8_t             *pos;
} patch_frag_info;
static patch_frag_info         *internal_patch_info;

T_HCI_PROCESS_INFO *hci_process_info;
T_PATCH_CONFIG *p_rtkbt_config_info = &rtkbt_config_info;
bool rtk_hci_pc_init(P_HCI_TP_RX_IND rx_ind)
{
    uint32_t tmp_flag;
    bool ret = false;

    hci_pos_process_init();
    hci_process_info = tp_osif_malloc(sizeof(T_HCI_PROCESS_INFO));
    internal_patch_info = tp_osif_malloc(sizeof(patch_frag_info));
    if ((internal_patch_info == NULL) || (hci_process_info == NULL))
    {
        hci_tp_err("internal_patch_info is %p, hci_process_info is %p", internal_patch_info,
                   hci_process_info);
    }
    // hci_tp_err("hci_process_info addr:%p, %x, internal_patch_info:%p, %x",hci_process_info, sizeof(T_HCI_PROCESS_INFO), internal_patch_info,sizeof(patch_frag_info));
    memset(hci_process_info, 0, sizeof(T_HCI_PROCESS_INFO));
    memset(internal_patch_info, 0, sizeof(patch_frag_info));
    strlcpy(hci_process_info->hci_stack_version, HCI_STACK_VERSION, sizeof (hci_process_info->hci_stack_version));
    if (rtk_hci_board_init(hci_process_info) == true)
    {
        //
        if ((hci_process_info->config_len == 0) && (hci_process_info->fw_len == 0))
        {
            // jump the patch
        }
    }
    else
    {
        //load the default
        hci_process_info->change_flag = 0;
        hci_process_info->fw_buf  = (uint8_t *)rtlbt_fw;
        hci_process_info->fw_len = rtlbt_fw_len;
        hci_process_info->config_buf = rtlbt_config;
        hci_process_info->config_len = rtlbt_config_len;
        hci_process_info->proto = p_rtkbt_config_info->rtlbt_fw_hci_proto;
        //load param
        //load baudrate
        //load bd_addr
        //load fw_log
        //load
        //return true;
    }
    ;

    tmp_flag = hci_process_info->change_flag;



    if (tmp_flag == 0) //load the default  not change
    {
        // do default thing
        // hci_process_info->patch_info.config_buf = rtlbt_config;
        // hci_process_info->patch_info.config_len = rtlbt_config_len;
        //load the baudrate
        //do nothing
        //load flow ctrl
        //load log
        // hci_process_info->patch_info.fw_buf = rtlbt_fw;
        //  hci_process_info->patch_info.fw_len = rtlbt_fw_len;
        //return true;

        internal_patch_info->config_buf = rtlbt_config;
        internal_patch_info->config_len = rtlbt_config_len;
        internal_patch_info->fw_buf = (uint8_t *)rtlbt_fw;
        internal_patch_info->fw_len = rtlbt_fw_len;
    }

    if (tmp_flag & RTLBT_FL_FW_CONFIG)
    {
        //set the config_addr load to info
        internal_patch_info->config_buf = hci_process_info->config_buf;
        internal_patch_info->config_len = hci_process_info->config_len;
        internal_patch_info->fw_buf = hci_process_info->fw_buf;
        internal_patch_info->fw_len = hci_process_info->fw_len;
    }
    else
    {
        internal_patch_info->config_buf = rtlbt_config;
        internal_patch_info->config_len = rtlbt_config_len;
        //load the config from config.c
    }

    if (tmp_flag & RTLBT_FL_FW_PATCH)
    {
        //set the patch_addr  load to info
        internal_patch_info->fw_buf = hci_process_info->fw_buf;
        internal_patch_info->fw_len = hci_process_info->fw_len;
        hci_tp_warn("==================USE THE EXT FW_PATCH===============");
    }
    else
    {
        // use the extern patch
        //check the merged patch
        hci_process_info->fw_buf  = (uint8_t *)rtlbt_fw;
        hci_process_info->fw_len = rtlbt_fw_len;
        //internal_patch_info->fw_buf = rtlbt_fw;
        //internal_patch_info->fw_len = rtlbt_fw_len;
    }

    //first check the flowctrl
    //get the uart config

    uint8_t uart_config[0x10];
    uint32_t rtk_baud;
    uint32_t uart_baudrate;
    ret = rtkbt_get_config_param(internal_patch_info->config_buf,
                                 internal_patch_info->config_len,
                                 p_rtkbt_config_info->rtlbt_config_uart_offset,   uart_config, 0x10);
    if (ret)
    {
        if (tmp_flag & RTLBT_FL_SET_BAUDRATE)
        {
            //set baudrate
            //change baudrate
            //set baudrate
            rtk_baud = uart_config[0] | (uart_config[1] << 8) | (uart_config[2] << 16) | (uart_config[3] << 24);

            hci_ps_utils_baudc_to_speed(rtk_baud, &uart_baudrate);

            rtlbt_ps_util_speed_to_baudc(&rtk_baud, hci_process_info->baudrate);
            uart_config[0] = rtk_baud & 0xff;
            uart_config[1] = (rtk_baud >> 8) & 0xff;
            uart_config[2] = (rtk_baud >> 16) & 0xff;
            uart_config[3] = (rtk_baud >> 24) & 0xff;
            hci_tp_warn("the original baudrate is %d, change to %d, %x ", uart_baudrate,
                        hci_process_info->baudrate, rtk_baud);
            //set the config baudrate
        }
        else
        {
            //load the
            rtk_baud = uart_config[0] | (uart_config[1] << 8) | (uart_config[2] << 16) | (uart_config[3] << 24);
            hci_ps_utils_baudc_to_speed(rtk_baud, &hci_process_info->baudrate);
            hci_tp_dbg("rtk_baud:%x, baudarte:%d", rtk_baud, hci_process_info->baudrate);
        }

        if (tmp_flag & RTLBT_FL_FW_FLOW_CTRL)
        {
            bool flow_ctl_flag = hci_process_info->flow_ctrl;
            if (flow_ctl_flag)
            {
                uart_config[12] |= 0x04;
            }
            else
            {
                uart_config[12] &= ~0x04;
            }

        }
        else
        {
            if (uart_config[12] & 0x04)
            {
                hci_process_info->flow_ctrl = true;
            }
            else
            {
                hci_process_info->flow_ctrl = false;
            }
            hci_tp_dbg("hci_process_info->flow_ctrl: %x", hci_process_info->flow_ctrl);
            //load the flwoctrl
        }
        rtkbt_set_config_param(internal_patch_info->config_buf,
                               internal_patch_info->config_len,
                               p_rtkbt_config_info->rtlbt_config_uart_offset,   uart_config, 0x10);
    }
    else
    {
        hci_tp_err("the configlen is %d not use config use baudrate:%d\r\n", hci_process_info->config_len,
                   hci_process_info->baudrate);
        if (tmp_flag & RTLBT_FL_SET_BAUDRATE)
        {

        }
    }
    //save the uart config
    //init hci uart

    //reset the bt
    rtkbt_reset();
    hci_process_info->proto = p_rtkbt_config_info->rtlbt_fw_hci_proto;
    rtk_hci_ps_uart_init(hci_process_info->proto, rx_ind);
    //rtk_hci_uart_init(priority, 115200, hci_process_info->flow_ctrl, rx_ind);

    if (tmp_flag & RTLBT_FL_EXT_BDADDR)
    {
        ret = rtkbt_set_config_param(internal_patch_info->config_buf,
                                     internal_patch_info->config_len,
                                     p_rtkbt_config_info->rtlbt_config_bd_addr_offset,    hci_process_info->bdaddr, 6);
        if (ret == true)
        {
            //ok
            hci_tp_err("============Changed BT ADDRESS OK=============");
        }
        else
        {
            //fail
            hci_tp_err("the BD ADDRESS OFFSET IS NOT IN THE CONFIG PLEASE ADD INTO IF YOU WANT CHANGE THE BDADRESS");
        }
        //set the config bdaddr
        //print the bd addr;
    }
    else
    {
        //config may not in
        ret =  rtkbt_get_config_param(internal_patch_info->config_buf,
                                      internal_patch_info->config_len,
                                      p_rtkbt_config_info->rtlbt_config_bd_addr_offset, hci_process_info->bdaddr, 6);

        if (ret == true)
        {
            //DEFAULT USE THE EFUSE STRIP THE CONFIG
            rtkbt_delete_last_param(internal_patch_info->config_buf, &internal_patch_info->config_len);
            //hci_tp_err("===========the last config len is %d===========", internal_patch_info->config_len);
        }
        else
        {
            //DEFAULT IS NO BDADDRESS, OK
        }

        //print the bd s;
    }

    if (tmp_flag & RTLBT_FL_FW_LOG)
    {
        uint8_t fw_log_open_byte = 0;
        if (hci_process_info->fw_log == true)
        {
            fw_log_open_byte = p_rtkbt_config_info->rtlbt_config_fw_log_on;
        }
        else
        {
            fw_log_open_byte = p_rtkbt_config_info->rtlbt_config_fw_log_off;
        }

        if (false == rtkbt_set_config_param(internal_patch_info->config_buf,
                                            internal_patch_info->config_len,
                                            p_rtkbt_config_info->rtlbt_config_fw_log_offset, &fw_log_open_byte, 1))
        {
            hci_tp_err("there is no fw log offset");
        }
        else
        {
            //hci_tp_err("==============Set the fw log is ok====================");
        }
    }
    else
    {
        //          //config may not in
        uint8_t fw_log_value;
        rtkbt_get_config_param(internal_patch_info->config_buf,
                               internal_patch_info->config_len,
                               p_rtkbt_config_info->rtlbt_config_fw_log_offset, &fw_log_value, 1);
        if (fw_log_value == p_rtkbt_config_info->rtlbt_config_fw_log_on)
        {
            hci_process_info->fw_log = true;
        }
        else
        {
            hci_process_info->fw_log = false;
        }
        //load the default value;
        //or do noting
    }

//  }

    hci_tp_dbg("the hci transport info:%s floctrl, %d baudrate",
               (uart_config[12] & 0x4) ? "open" : "close",
               hci_process_info->baudrate);
    if (hci_process_info->proto == RTLBT_UART_H5)
    {
        /* */

    }
    else
    {

    }

    ///free hci_process_info

    //=======init the timer========
    return true;
}
///==========================================internal====func===========
bool rtk_hci_ps_deinit(void)
{
    tp_osif_free(internal_patch_info);
    tp_osif_free(hci_process_info);
    internal_patch_info = NULL;
    hci_process_info = NULL;
    return true;
}

uint8_t hci_pc_read_local_ver(void)
{
    bool ret = false;
    ret = hci_tp_send_hci_cmd(HCI_READ_LOCAL_VERSION_INFO, NULL, 0);
    if (ret == true)
    {
        /*TODO:  send_hci_index++ */
        return HCI_TP_CHECK_OK;
    }
    hci_tp_err("%s:[%d] \n", __FUNCTION__, __LINE__);
    return HCI_TP_CONFIG_FAIL;
}

uint8_t hci_pc_read_local_ver_check(uint8_t len, uint8_t *p_buf)
{
    (void)len;
    uint16_t    lmp_subver;
    uint8_t     hci_ver;
    uint16_t    hci_rev;
    uint8_t    lmp_ver;
    uint16_t  manufacture_name;
    uint8_t idx = 0;
    /* Skip status, hci version, hci revision, lmp version & manufacture name */
    hci_ver = p_buf[idx++];
    hci_rev = p_buf[idx] | (p_buf[idx + 1] << 8);
    idx += 2;
    lmp_ver = p_buf[idx++];
    manufacture_name = p_buf[idx] | (p_buf[idx + 1] << 8);
    idx += 2;
    lmp_subver = p_buf[idx] | (p_buf[idx + 1] << 8);
    if (lmp_subver != p_rtkbt_config_info->rtlbt_lmp_subver)
    {
        hci_tp_err("BT lmp_subver is 0x%04x, we current lmp_subver:0x%04x,please check the rtl%04x%x_config.c file",
                   lmp_subver, p_rtkbt_config_info->rtlbt_lmp_subver, lmp_subver, hci_rev);
        return HCI_TP_CHECK_ERROR;
    }

    if ((p_rtkbt_config_info->rtlbt_fw_check_flag & RTL_FW_MATCH_HCI_VER) &&
        (hci_ver != p_rtkbt_config_info->rtlbt_hci_ver))
    {
        hci_tp_err("FLAG:%x, BT hci_ver is 0x%02x, we current hci_ver:0x%02x,please check the rtl%04x%x_config.c file",
                   p_rtkbt_config_info->rtlbt_fw_check_flag,
                   hci_ver, p_rtkbt_config_info->rtlbt_hci_ver, lmp_subver, hci_rev);
        return HCI_TP_CHECK_ERROR;
    }
    if ((p_rtkbt_config_info->rtlbt_fw_check_flag & RTL_FW_MATCH_HCI_REV) &&
        (hci_rev != p_rtkbt_config_info->rtlbt_hci_rev))
    {
        hci_tp_err("BT hci_rev is 0x%04x, we current hci_rev:0x%04x,please check the rtl%04x%x_config.c file",
                   hci_rev, p_rtkbt_config_info->rtlbt_hci_rev, lmp_subver, hci_rev);
        return HCI_TP_CHECK_ERROR;
    }
    hci_tp_dbg("lmp_subver 0x%04x, manufacture_name:%x, lmp_ver:%x, hci_ver %x, hci_rev:%x, check_flag:%x",
               lmp_subver,
               manufacture_name,
               lmp_ver, hci_ver, hci_rev, p_rtkbt_config_info->rtlbt_fw_check_flag);
    /*TODO: save the lmp_subversion to find patch*/
    return HCI_TP_CHECK_OK;
}

uint8_t hci_pc_read_rom_ver(void)
{
    bool ret = false;
    ret = hci_tp_send_hci_cmd(HCI_VSC_READ_ROM_VERSION, NULL, 0);
    if (ret == true)
    {
        /*TODO:find the patchinfo  send_hci_index++ */
        return HCI_TP_CHECK_OK;
    }
    hci_tp_err("ret = false \n");
    return HCI_TP_CONFIG_FAIL;
}
bool hci_rtk_find_patch(uint8_t chip_id, patch_frag_info *patch_info,
                        T_HCI_PROCESS_INFO *hci_board_info)
{
    uint16_t            mp_num_of_patch = 0;
    uint16_t            fw_chip_id = 0;
    uint32_t            fw_chip_offset = 0;
    uint8_t             i;
    uint8_t             *orignal_fw_patch = hci_board_info->fw_buf;
    uint32_t            lmp_subversion;;
    //check the merged patch
    if (orignal_fw_patch == NULL)
    {
        hci_tp_err("orignal_fw_patch is NULL");
        return false;
    }
    if (memcmp(hci_board_info->fw_buf, "Realtech", sizeof("Realtech") - 1))
    {
        //if single patch or merged patch
        patch_info->fw_buf = hci_board_info->fw_buf;
        patch_info->fw_len = hci_board_info->fw_len;
        hci_tp_info("orignal_fw_patch is single patch");
    }
    else
    {

        memcpy(&lmp_subversion, hci_board_info->fw_buf + 8, 4);
        hci_tp_info("orignal_fw_patch is merged patch, %x\r\n", lmp_subversion);
        mp_num_of_patch = orignal_fw_patch[0x0c] | (orignal_fw_patch[0x0d] << 8);
        if (mp_num_of_patch == 1)
        {
            //we use only patch
            //check chip_id
            fw_chip_id = orignal_fw_patch[0x0e] | (orignal_fw_patch[0x0f] << 8);
            if (fw_chip_id != chip_id)
            {
                hci_tp_warn("patch is %x, chip_rom_version is:%x, only patch ,we use it", fw_chip_id, chip_id);
            }
            patch_info->fw_len  =  orignal_fw_patch[0x0e + 2] | (orignal_fw_patch[0x0f + 2] << 8);
            fw_chip_offset = orignal_fw_patch[0x0e + 4] | (orignal_fw_patch[0x0f + 4] << 8);

        }
        else
        {
            //we use the more patch
            for (i = 0 ; i < mp_num_of_patch; i++)
            {
                fw_chip_id = orignal_fw_patch[0x0e + 2 * i] | (orignal_fw_patch[0x0f + 2 * i] << 8);

                if (fw_chip_id == chip_id)
                {
                    patch_info->fw_len  =  orignal_fw_patch[0x0e + 2 * mp_num_of_patch + 2 * i] |
                                           (orignal_fw_patch[0x0f + 2 * mp_num_of_patch + 2 * i] << 8);
                    fw_chip_offset = orignal_fw_patch[0x0e + 4 * mp_num_of_patch + 4 * i] |
                                     (orignal_fw_patch[0x0f + 4 * mp_num_of_patch + 4 * i] << 8);
                    break;
                }
            }
            if (i >= mp_num_of_patch)
            {
                hci_tp_dbg("\n ERROR:no match patch\n");
                return false;
            }
        }

        patch_info->fw_buf = tp_osif_malloc(patch_info->fw_len);
        if (patch_info->fw_buf == NULL)
        {
            hci_tp_dbg("\n fw_buf ,malloc %d byte fail, \n", patch_info->fw_len);
            return false;
        }
        else
        {
            memcpy(patch_info->fw_buf, orignal_fw_patch + fw_chip_offset, patch_info->fw_len);
            memcpy(patch_info->fw_buf + patch_info->fw_len - 4, &lmp_subversion, 4);
        }

        hci_tp_dbg("Current patch:lmp_subversion:%x, mp_num_of_patch:%x,chip_id:%x, len:%x, offset:%x",
                   lmp_subversion, mp_num_of_patch, fw_chip_id, patch_info->fw_len, fw_chip_offset);
    }
    return true;
}

uint8_t hci_pc_read_rom_ver_check(uint8_t len, uint8_t *p_buf)
{
    (void)len;
    uint8_t    rom_version;
    patch_frag_info *path_info = internal_patch_info;
    rom_version = p_buf[0];
    if ((p_rtkbt_config_info->rtlbt_fw_check_flag & RTL_FW_MATCH_ECO_VER) &&
        (rom_version != p_rtkbt_config_info->rtlbt_eco_version))
    {
        hci_tp_err("BT rom_version is 0x%02x, we current rom_version:0x%02x,please check the rtl%04x_config.c file",
                   rom_version, p_rtkbt_config_info->rtlbt_eco_version, p_rtkbt_config_info->rtlbt_lmp_subver);
        while (true);
    }
    hci_tp_info("rom_version 0x%02x", rom_version);
    /*TODO: save the chip id  to find patch*/
    //
    hci_rtk_find_patch(rom_version + 1, path_info, hci_process_info);
    return HCI_TP_CHECK_OK;
}
//===========================set_baudrate============================
static void hci_baudrate_time_out(void *xtimer)
{
    hci_tp_info("UART DEINITchange host baud to %u\n", hci_process_info->baudrate);
    hci_process_info->baud_changing = false;

    rtk_hci_uart_set_baudrate(hci_process_info->baudrate);
    hci_ps_start_next_process();
    //h5_next_init_step(&btrtl);
}

uint8_t hci_pc_set_controller_baudrate(void)
{
    bool ret = false;
    /*TODO get the config baudrate th rkt_baud */
    uint8_t rtk_baud[4] = {0x02, 0x80, 0x92, 0x04};
    uint32_t rtk_baudrate_value = 0;
    rtlbt_ps_util_speed_to_baudc(&rtk_baudrate_value, hci_process_info->baudrate);
    hci_tp_info("rtk_baudrate_value = 0x%x", rtk_baudrate_value);

    rtk_baud[0] = (uint8_t)(rtk_baudrate_value & 0xff);
    rtk_baud[1] = (uint8_t)((rtk_baudrate_value >> 8) & 0xff);
    rtk_baud[2] = (uint8_t)((rtk_baudrate_value >> 16) & 0xff);
    rtk_baud[3] = (uint8_t)((rtk_baudrate_value >> 24) & 0xff);
    //create the timer
    ret = tp_osif_timer_create(&(hci_process_info->baudrate_timer_handle), "h5baud", 0, 500, 0,
                               hci_baudrate_time_out);
    if (!ret)
    {
        hci_tp_err("create baud timer error\n");
        return HCI_TP_CONFIG_FAIL;
    }
    ret = hci_tp_send_hci_cmd(HCI_VSC_UPDATE_BAUDRATE, rtk_baud, 4);
    if (ret == true)
    {
        /*TODO:  send_hci_index++ */
        return HCI_TP_CHECK_OK;
    }
    hci_tp_err("ret = false\n");
    return HCI_TP_CONFIG_FAIL;
}

uint8_t hci_pc_set_baudrate_check(uint8_t len, uint8_t *p_buf)
{
    (void)len;
    (void)p_buf;
    hci_tp_info("hci_tp_set_controller_baudrate: baudrate %d\n", hci_process_info->baudrate);
    //start the timer
    hci_process_info->baud_changing = true;

    tp_osif_timer_restart(&hci_process_info->baudrate_timer_handle, 20);
    if (hci_process_info->proto == RTLBT_UART_H5)
    {
        extern void baudrate_send_the_ack(void);
        baudrate_send_the_ack();
    }
    else
    {
        //h4 no need send the ack
    }
    return HCI_TP_CHECK_WAIT_TIMEOUT;
}
//===========================download_patch============================
void hci_ps_hci_num_plus(void)
{
    if (internal_patch_info != NULL)
    {
        internal_patch_info->pre_hci_send_num++;
    }
    else
    {
        //download patch complete no need
    }
}

static void comput_buf_len(patch_frag_info  *patch_info, uint8_t *packet_buf, uint8_t *packet_len)
{
    uint16_t packet_has_send_data_len = 0;
    packet_has_send_data_len = patch_info->curr_index * PATCH_FRAGMENT_MAX_SIZE;
    if (patch_info->curr_index < patch_info->end_index)
    {
        *packet_len = PATCH_FRAGMENT_MAX_SIZE;
    }
    else if (patch_info->curr_index == patch_info->end_index)
    {
        *packet_len = patch_info->last_pkt;
    }
    else
    {
        //addit packet
        *packet_len = 0;
    }
    //compute the buffer

    if (packet_has_send_data_len >= patch_info->fw_len)
    {
        //need copy the config domain
        patch_info->pos = patch_info->config_buf + packet_has_send_data_len - patch_info->fw_len;
        memcpy(packet_buf, patch_info->pos, *packet_len);
    }
    else if (packet_has_send_data_len + *packet_len <= patch_info->fw_len)
    {
        //patch domain
        patch_info->pos = patch_info->fw_buf + packet_has_send_data_len;
        memcpy(packet_buf, patch_info->pos, *packet_len);
    }
    else if (packet_has_send_data_len <= (patch_info->fw_len + patch_info->config_len))
    {
        //
        patch_info->pos = patch_info->fw_buf + packet_has_send_data_len;
        memcpy(packet_buf, patch_info->pos, patch_info->fw_len - packet_has_send_data_len);
        patch_info->pos = patch_info->config_buf;
        memcpy(packet_buf + (patch_info->fw_len - packet_has_send_data_len), patch_info->pos,
               *packet_len - (patch_info->fw_len - packet_has_send_data_len));
    }
    else
    {
        //the addit packet
        hci_tp_info("the addit packet:total packet :%d,end:%d add_packet:%d", patch_info->total_index,
                    patch_info->end_index, patch_info->addi_pkt);
    }
    if (CHECK_SW(HCI_TP_DEBUG_DOWNLOAD_PATCH))
    {
        tp_osif_printf("curr_index:%d, current fram_size:%d", patch_info->curr_index, *packet_len);
    }

}

static uint8_t comput_index(uint8_t curr_indx, uint8_t endindx)
{
    //hci_tp_info("comput_before:curr_index:%x, total_index:%x", curr_indx,endindx);
    if (curr_indx < 0x80)
    {
        if (curr_indx == endindx)
        {
            curr_indx |= 0x80;
        }
        else
        {
            //normal
        }
    }
    else
    {
        if (curr_indx == endindx)
        {
            curr_indx += (curr_indx & 0x80) >> 7;
        }
        else
        {
            curr_indx = (curr_indx & 0x7f) + 1;
        }
    }
    //hci_tp_info("comput_after:curr_index:%x, total_index:%x", curr_indx,endindx);
    return curr_indx;
}

static void calc_pkt_num(patch_frag_info *path_info)
{
    uint16_t size = path_info->fw_len + path_info->config_len;

    path_info->end_index = (uint8_t)((size - 1) / PATCH_FRAGMENT_MAX_SIZE);
    path_info->last_pkt = (size) % PATCH_FRAGMENT_MAX_SIZE;

    path_info->addi_pkt = ((path_info->end_index + 1 + path_info->pre_hci_send_num) % 8) ? \
                          (8 - (path_info->end_index + 1 + path_info->pre_hci_send_num) % 8) : 0;
    path_info->total_index = path_info->addi_pkt + path_info->end_index;

    hci_tp_info("end_index %d, last_pkt is %d byte, addi_pkt %d, total_index:%d, has send hci:%x\n",
                path_info->end_index, path_info->last_pkt, path_info->addi_pkt, path_info->total_index,
                path_info->pre_hci_send_num);

    if (path_info->last_pkt == 0)
    {
        path_info->last_pkt = PATCH_FRAGMENT_MAX_SIZE;
    }

    path_info->curr_index = 0;
    path_info->data_sent = 0;
    path_info->pos = path_info->fw_buf;
}
uint8_t hci_pc_download_patch(void)
{
    bool ret = false;
    uint8_t real_idx = 0;
    patch_frag_info *path_info = internal_patch_info;
    uint8_t tbuf[PATCH_FRAGMENT_MAX_SIZE];
    uint8_t packet_len = 0;

    if ((path_info->fw_len == 0) && (path_info->config_len == 0))
    {
        tp_osif_printf("==============JUMP THE DOWNLOAD PATCH===================");
        return HCI_TP_NOT_SEND;
    }

    if (path_info->curr_index == 0)
    {
        //first time enter the packet
        //check the patch and copy the patch;
        //load the patch

        calc_pkt_num(internal_patch_info);
    }
    real_idx = comput_index(path_info->curr_index, path_info->total_index);
    comput_buf_len(path_info, tbuf, &packet_len);

    ret =  hci_tp_send_hci_cmd_with_byte(HCI_VSC_DOWNLOAD_PATCH, real_idx, tbuf, packet_len);
    if (ret == true)
    {
        path_info->curr_index++;
        return HCI_TP_CHECK_OK;
    }
    hci_tp_err("%s:[%d] \n", __FUNCTION__, __LINE__);
    return HCI_TP_CONFIG_FAIL;
}

uint8_t hci_pc_download_patch_check(uint8_t len, uint8_t *p_buf)
{
    (void)len;
    uint8_t index;
    index = p_buf[0];
    if (index & 0x80)   /* receive the last fragment completed evt */
    {
        tp_osif_printf("Download patch completely\n");
        //free patch
        if (internal_patch_info->fw_buf != hci_process_info->fw_buf)
        {
            //this is malloc patch
            hci_tp_info("free patch completely\n");
            tp_osif_free(internal_patch_info->fw_buf);
        }
        return HCI_TP_CHECK_OK;
    }
    else
    {
        return HCI_TP_CHECK_AGAIN;
    }
}

uint8_t hci_ps_set_hci_reset(void)
{
    bool ret = false;
    ret = hci_tp_send_hci_cmd(HCI_HCI_RESET, NULL, 0);
    if (ret == true)
    {
        return HCI_TP_CHECK_OK;
    }
    hci_tp_err("ret = false\n");
    return HCI_TP_CONFIG_FAIL;
}
