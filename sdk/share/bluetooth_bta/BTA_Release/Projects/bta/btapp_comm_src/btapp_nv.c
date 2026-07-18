/****************************************************************************
**
**  Name:          btapp_nv.c
**
**  Description:   Contains btapp nvram abstraction source file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "ite/itp.h"
#include <sys/ioctl.h>
#include <stdlib.h>
#include "btapp_nv.h"


//#define BT_FAT_FILE "B:/btDB"
#define BT_FAT_FILE "A:/btDB"
#define BT_IRK_FILE "A:/btIRK"
#define BT_HS_DB_FILE "A:/btHS"

enum{
    CONF_DB_READ  = 0,
    CONF_DB_WRITE = 1,
    IRK_READ      = 2,
    IRK_WRITE     = 3,
    HS_DB_READ    = 4,
    HS_DB_WRITE   = 5,

    IDLE          =-1,
};

static int gOperation = IDLE;
static int gbFatThread = 0;
static pthread_mutex_t gMutexBtAppNv;
static uint8_t* gpBuffer = NULL;
static int    gBufferSize = 0;
static uint8_t* temp_conf_db = NULL;
static uint8_t* temp_hs_db = NULL;
static uint8_t* temp_irk = NULL;



static void* btFatOperation(void* arg)
{
    gbFatThread = 1;
    FILE* btFile = NULL;
    while(1)
    {
        //Read operation
        if (gOperation == CONF_DB_READ)
        {
            printf("CONF_DB_READ operation\n");

			if (!temp_conf_db)
                temp_conf_db = malloc(gBufferSize);

            if (gpBuffer && gBufferSize)
            {
                btFile = fopen(BT_FAT_FILE, "rb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_FAT_FILE);
                }
                else
                {
                    fread(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);

					if(temp_conf_db)
					{
                    	memcpy(temp_conf_db, gpBuffer, gBufferSize);
					}
					else
					{
						printf("temp_conf_db malloc failed XXXXX\n");
					}
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else if (gOperation == CONF_DB_WRITE)
        {
            printf("CONF_DB_WRITE operation\n");

			if (!temp_conf_db)
		    	temp_conf_db = malloc(gBufferSize);

            if (gpBuffer && gBufferSize)
            {
				if(temp_conf_db)
				{
					if (!memcmp(temp_conf_db, gpBuffer, gBufferSize))
	                {
	                    printf("bt conf_db is same, skip write\n");
	                    gOperation = IDLE;
	                    continue;
	                }

	                printf("bt conf_db need update\n");
	                memcpy(temp_conf_db, gpBuffer, gBufferSize);
				}
				else
				{
					printf("temp_conf_db malloc failed XXXXX\n");
				}
				
                btFile = fopen(BT_FAT_FILE, "wb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_FAT_FILE);
                }
                else
                {
                    fwrite(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
                    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                    #if (CFG_LFS_CACHE_SIZE > 0)
                        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
                    #endif
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else if(gOperation == IRK_READ)
        {
        	printf("IRK_READ operation\n");

			if(!temp_irk)
			{
				temp_irk = malloc(gBufferSize);
			}
			
            if (gpBuffer && gBufferSize)
            {
                btFile = fopen(BT_IRK_FILE, "rb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_IRK_FILE);
                    gpBuffer = NULL,
                    gBufferSize = IDLE;
                }
                else
                {
                    fread(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
					if(temp_irk)
					{
                    	memcpy(temp_irk, gpBuffer, gBufferSize);
					}
					else
					{
						printf("temp_irk malloc failed XXXXX\n");
					}
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else if(gOperation == IRK_WRITE)
        {
        	printf("IRK_WRITE operation\n");

			if(!temp_irk)
			{
				temp_irk = malloc(gBufferSize);
			}
			
            if (gpBuffer && gBufferSize)
            {
            	if(temp_irk)
            	{
            		if (!memcmp(temp_irk, gpBuffer, gBufferSize))
	                {
	                    printf("bt irk is same, skip write\n");
	                    gOperation = IDLE;
	                    continue;
	                }
					printf("bt irk need update\n");
	                memcpy(temp_irk, gpBuffer, gBufferSize);
            	}
				else
				{
					printf("temp_irk malloc failed XXXXX\n");
				}
                btFile = fopen(BT_IRK_FILE, "wb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_IRK_FILE);
					gOperation = IDLE;
                }
                else
                {
                    fwrite(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
                   	ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                    #if (CFG_LFS_CACHE_SIZE > 0)
                        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
                    #endif
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else if(gOperation == HS_DB_READ)
        {
        	printf("HS_DB_READ operation\n");

			if (!temp_hs_db)
			{
                temp_hs_db = malloc(gBufferSize);
			}
			
            if (gpBuffer && gBufferSize)
            {
                btFile = fopen(BT_HS_DB_FILE, "rb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_HS_DB_FILE);
                    gpBuffer = NULL,
                    gBufferSize = IDLE;
                }
                else
                {
                    fread(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);

					if(temp_hs_db)
					{
						memcpy(temp_hs_db, gpBuffer, gBufferSize);
					}
					else
					{
						printf("temp_hs_db malloc failed XXXXX\n");
					}
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else if(gOperation == HS_DB_WRITE)
        {
        	printf("HS_DB_WRITE operation\n");

			if (!temp_hs_db)
			{
		    	temp_hs_db = malloc(gBufferSize);
			}
			
            if (gpBuffer && gBufferSize)
            {
				if(temp_hs_db)
				{
					if (!memcmp(temp_hs_db, gpBuffer, gBufferSize))
	                {
	                    printf("bt hs_db is same, skip write\n");
	                    gOperation = IDLE;
	                    continue;
	                }

	                printf("bt hs_db need update\n");
	                memcpy(temp_hs_db, gpBuffer, gBufferSize);
				}
				else
				{
					printf("temp_hs_db malloc failed XXXXX\n");
				}
				
                btFile = fopen(BT_HS_DB_FILE, "wb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_HS_DB_FILE);
                }
                else
                {
                    fwrite(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
                    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                    #if (CFG_LFS_CACHE_SIZE > 0)
                        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
                    #endif
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = IDLE;
        }
        else
        {
            usleep(100*1000);
        }
    }
	return NULL;
}

void config_create_fat_thread(void)
{
    pthread_t task;
    pthread_attr_t attr;
    // init mutex
    pthread_mutex_init(&gMutexBtAppNv, NULL);
    pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 20 * 1024);
    pthread_create(&task, &attr, btFatOperation, NULL);
    usleep(5000);
}

void config_write_dev_db(uint8_t* buffer, int size)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = buffer;
    gBufferSize = size;
    gOperation = 1;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}

void config_read_dev_db(uint8_t* buffer, int size)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }

    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = buffer;
    gBufferSize = size;
    gOperation = 0;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);

}

void btapp_init_device_db (void)
{
    config_create_fat_thread();

    memset(&btapp_device_db,0x00,sizeof(btapp_device_db));

    /* set visibility to true by default. If visibility setting
    was stored in nvram previously it will get overwritten when
    all the parameters are read from nvram */
#if ((defined BR_INCLUDED) && (BR_INCLUDED == TRUE))
    btapp_device_db.visibility = FALSE;
    btapp_device_db.pairability = FALSE;
#endif

#if BLE_INCLUDED == TRUE
    btapp_device_db.le_conn_mode = BTA_DM_BLE_NON_CONNECTABLE;
    btapp_device_db.le_disc_mode = BTA_DM_BLE_NON_DISCOVERABLE;
#endif
    /* Set the local device name to default name */
    strcpy(btapp_device_db.local_device_name, btapp_cfg.cfg_dev_name);

    /* Phone needs to store in nvram some information about
    itself and other bluetooth devices with which it regularly communicates
    The information that has to be stored typically are
    local bluetooth name, visbility setting bdaddr, name, link_key,
    trust relationship etc.  */

    /* read device data base stored in nv-ram */
    btapp_nv_init_device_db();

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
    btapp_nv_init_hs_db();
#endif

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
    btapp_nv_init_ble_info();
#if (defined BTA_GATT_INCLUDED) && (BTA_GATT_INCLUDED == TRUE)
//    btapp_nv_init_gattc_db();
//    btapp_nv_init_gatts_hndl_range_db();
//    btapp_nv_init_gatts_srv_chg_db();
#endif
#endif

}
/*******************************************************************************
**
** Function         btapp_store_device
**
** Description      stores peer device to NVRAM
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_store_device( tBTAPP_REM_DEVICE * p_rem_device)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (btapp_device_db.device[i].in_use
            && memcmp(&btapp_device_db.device[i].bd_addr, p_rem_device->bd_addr, BD_ADDR_LEN))
            continue;
        memcpy(&btapp_device_db.device[i], p_rem_device, sizeof(btapp_device_db.device[i]));
        btapp_device_db.device[i].in_use = TRUE;

#if BLE_INCLUDED == TRUE
        APPL_TRACE_EVENT1("store ble p_rem_device->device_type = %d", p_rem_device->device_type);
#endif
        break;
    }

    if (i == BTAPP_NUM_REM_DEVICE)
        return FALSE;

    /* update data base in nvram */
    btapp_nv_store_device_db();
    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_get_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_get_device_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use
            || memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btapp_device_db.device[i];

    }

    return NULL;
}
/*******************************************************************************
**
** Function         btapp_get_device_record_idx
**
** Description      gets the device record index of a stored device
**
**
** Returns          void
*******************************************************************************/
UINT8 btapp_get_device_record_idx(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use
            || memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return i;

    }

    return 0xff;
}

/*******************************************************************************
**
** Function         btapp_get_inquiry_record
**
** Description      gets the device record from inquery db
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_get_inquiry_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_INQ_DEVICE; i++)
    {
        if (memcmp(btapp_inq_db.remote_device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btapp_inq_db.remote_device[i];
    }

    return NULL;
}


/*******************************************************************************
**
** Function         btapp_alloc_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_alloc_device_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use)
        {
            /* not in use */
            memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i]));
            btapp_device_db.device[i].in_use = TRUE;
            memcpy(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN);
            return &btapp_device_db.device[i];
        }
    }

    APPL_TRACE_ERROR0("btapp alloc device record failed, all entry has been used!, delete the oldest info");

    btapp_delete_device(btapp_device_db.device[0].bd_addr);
    return btapp_alloc_device_record(bd_addr);
}

/*******************************************************************************
**
** Function         btapp_nv_store_device_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_device_db(void)
{
    /* this stores all nvram parameters */
/*#if (BT_CONFIG_FILE == TRUE)
    UINT32 entry = sizeof(btapp_device_db);
    // this stores all nvram parameters
    config_write_dev_db((unsigned char *)&btapp_device_db, entry);
#endif*/
    config_write_dev_db((unsigned char *)&btapp_device_db, sizeof(btapp_device_db));
	
    int i = 0;
    int j = 0;
    tBTAPP_REM_DEVICE * p;

    //printf("--- stored linkkey ---\r\n");
    for (i = 0; i < BTAPP_NUM_REM_DEVICE; i++)
    {

        p = &btapp_device_db.device[i];

        if (p->in_use)
        {
            printf("--- stored linkkey idx:%02d %s---\r\n",i, p->name);

            printf("[%02x:%02x:%02x:%02x:%02x:%02x]\r\n",p->bd_addr[0],
                                                         p->bd_addr[1],
                                                         p->bd_addr[2],
                                                         p->bd_addr[3],
                                                         p->bd_addr[4],
                                                         p->bd_addr[5]);
			printf("name %s\n",p->name);
            /* BR/EDR */
            if (p->link_key_present)
            {
                printf("linkkey ");
                for (j = 0; j < LINK_KEY_LEN; j++)
                {
                    printf("%02x", p->link_key[j]);
                }
                printf("\r\n");
            }

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
            /* BLE */
            if( p->key_mask & BTA_LE_KEY_PENC)
            {
                printf("ltk:    ");
                for (j = 0; j < LINK_KEY_LEN; j++)
                {
                    printf("%02x", p->penc_key.ltk[j]);
                }
                printf("\r\n");
            }
#endif
        }
    }
}

/*******************************************************************************
**
** Function         btapp_nv_init_device_db
**
** Description      Inits device data base with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_device_db(void)
{
    int i = 0;
    int j = 0;
    tBTAPP_REM_DEVICE * p;

    config_read_dev_db((unsigned char *)&btapp_device_db, sizeof(btapp_device_db));
    //APPL_TRACE_EVENT0("--- stored linkkey ---\r\n");
    for (i = 0; i < BTAPP_NUM_REM_DEVICE; i++)
    {
        p = &btapp_device_db.device[i];

        if (p->in_use)
        {
            printf("--- stored linkkey idx:%02d %s+++\r\n",i, p->name);

            printf("[%02x:%02x:%02x:%02x:%02x:%02x]\r\n",p->bd_addr[0],
                                                             p->bd_addr[1],
                                                             p->bd_addr[2],
                                                             p->bd_addr[3],
                                                             p->bd_addr[4],
                                                             p->bd_addr[5]);
			printf("init name %s\n",p->name);
            /* BR/EDR */
            if (p->link_key_present)
            {
                printf("linkkey ");
                for (j = 0; j < LINK_KEY_LEN; j++)
                {
                    printf("%02x", p->link_key[j]);
                }
                printf("\r\n");
            }

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
            /* BLE */
            if( p->key_mask & BTA_LE_KEY_PENC)
            {
                printf("ltk:    ");
                for (j = 0; j < LINK_KEY_LEN; j++)
                {
                    printf("%02x", p->penc_key.ltk[j]);
                }
                printf("\r\n");
            }
#endif
        }
    }
}

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_hs_db
**
** Description      Inits HS settings with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_hs_db(void)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = (uint8_t*)&btapp_hs_db;
    gBufferSize = sizeof(btapp_hs_db);
    gOperation = HS_DB_READ;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }
    if (gBufferSize <0)
    {
        printf("btapp_nv_store_hs_db open failure \r\n");
        memset(&btapp_hs_db, 0, sizeof(btapp_hs_db));
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}
/*******************************************************************************
**
** Function         btapp_nv_store_hs_db
**
** Description      Stores all parameters into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_hs_db(void)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = (uint8_t*)&btapp_hs_db;//snadra
    gBufferSize = sizeof(btapp_hs_db);
    gOperation = HS_DB_WRITE;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}
#endif

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_ble_info
**
** Description      Inits ble info information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_ble_info(void)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = (uint8_t*)&btapp_ble_local_key;
    gBufferSize = sizeof(btapp_ble_local_key);
    gOperation = IRK_READ;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }

    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}
/*******************************************************************************
**
** Function         btapp_nv_store_ble_local_keys
**
** Description      Stores ble local id keys into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_ble_local_keys(void)
{
    if (!gbFatThread)
    {
        printf("%s bt FAT operation thread is not created\n",__func__);
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = (uint8_t*)&btapp_ble_local_key;
    gBufferSize = sizeof(btapp_ble_local_key);
    gOperation = IRK_WRITE;
    usleep(1000);
    while (gOperation != IDLE)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_gattc_db
**
** Description      Inits GATT client database with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gattc_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_gattc_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gattc_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_init_gatts_hndl_range_db
**
** Description      Inits GATTS handle map with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gatts_hndl_range_db(void)
{

}

/*******************************************************************************
**
** Function         btapp_nv_store_gatts_hndl_range_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gatts_hndl_range_db(void)
{

}

/*******************************************************************************
**
** Function         btapp_nv_init_gatts_srv_chg_db
**
** Description      Initialize GATTS service change db with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gatts_srv_chg_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_gatts_srv_chg_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gatts_srv_chg_db(void)
{

}

#endif
#endif

/*******************************************************************************
**
** Function         btapp_delete_device
**
** Description      deletes the device from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_delete_device(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        //Simply sort
        for ( ;i < (BTAPP_NUM_REM_DEVICE-1); i++)
        {
            if (!btapp_device_db.device[i+1].in_use)
            {
                memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i+1]));
                break;
            }
            memcpy(&btapp_device_db.device[i], &btapp_device_db.device[i+1], sizeof(btapp_device_db.device[i+1]));

        }

        if (i==(BTAPP_NUM_REM_DEVICE-1))
            memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i+1]));
        break;

    }

    /* update data base in nvram */
    btapp_nv_store_device_db();

}

