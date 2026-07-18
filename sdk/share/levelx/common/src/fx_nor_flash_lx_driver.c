/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** FileX Component                                                       */ 
/**                                                                       */
/**   FileX NOR FLASH Driver                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#include "fx_api.h"
#include "lx_api.h"


/* Create a NOR flash control block.  */

/* Define prototypes.  */
UINT  ite_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash);

VOID  _fx_nor_flash_lx_driver(FX_MEDIA *media_ptr)
{

UCHAR       *source_buffer;
UCHAR       *destination_buffer;
ULONG       logical_sector;
ULONG       i;
UINT        status;


    /* There are several useful/important pieces of information contained in the media
       structure, some of which are supplied by FileX and others are for the driver to
       setup. The following is a summary of the necessary FX_MEDIA structure members:
       
            FX_MEDIA Member                              Meaning
                                        
        fx_media_driver_request             FileX request type. Valid requests from FileX are 
                                            as follows:

                                                    FX_DRIVER_READ
                                                    FX_DRIVER_WRITE                 
                                                    FX_DRIVER_FLUSH
                                                    FX_DRIVER_ABORT
                                                    FX_DRIVER_INIT
                                                    FX_DRIVER_BOOT_READ
                                                    FX_DRIVER_RELEASE_SECTORS
                                                    FX_DRIVER_BOOT_WRITE
                                                    FX_DRIVER_UNINIT

        fx_media_driver_status              This value is RETURNED by the driver. If the 
                                            operation is successful, this field should be
                                            set to FX_SUCCESS for before returning. Otherwise,
                                            if an error occurred, this field should be set
                                            to FX_IO_ERROR. 

        fx_media_driver_buffer              Pointer to buffer to read or write sector data.
                                            This is supplied by FileX.

        fx_media_driver_logical_sector      Logical sector FileX is requesting.

        fx_media_driver_sectors             Number of sectors FileX is requesting.


       The following is a summary of the optional FX_MEDIA structure members:
       
            FX_MEDIA Member                              Meaning
                                        
        fx_media_driver_info                Pointer to any additional information or memory.
                                            This is optional for the driver use and is setup
                                            from the fx_media_open call. The RAM disk uses
                                            this pointer for the RAM disk memory itself.

        fx_media_driver_write_protect       The DRIVER sets this to FX_TRUE when media is write 
                                            protected. This is typically done in initialization, 
                                            but can be done anytime.

        fx_media_driver_free_sector_update  The DRIVER sets this to FX_TRUE when it needs to 
                                            know when clusters are released. This is important
                                            for FLASH wear-leveling drivers.

        fx_media_driver_system_write        FileX sets this flag to FX_TRUE if the sector being
                                            written is a system sector, e.g., a boot, FAT, or 
                                            directory sector. The driver may choose to use this
                                            to initiate error recovery logic for greater fault
                                            tolerance.

        fx_media_driver_data_sector_read    FileX sets this flag to FX_TRUE if the sector(s) being
                                            read are file data sectors, i.e., NOT system sectors.

        fx_media_driver_sector_type         FileX sets this variable to the specific type of 
                                            sector being read or written. The following sector
                                            types are identified:

                                                    FX_UNKNOWN_SECTOR 
                                                    FX_BOOT_SECTOR
                                                    FX_FAT_SECTOR
                                                    FX_DIRECTORY_SECTOR
                                                    FX_DATA_SECTOR  
    */

    /* Process the driver request specified in the media control block.  */
    FX_DRV_INFO *drvInfo = (FX_DRV_INFO *)media_ptr -> fx_media_driver_info;
    LX_NOR_FLASH *norFlash = (LX_NOR_FLASH *)drvInfo->lxDriverInfo.wearDriver;
    switch(media_ptr -> fx_media_driver_request)
    {

        case FX_DRIVER_READ:
        {

            /* Setup the destination buffer and logical sector.  */
            logical_sector =      media_ptr -> fx_media_driver_logical_sector;
            destination_buffer =  (UCHAR *) media_ptr -> fx_media_driver_buffer;
            
            /* Loop to read sectors from flash.  */
            for (i = 0; i < media_ptr -> fx_media_driver_sectors; i++)
            {
            
                /* Read a sector from NOR flash.  */
                status =  lx_nor_flash_sector_read(norFlash, logical_sector, destination_buffer);
                /* Determine if the read was successful.  */
                if (status != LX_SUCCESS)
                {
                
                    /* Return an I/O error to FileX.  */
                    printf("levelx read error %X\r\n", status);
                    media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                    
                    return;
                } 

                /* Move to the next entries.  */
                logical_sector++;
                destination_buffer =  destination_buffer + media_ptr -> fx_media_bytes_per_sector;
            }

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }
        
        case FX_DRIVER_WRITE:
        {

            /* Setup the source buffer and logical sector.  */
            logical_sector =      media_ptr -> fx_media_driver_logical_sector;
            source_buffer =       (UCHAR *) media_ptr -> fx_media_driver_buffer;
            /* Loop to write sectors to flash.  */
            for (i = 0; i < media_ptr -> fx_media_driver_sectors; i++)
            {
            
                /* Write a sector to NOR flash.  */
                status =  lx_nor_flash_sector_write(norFlash, logical_sector, source_buffer);

                /* Determine if the write was successful.  */
                if (status != LX_SUCCESS)
                {
                
                    /* Return an I/O error to FileX.  */
                    printf("levelx write error %X\r\n", status);
                    media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                    
                    return;
                } 

                /* Move to the next entries.  */
                logical_sector++;
                source_buffer =  source_buffer + media_ptr -> fx_media_bytes_per_sector;
            }

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_RELEASE_SECTORS:
        {
        
            /* Setup the logical sector.  */
            logical_sector =  media_ptr -> fx_media_driver_logical_sector;

            /* Release sectors.  */
            for (i = 0; i < media_ptr -> fx_media_driver_sectors; i++)
            {
            
                /* Release NOR flash sector.  */
                status =  lx_nor_flash_sector_release(norFlash, logical_sector);

                /* Determine if the sector release was successful.  */
                if (status != LX_SUCCESS)
                {
                
                    /* Return an I/O error to FileX.  */
                    media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                    
                    return;
                } 

                /* Move to the next entries.  */
                logical_sector++;
            }

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_FLUSH:
        {

            /* Return driver success.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_ABORT:
        {

            /* Return driver success.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_INIT:
        {

            /* FLASH drivers are responsible for setting several fields in the 
               media structure, as follows:

                    media_ptr -> fx_media_driver_free_sector_update
                    media_ptr -> fx_media_driver_write_protect

               The fx_media_driver_free_sector_update flag is used to instruct
               FileX to inform the driver whenever sectors are not being used.
               This is especially useful for FLASH managers so they don't have 
               maintain mapping for sectors no longer in use.

               The fx_media_driver_write_protect flag can be set anytime by the
               driver to indicate the media is not writable.  Write attempts made
               when this flag is set are returned as errors.  */

            /* Perform basic initialization here... since the boot record is going
               to be read subsequently and again for volume name requests.  */

            /* With flash wear leveling, FileX should tell wear leveling when sectors
               are no longer in use.  */
            media_ptr -> fx_media_driver_free_sector_update =  FX_TRUE;

            media_ptr -> fx_media_driver_write_protect = drvInfo->writeProtect;        
            /* Open the NOR flash simulation.  */
            norFlash->lx_media_driver_info = media_ptr -> fx_media_driver_info;
#if 0            
            status =  lx_nor_flash_open(norFlash, "nor flash", ite_lx_nor_flash_driver_initialize);
            /* Determine if the flash open was successful.  */
            if (status != LX_SUCCESS)
            {
                
                /* Return an I/O error to FileX.  */
                media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                printf("lx_nor_flash_open error %X\r\n", status);
                return;
            }
            if(drvInfo->lxDriverInfo.wearCache && drvInfo->lxDriverInfo.wearCacheSize)
            {
                status = lx_nor_flash_extended_cache_enable(norFlash, drvInfo->lxDriverInfo.wearCache, drvInfo->lxDriverInfo.wearCacheSize);
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }
#else
            if(drvInfo->lxDriverInfo.wearCache && drvInfo->lxDriverInfo.wearCacheSize)
            {
                FX_DRV_INFO*   nor_driver = (FX_DRV_INFO*)norFlash->lx_media_driver_info;
                F_PHY phy;
                nor_driver->driver->getphy(nor_driver->driver, &phy);
                ULONG bytes_per_block =  phy.sectors_per_cluster * phy.bytes_per_sector;
                ULONG sectors_mapping_size =  nor_driver->sectorNum * sizeof(ULONG);
                
                ULONG *lx_mapping_buf = malloc(sectors_mapping_size);
                ULONG *lx_work_buf = malloc(bytes_per_block);
                status = lx_nor_flash_open_with_cache(norFlash, "nor flash", ite_lx_nor_flash_driver_initialize, drvInfo->lxDriverInfo.wearCache, 
                                                        drvInfo->lxDriverInfo.wearCacheSize, lx_work_buf, bytes_per_block, lx_mapping_buf, sectors_mapping_size);
                free(lx_work_buf);
            }
            else status = LX_ERROR;
#endif            
            media_ptr -> fx_media_driver_status =  status ? FX_IO_ERROR : FX_SUCCESS;
            break;
        }

        case FX_DRIVER_UNINIT:
        {

            /* There is nothing to do in this case for the RAM driver.  For actual
               devices some shutdown processing may be necessary.  */

            /* Close the NOR flash simulation.  */
            status =  lx_nor_flash_close(norFlash);

            /* Determine if the flash close was successful.  */
            if (status != LX_SUCCESS)
            {
                
                /* Return an I/O error to FileX.  */
                media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                printf("lx_nor_flash_close error %X\r\n", status);
                return;
            } 
            if(norFlash->lx_nor_flash_mapping_map)
            {
                free(norFlash->lx_nor_flash_mapping_map);   // free lx_mapping_buf
                norFlash->lx_nor_flash_mapping_map = NULL;
            }
            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }
        
        case FX_DRIVER_BOOT_READ:
        {

            /* Read the boot record and return to the caller.  */

            /* Setup the destination buffer.  */
            destination_buffer =  (UCHAR *) media_ptr -> fx_media_driver_buffer;

            /* Read boot sector from NOR flash.  */
            status =  lx_nor_flash_sector_read(norFlash, 0, destination_buffer);

            /* For NOR driver, determine if the boot record is valid.  */
            if ((destination_buffer[0] != (UCHAR) 0xEB) ||
                (destination_buffer[1] != (UCHAR) 0x34) ||
                (destination_buffer[2] != (UCHAR) 0x90))
            {

                /* Invalid boot record, return an error!  */
                media_ptr -> fx_media_driver_status =  FX_MEDIA_INVALID;
                return;
            }

            /* Determine if the boot read was successful.  */
            if (status != LX_SUCCESS)
            {
                
                /* Return an I/O error to FileX.  */
                media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                    
                return;
            } 

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_BOOT_WRITE:
        {

            /* Make sure the media bytes per sector equals to the LevelX logical sector size.  */
            
            if (media_ptr -> fx_media_bytes_per_sector != (LX_NOR_SECTOR_SIZE) * sizeof(ULONG))
            {

                /* Sector size mismatch, return error.  */
                printf("Sector size mismatch %X\r\n", media_ptr -> fx_media_bytes_per_sector);
            
                media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                break;
            }

            /* Write the boot record and return to the caller.  */

            /* Setup the source buffer.  */
            source_buffer =       (UCHAR *) media_ptr -> fx_media_driver_buffer;

            /* Write boot sector to NOR flash.  */
            status =  lx_nor_flash_sector_write(norFlash, 0, source_buffer);
            
            /* Determine if the boot write was successful.  */
            if (status != LX_SUCCESS)
            {
                
                /* Return an I/O error to FileX.  */
                media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
                    
                return;
            } 

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break ;
        }

        default:
        {

            /* Invalid driver request.  */
            media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
            break;
        }
    }
}

