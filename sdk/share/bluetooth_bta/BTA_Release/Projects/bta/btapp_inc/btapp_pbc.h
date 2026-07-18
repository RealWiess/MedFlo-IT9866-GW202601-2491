/*****************************************************************************
**                                                                            
**  Name:             btapp_pbc.h                                              
**                                                                            
**  Description:     This file contains btapp interface               
**				     definition                                         
**                                                                            
**  Copyright (c) 2003-2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.                     
******************************************************************************/

#ifndef BTAPP_PBC_H
#define BTAPP_PBC_H


#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)

#include "xml_vlist_api.h" 
#include "xml_flp_api.h"

#ifndef BTUI_SCREEN_LENGTH
#define BTUI_SCREEN_LENGTH           16
#endif

#ifndef BTUI_MAX_PATH_LENGTH
#define BTUI_MAX_PATH_LENGTH         512
#endif

#define PBC_VCARD_LISTING_SIZE 8192
#define PBC_MAX_VLIST_ITEMS (PBC_VCARD_LISTING_SIZE / sizeof(tXML_VLIST_ENTRY))

#define PBC_CLNT_MAX_GKI_BLOCKS      20

typedef struct
{    
    tXML_VLIST_PARSER p_xml_parser; 
    UINT8   carry_buf[XML_FOLDER_CARRY_OVER_LEN];
    UINT16  num_list_items; /* Current index into list array */
    UINT16  max_peer_items;
    BOOLEAN new_parse;
    tXML_VLIST_ENTRY flist[PBC_MAX_VLIST_ITEMS];

    /* GKI buffer for storage of name of */
    /* each file/folder entry which is flist[].data*/
    UINT8   *p_gkiblk[PBC_CLNT_MAX_GKI_BLOCKS];                          
    UINT8   num_gki_blocks;

} tBTUI_PBC_LIST_PARSER;

typedef struct
{
    char*   data; /* Room for '\0' */
    char    screenname[BTUI_SCREEN_LENGTH + 3]; /* Room for '<', '>', and '\0' */
} tBTUI_PBC_FOLDER_LIST;
#endif

enum
{
    BTUI_PBC_NONE,
    BTUI_PBC_CHDIR,
    BTUI_PBC_BACKOFF
};
typedef UINT8 tBTUI_PBC_OP;

typedef struct
{
    BOOLEAN         pb;
    BOOLEAN         ich;
    BOOLEAN         och;
    BOOLEAN         mch;
    BOOLEAN         cch;
} tBTUI_PBC_PB;

enum
{
    BTUI_PBC_LVL_TOP = 0,
    BTUI_PBC_LVL_ROOT,
    BTUI_PBC_LVL_PARENT,
    BTUI_PBC_LVL_TEL,
    BTUI_PBC_LVL_SIM1,
    BTUI_PBC_LVL_SIM1_TEL,
    BTUI_PBC_LVL_TEL_PB,
    BTUI_PBC_LVL_TEL_ICH,
    BTUI_PBC_LVL_TEL_OCH,
    BTUI_PBC_LVL_TEL_MCH,
    BTUI_PBC_LVL_TEL_CCH,
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
    BTUI_PBC_LVL_TEL_SPD,
    BTUI_PBC_LVL_TEL_FAV,
#endif
    BTUI_PBC_LVL_SIM1_TEL_PB,
    BTUI_PBC_LVL_SIM1_TEL_ICH,
    BTUI_PBC_LVL_SIM1_TEL_OCH,
    BTUI_PBC_LVL_SIM1_TEL_MCH,
    BTUI_PBC_LVL_SIM1_TEL_CCH
};

#define BTUI_PBC_LVL_FIRST_LEAVE    BTUI_PBC_LVL_TEL_PB
#define BTUI_PBC_LVL_LAST           BTUI_PBC_LVL_SIM1_TEL_CCH

typedef struct
{
    char    *name;
    UINT8   parent;
} tBTUI_PBC_PB_DIR;

typedef struct
{
#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)
     tBTUI_PBC_LIST_PARSER    xml_parser;            /* xml parser */
     tBTUI_PBC_FOLDER_LIST      *p_flist; /* Points to the current working folder list items */
     tBTUI_PBC_FOLDER_LIST    * selected_item;
#endif
    //tHT_MENU_ITEM              *p_peer_menu;     /* Folder Listing of peer device */
     char                       *p_fullpath;
     char                       *p_workdir;
     UINT32                        peer_menu_handle;
     UINT32                        bytes_transferred;
     UINT8                         num_files;
     BOOLEAN                      is_initialized;            
     BOOLEAN                      first_listing;
     BOOLEAN                      list_refresh;
     BOOLEAN                      inside_pbc;
     BOOLEAN                      auth_already_failed_once;
     tBTUI_PBC_OP                 folder_op;
     tBTA_SERVICE_ID            connected_service;
     UINT32                        main_pbc_menu_handle;
     UINT32                        progress_msg_handle;
     UINT16                         num_menu_entries;
     UINT16                        num_chdir_to_root;
     BOOLEAN                      sim1; /* SIM 1 exists */
     UINT8                        pb_level;     /* current phonebook level */
     UINT8                        sel_level;    /* menu select level */
     UINT16                       list_start_offset;
     BOOLEAN                      log_curr_folder_listing;
} tBTUI_PBC_CB;

extern tBTUI_PBC_CB btui_pbc_cb;

extern void btapp_pbc_init();
extern void btapp_pbc_close(void);
//extern void btapp_pbc_connect();
extern void btapp_pbc_connect(int argc, const char **argv);
extern void btapp_pbc_chdir(char *chdir, BOOLEAN back_up);
extern void btapp_pbc_maintain_workdir(char *dir, BOOLEAN back_up);
extern BOOLEAN  btapp_pbc_get_phonebook(char *file_name, 
                         UINT16 max_list_count, UINT16 list_start_offset, BOOLEAN is_reset_miss_calls);
extern BOOLEAN  btapp_pbc_get_vcard(char *file_name);
extern BOOLEAN btapp_pbc_list_vards(char *p_dir, char *p_value,
                         UINT16 max_list_count, UINT16 list_start_offset, BOOLEAN is_reset_miss_calls);

extern void btapp_pbc_refresh_peer_folder();
extern void btapp_pbc_abort(void);

#endif
