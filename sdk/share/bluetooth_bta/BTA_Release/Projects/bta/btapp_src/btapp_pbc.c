/****************************************************************************
**                                                                           
**  Name:          btapp_pbc.c                                           
**                 
**  Description:   Contains btui functions for phone book access client
**                 
**                                                                           
**  Copyright (c) 2003-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/

#include "bt_target.h"

#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)

//#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "bta_api.h"
#include "bta_pbc_api.h"
#include "bta_fs_co.h"
//#include "btui.h"
#include "btapp_int.h"
#include "btapp_pbc.h"
#include "bta_pbs_int.h"
#include "utfc.h"
#include "gki.h"
#include "btapp_utility.h"


/* File Listing temporary object store file descriptor */
#define BTA_PBC_FD_IS_UNUSED    -30000
#define BTA_PBC_TEMP_LIST_FILE  "temp38524056.dat"

#ifndef UI_PBC_ID
#define UI_PBC_ID    10
#endif

#define BTUI_PBC_DEFAULT_LOCAL_SUP_FEAS 0x000003FF

/* BTUI PBC client main control block */
tBTUI_PBC_CB btui_pbc_cb;

/* Peer supported features */
tBTA_PBC_SUP_FEA_MASK       peer_features;

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
void btui_pbc_cback(tBTA_PBC_EVT event, tBTA_PBC *p_data);

void btapp_pbc_maintain_workdir(char *dir, BOOLEAN back_up);
static BOOLEAN pbc_xml_vlist_parse (tBTA_PBC_LIST *p_list) ;


/*******************************************************************************
**
** Function         btui_pbc_platform_get_root
**
** Description      Returns a pointer to the root path.
**                  
**
** Returns          void
*******************************************************************************/
void btui_pbc_platform_get_root(char *p_path)
{
    //sprintf (p_path, "%s\\test_files\\pbc",btui_cfg.root_path);
    //sprintf (p_path, "\\test_files\\pbc");
    sprintf (p_path, "A:");
}


/*******************************************************************************
**
** Function         btapp_pbc_init
**
** Description      Initializes PBC Client application
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_pbc_init()
{
    memset(&btui_pbc_cb, 0, sizeof(tBTUI_PBC_CB));

    BTA_PbcEnable(btui_pbc_cback, 10, BTUI_PBC_DEFAULT_LOCAL_SUP_FEAS);
}


/*******************************************************************************
**
** Function         btapp_pbc_close
**
** Description      Closes PBC client connection
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_pbc_close(void)
{
    BTA_PbcClose();
}



/*******************************************************************************
**
** Function         btapp_pbc_connect
**
** Description      Creates connection to the selected device
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_pbc_connect(int argc, const char **argv)
{
    BD_ADDR* connect_bda = NULL;

    if( argc!=2)
    {
        printf("please input bdaddr\r\n");
        return;
    }

    connect_bda = btapp_utl_str_to_bda(argv[1]);
    uint8_t * pp = *connect_bda;

    btui_pbc_cb.auth_already_failed_once = FALSE;   /* Reset authentication flag */
    printf("btapp_pbc_connect :%02x-%02x-%02x-%02x-%02x-%02x\r\n",  pp[0],\
                                                                    pp[1],\
                                                                    pp[2],\
                                                                    pp[3],\
                                                                    pp[4],\
                                                                    pp[5]);
    BTA_PbcOpen(*connect_bda, 0);
}

/*******************************************************************************
**
** Function         btapp_pbc_chdir
**
** Description      Changes peer directory
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_pbc_chdir(char *chdir, BOOLEAN back_up)
{

    tBTA_PBC_FLAG flag = (!back_up) ? BTA_PBC_FLAG_NONE : BTA_PBC_FLAG_BACKUP;
    APPL_TRACE_EVENT1("btapp_pbc_chdir: path= %s", chdir);
    BTA_PbcChDir(chdir, flag);
    btapp_pbc_maintain_workdir(chdir, back_up);
}



 
/*******************************************************************************
**
** Function         btapp_pbc_maintain_workdir
**
** Description      maintain working dir in pbc side
**                  
**
** Returns          void
*******************************************************************************/   
void btapp_pbc_maintain_workdir(char *dir, BOOLEAN back_up)
{
    char * p_fullpath = btui_pbc_cb.p_fullpath;   /*full path*/
    char * p_workdir = btui_pbc_cb.p_workdir;   /* last segment */
    char * p_path;

    if ( !dir || dir[0] == '\0')
    {
        /* Set path to root which is '\0' */
        *p_fullpath = '\0';
        *p_workdir= '\0';
        return;
    }

    /* if not backup */
    if (!back_up)
    {
        /* if change into NULL folder, set to root*/
        if ( *dir == '\0')
        {
                *p_fullpath = '\0';
                *p_workdir= '\0';
        }
        /* make sure working directory is not too long */
        if ((strlen(p_fullpath) + strlen(dir) + 2) <= BTUI_MAX_PATH_LENGTH)
        {   
            sprintf(p_fullpath, "%s\\%s", btui_pbc_cb.p_fullpath, dir);
            strcpy(p_workdir, dir);
        }
        
    }
    else /* backup level unless already at root */
    {   
        if (strlen(p_fullpath) != 0)    /* not root */
        {
            /* Find the last occurrence of separator and replace with '\0' */
            if ((p_path = strrchr(p_fullpath, (int)'\\')) != NULL)
            {
                *p_path = '\0';
                /* find the last segment of the full path */
                if ((p_path = strrchr(p_fullpath, (int)'\\')) != NULL)
                    strcpy(p_workdir, (p_path+1));
                else 
                    *p_workdir = '\0';
            }
        }

    }

    APPL_TRACE_EVENT2("pbc_maintain_workdir: fullpath [%s] wprkdir [%s]", p_fullpath,p_workdir );
}


/*******************************************************************************
**
** Function         btapp_pbc_get_phonebook
**
** Description      gets phonebook from peer device
**                  
**
** Returns          void
*******************************************************************************/   
BOOLEAN  btapp_pbc_get_phonebook(char *file_name, 
                         UINT16 max_list_count, UINT16 list_start_offset, BOOLEAN is_reset_miss_calls)
{

    UINT16   len = strlen(file_name) + 1;
    char    *p_path;
    BOOLEAN  ret = FALSE;
    char *p = file_name;
    char *p_name = NULL;

    do
    {
        /* make sure the correct service is connected */
        if (btui_pbc_cb.connected_service != BTA_PBAP_SERVICE_ID)
            break;

        /* double check the specified phonebook is correct */
        if(strncmp(p, "SIM1/", 5) == 0)
            p += 5;
        if(strncmp(p, "telecom/", 8) != 0)
            break;
        p += 8;
        p_name = p;
        if(strcmp(p, "pb.vcf"))
        {
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
            if (strcmp(p, "fav.vcf") && strcmp(p, "spd.vcf"))
#endif
            {

                p++;
                if(strcmp(p, "ch.vcf") != 0)
                    break;
            }
        }

        if ((p_path = (char *) GKI_getbuf(BTUI_MAX_PATH_LENGTH + 1)) == NULL)
        {
            break;
        }

        ret = TRUE;
        btui_pbc_platform_get_root(p_path);
        sprintf(p_path, "%s/%s", p_path, p_name);
        APPL_TRACE_DEBUG1("GET PhoneBook p_path = %s", p_path);
        APPL_TRACE_DEBUG2("file name = %s, len = %d", file_name, len);

        btui_pbc_cb.bytes_transferred = 0;
        BTA_PbcGetPhoneBook(p_path, file_name, BTA_PBC_FILTER_FN| BTA_PBC_FILTER_TEL, BTA_PBC_FORMAT_CARD_30,
                             max_list_count, list_start_offset, is_reset_miss_calls, BTA_PBC_FILTER_ALL,
                             0);

        GKI_freebuf(p_path);
    } while(0);

    return ret;
}

/*******************************************************************************
**
** Function         btapp_pbc_get_vcard
**
** Description      gets vcard from peer device
**                  
**
** Returns          void
*******************************************************************************/   
BOOLEAN  btapp_pbc_get_vcard(char *file_name)
{

    UINT16   len = strlen(file_name) + 1;
    char    *p_path;

    if (btui_pbc_cb.connected_service != BTA_PBAP_SERVICE_ID ||
        (p_path = (char *) GKI_getbuf(BTUI_MAX_PATH_LENGTH + 1)) == NULL)
    {

        return FALSE;
    }

    btui_pbc_platform_get_root(p_path);
    sprintf(p_path, "%s\\%s", p_path, file_name);
    APPL_TRACE_DEBUG3("GET vCard Entry p_path = %s,  file name = %s, len = %d",
                        p_path, file_name, len);

    btui_pbc_cb.bytes_transferred = 0;
    //BTA_PbcGetCard(p_path, file_name, btui_cfg.pbc_filter_mask, btui_cfg.pbc_format);

    BTA_PbcGetCard(p_path, file_name, BTA_PBC_FILTER_ALL, BTA_PBC_FORMAT_CARD_21);

    GKI_freebuf(p_path);

    return TRUE;

}

/*******************************************************************************
**
** Function         btapp_pbc_list_vards
**
** Description      refreshes peer vcard listing
**                  
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_pbc_list_vards(char *p_dir, char *p_value,
                          UINT16 max_list_count, UINT16 list_start_offset,
                          BOOLEAN is_reset_miss_calls)
{
    if (btui_pbc_cb.connected_service != BTA_PBAP_SERVICE_ID)
        return FALSE;
    APPL_TRACE_EVENT0("btapp_pbc_list_vards");
    btui_pbc_cb.list_refresh = TRUE;
    BTA_PbcListCards(p_dir, BTA_PBC_ORDER_INDEXED, p_value,
                      BTA_PBC_ATTR_NAME,  max_list_count,
                       list_start_offset, is_reset_miss_calls,
                       BTA_PBC_FILTER_ALL, 0);
    return TRUE;
}



/*******************************************************************************
**
** Function         btui_pbc_cback
**
** Description      PBC UI Callback function.  Handles all PBC events for the UI
**                  
**
** Returns          void
**
*******************************************************************************/
void btui_pbc_cback(tBTA_PBC_EVT event, tBTA_PBC *p_data)
{
    char buf[80];
    //tBTUI_BTA_MSG * p_event_msg;
    BOOLEAN parse_status = FALSE;

    APPL_TRACE_EVENT1("btui_pbc_cback event:%d", event);
    switch(event)
    {
    case BTA_PBC_ENABLE_EVT:
        APPL_TRACE_EVENT0("Phone Book Access Client Enabled...");
        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_ENABLE;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}
        break;

    case BTA_PBC_OPEN_EVT:
        if (p_data->open.service == BTA_PBAP_SERVICE_ID)
        {
            btui_pbc_cb.pb_level = BTUI_PBC_LVL_TOP;
            sprintf(buf, "PBC CONNECTION OPEN...[%s]", "PBAP");
        }

        btui_pbc_cb.connected_service = p_data->open.service;

        APPL_TRACE_EVENT0(buf);

        /* Allocate memory to hold the directory listing */
        btui_pbc_cb.p_flist = (tBTUI_PBC_FOLDER_LIST *)GKI_getbuf(GKI_MAX_BUF_SIZE);
        btui_pbc_cb.num_menu_entries = 0;

        /* initialize xml parser cb*/
        btui_pbc_cb.xml_parser.max_peer_items = (UINT16)(GKI_MAX_BUF_SIZE / sizeof(tBTUI_PBC_FOLDER_LIST));
        btui_pbc_cb.xml_parser.new_parse = TRUE;
        if (p_data->open.service == BTA_PBAP_SERVICE_ID)
            memset(&btui_pbc_cb.xml_parser.p_xml_parser.xml_user_data, 0, 
                    sizeof(tXML_VLIST_STATE));

        btui_pbc_cb.first_listing = TRUE;
        btui_pbc_cb.list_refresh = FALSE;
        btui_pbc_cb.folder_op = BTUI_PBC_NONE;
        
        /* Allocate a buffer to hold the current working directory in server */
        btui_pbc_cb.p_fullpath = (char *)GKI_getbuf(BTUI_MAX_PATH_LENGTH + 1);
        *btui_pbc_cb.p_fullpath = '\0';
        btui_pbc_cb.p_workdir = (char *)GKI_getbuf(BTUI_MAX_PATH_LENGTH + 1);
        *btui_pbc_cb.p_workdir = '\0';

        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_OPEN;
        //    p_event_msg->open.service = p_data->open.service;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}

        /* auto sync phonebook */
        btapp_pbc_get_phonebook(BTA_PBS_PULLPB_NAME, 65535, 0, TRUE);
        break;
 
    
    case BTA_PBC_CLOSE_EVT:
        /* free the memory asociated with the folder listing */
        if (btui_pbc_cb.p_flist)
        {
            GKI_freebuf(btui_pbc_cb.p_flist);
            btui_pbc_cb.p_flist = NULL;
        }
        btui_pbc_cb.xml_parser.max_peer_items = 0;

        /* clean up left over data (if any) in xml parser */
        if (btui_pbc_cb.connected_service == BTA_PBAP_SERVICE_ID)
            XML_VlistParse( &btui_pbc_cb.xml_parser.p_xml_parser,
                         NULL,0,NULL,NULL,&btui_pbc_cb.xml_parser.num_list_items);

        /* release previously allocated GKI buffers for folder entry data*/
        for (;btui_pbc_cb.xml_parser.num_gki_blocks >0 ;)
             GKI_freebuf(btui_pbc_cb.xml_parser.p_gkiblk[--btui_pbc_cb.xml_parser.num_gki_blocks]);

        if (btui_pbc_cb.p_fullpath)
        {
            GKI_freebuf(btui_pbc_cb.p_fullpath);
            btui_pbc_cb.p_fullpath = NULL;
        }

        /* free the memory asociated with the peer working directory */
        if (btui_pbc_cb.p_workdir)
        {
            GKI_freebuf(btui_pbc_cb.p_workdir);
            btui_pbc_cb.p_workdir = NULL;
        }
        APPL_TRACE_EVENT0("Phone Book Access Client CONNECTION CLOSED...");
        btui_pbc_cb.connected_service = 0;
        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_CLOSE;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}
        break;

    case BTA_PBC_AUTH_EVT:
        if (!btui_pbc_cb.auth_already_failed_once)
        {
            btui_pbc_cb.auth_already_failed_once = TRUE;
            //BTA_PbcAuthRsp(btui_cfg.pbc_password, btui_cfg.pbc_userid);
            BTA_PbcAuthRsp("1234", "guest");
        }
        else
        {
            APPL_TRACE_EVENT1("Phone Book Access Client AUTH Failure (pin %s)...",
                              "1234");
            btui_pbc_cb.auth_already_failed_once = FALSE;
            BTA_PbcClose(); /* Close the connection because pin code didn't match */
        }
        break;

    case BTA_PBC_LIST_EVT:
        if(p_data->list.status == BTA_PBC_OK)
        {
            if (btui_pbc_cb.folder_op != BTUI_PBC_BACKOFF)
            {
                if (btui_pbc_cb.connected_service == BTA_PBAP_SERVICE_ID)
                    parse_status = pbc_xml_vlist_parse(&p_data->list);
            }
        }
        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_LIST;
        //    p_event_msg->pbc_list.final = p_data->list.final;
        //    p_event_msg->pbc_list.status = p_data->list.status;
        //    p_event_msg->pbc_list.parse_status = parse_status;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}
        break;

    case BTA_PBC_PROGRESS_EVT:
        btui_pbc_cb.bytes_transferred += p_data->prog.bytes;
        if (p_data->prog.file_size != BTA_FS_LEN_UNKNOWN)
        {
            APPL_TRACE_EVENT2("Phone Book Access Client PROGRESS (%d of %d total)...",
                              btui_pbc_cb.bytes_transferred, p_data->prog.file_size);
        }
        else
        {
            APPL_TRACE_EVENT1("Phone Book Access Client PROGRESS (%d bytes total)...",
                              btui_pbc_cb.bytes_transferred);
        }
        break;

    case BTA_PBC_CHDIR_EVT:
        if (btui_pbc_cb.folder_op != BTUI_PBC_BACKOFF)
            btui_pbc_cb.folder_op = BTUI_PBC_CHDIR;
        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_CHDIR;
        //    p_event_msg->pbc_status.status = p_data->status;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}
        break;

    case BTA_PBC_PHONEBOOK_EVT:
        APPL_TRACE_EVENT1("Phone Book size %d", p_data->pb.phone_book_size);
        btui_pbc_cb.bytes_transferred = p_data->pb.phone_book_size;
        break;
   
    case BTA_PBC_GETFILE_EVT:
        if(p_data->status == BTA_PBC_OK)
        {
            if (btui_pbc_cb.bytes_transferred < 1024)
                sprintf(buf, "Transfer Complete: status %d, (%d total)",
                        p_data->status, btui_pbc_cb.bytes_transferred);
            else
                sprintf(buf, "Transfer Complete: status %d, (%dk total)",
                        p_data->status, btui_pbc_cb.bytes_transferred / 1024);
        }
        else
        {
            if(p_data->status == BTA_PBC_NO_PERMISSION)
                sprintf(buf, "Transfer Failed No permission ");
            else if(p_data->status == BTA_PBC_NOT_FOUND)
                sprintf(buf, "Transfer Failed, File Not Found");
            else if(p_data->status == BTA_PBC_FULL)
                sprintf(buf, "Transfer Failed, Full or Too Big");
            else if(p_data->status == BTA_PBC_ABORTED)
                sprintf(buf, "Transfer Aborted");
            else if(p_data->status == BTA_PBC_PRECONDITION_FAILED)
                sprintf(buf, "Transfer Failed, Precondition failed");
            else
                sprintf(buf, "Transfer Failed");

        }
        btapp_pbc_close();
        APPL_TRACE_EVENT0(buf);
        //if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    
        //    p_event_msg->hdr.event = BTUI_MMI_PBC_GET;
        //    p_event_msg->pbc_status.status = p_data->status;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        //}
        break;
    }
}



/*******************************************************************************
**
** Function         btapp_pbc_abort
**
** Description      Abort the file transfer operation.
**                  
** Returns          void
*******************************************************************************/  
void btapp_pbc_abort(void)
{
    /* Abort the current file transfer operation */
    BTA_PbcAbort();
}


/*******************************************************************************
**
** Function     pbc_xml_vlist_parse
**
** Description  This function is called to parse the current listing data received.
**              If final flag is set, display all the vlist entry items received.
**
** Returns      TRUE if done, otherwise FALSE
**
*******************************************************************************/


static BOOLEAN pbc_xml_vlist_parse (tBTA_PBC_LIST *p_list) 
{
    BOOLEAN done = FALSE, x_error= FALSE;
    tBTUI_PBC_LIST_PARSER *parser_cb = &btui_pbc_cb.xml_parser;
    tXML_VLIST_RES x_result = XML_VLIST_NO_RES;
    UINT16  size = sizeof(tXML_VLIST_ENTRY)*PBC_MAX_VLIST_ITEMS;
    UINT8 * p_dst_data = NULL;

    if (parser_cb->new_parse)
    {           
            parser_cb->num_list_items = 0;  /* reset */
            parser_cb->new_parse = FALSE;
            memset( (void *) parser_cb->flist, 0, sizeof( parser_cb->flist) ); /* this is not needed by parser */

            /* release previously allocated GKI buffers */
            for (;parser_cb->num_gki_blocks >0 ; )
                GKI_freebuf(parser_cb->p_gkiblk[--parser_cb->num_gki_blocks]);

            XML_VlistInit( &parser_cb->p_xml_parser,
                             parser_cb->flist,
                             PBC_MAX_VLIST_ITEMS  );
            APPL_TRACE_EVENT0("pbc_xml_vlist_parse: XML_VlistInit\r\n");
    }
    
    if (!done) 
    {
        do 
        {
            size = 32 * BTUI_SCREEN_LENGTH;

            if (PBC_CLNT_MAX_GKI_BLOCKS > parser_cb->num_gki_blocks &&
                (p_dst_data = (UINT8 *) GKI_getbuf(size))!= NULL)
            {
                memset((void*)p_dst_data, 0, size);

                /* enqueue memory block heads */
                parser_cb->p_gkiblk[parser_cb->num_gki_blocks] = p_dst_data;
                parser_cb->num_gki_blocks ++;

                APPL_TRACE_DEBUG2("pbc_xml_vlist_parse: allocate buffer No.[%d], entry %d\r\n",parser_cb->num_gki_blocks, parser_cb->num_list_items );

                x_result = XML_VlistParse( &parser_cb->p_xml_parser,
                                        p_list->data,p_list->len,
                                        p_dst_data, &size,
                                        &parser_cb->num_list_items );
            }
            else
            {
                XML_VlistParse( &parser_cb->p_xml_parser,
                              NULL,0,NULL,NULL,&parser_cb->num_list_items);

                APPL_TRACE_DEBUG0("pbc_xml_vlist_parse: Out of Resources,discard partial data. ");
                break;
            }
        } 
        while (x_result == XML_VLIST_DST_NO_RES);

        if (x_result == XML_VLIST_OK || x_result == XML_VLIST_NO_RES )
                x_error = TRUE;

        APPL_TRACE_EVENT5("pbc_xml_vlist_parse: XML_VlistParse ( final:%d, status:%d, len:%d entries %d): %d\r\n",
                           p_list->final,
                           p_list->status,
                           p_list->len,
                           parser_cb->num_list_items,
                           x_result );
    } 
        
    /* if last package of list data and no error occurs   */
    /* display parsed folder entry                          */
    if (p_list->final && !x_error)
            done = TRUE;

    return done;
}


#endif

