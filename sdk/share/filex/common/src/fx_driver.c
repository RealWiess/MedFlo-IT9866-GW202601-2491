/** @file
 * ITE nor dirver for FileX.
 *
 * @author Jack Yu
 * @version 1.0
 * @date 2024
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** FileX Component                                                       */
/**                                                                       */
/**   Nor Driver                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#include "fx_api.h"
#include <stdio.h>

#include "ite/ith.h"


VOID  _fx_driver(FX_MEDIA *media_ptr)
{

UCHAR *source_buffer;
UCHAR *destination_buffer;
UINT   bytes_per_sector;
uint32_t norResult = 0;
FX_DRV_INFO *drvInfo = (FX_DRV_INFO *)media_ptr -> fx_media_driver_info;
        
    /* There are several useful/important pieces of information contained in
       the media structure, some of which are supplied by FileX and others
       are for the driver to setup. The following is a summary of the
       necessary FX_MEDIA structure members:

            FX_MEDIA Member                    Meaning

        fx_media_driver_request             FileX request type. Valid requests from
                                            FileX are as follows:

                                                    FX_DRIVER_READ
                                                    FX_DRIVER_WRITE
                                                    FX_DRIVER_FLUSH
                                                    FX_DRIVER_ABORT
                                                    FX_DRIVER_INIT
                                                    FX_DRIVER_BOOT_READ
                                                    FX_DRIVER_RELEASE_SECTORS
                                                    FX_DRIVER_BOOT_WRITE
                                                    FX_DRIVER_UNINIT

        fx_media_driver_status              This value is RETURNED by the driver.
                                            If the operation is successful, this
                                            field should be set to FX_SUCCESS for
                                            before returning. Otherwise, if an
                                            error occurred, this field should be
                                            set to FX_IO_ERROR.

        fx_media_driver_buffer              Pointer to buffer to read or write
                                            sector data. This is supplied by
                                            FileX.

        fx_media_driver_logical_sector      Logical sector FileX is requesting.

        fx_media_driver_sectors             Number of sectors FileX is requesting.


       The following is a summary of the optional FX_MEDIA structure members:

            FX_MEDIA Member                              Meaning

        fx_media_driver_info                Pointer to any additional information
                                            or memory. This is optional for the
                                            driver use and is setup from the
                                            fx_media_open call. The RAM disk uses
                                            this pointer for the RAM disk memory
                                            itself.

        fx_media_driver_write_protect       The DRIVER sets this to FX_TRUE when
                                            media is write protected. This is
                                            typically done in initialization,
                                            but can be done anytime.

        fx_media_driver_free_sector_update  The DRIVER sets this to FX_TRUE when
                                            it needs to know when clusters are
                                            released. This is important for FLASH
                                            wear-leveling drivers.

        fx_media_driver_system_write        FileX sets this flag to FX_TRUE if the
                                            sector being written is a system sector,
                                            e.g., a boot, FAT, or directory sector.
                                            The driver may choose to use this to
                                            initiate error recovery logic for greater
                                            fault tolerance.

        fx_media_driver_data_sector_read    FileX sets this flag to FX_TRUE if the
                                            sector(s) being read are file data sectors,
                                            i.e., NOT system sectors.

        fx_media_driver_sector_type         FileX sets this variable to the specific
                                            type of sector being read or written. The
                                            following sector types are identified:

                                                    FX_UNKNOWN_SECTOR
                                                    FX_BOOT_SECTOR
                                                    FX_FAT_SECTOR
                                                    FX_DIRECTORY_SECTOR
                                                    FX_DATA_SECTOR
     */

    /* Process the driver request specified in the media control block.  */
    switch (media_ptr -> fx_media_driver_request)
    {

    case FX_DRIVER_READ:
    {
        norResult = drvInfo->driver->readmultiplesector(drvInfo->driver, media_ptr->fx_media_driver_buffer, media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors, media_ptr->fx_media_driver_sectors);
        media_ptr -> fx_media_driver_status =  norResult;
        break;
    }

    case FX_DRIVER_WRITE:
    {        
        norResult = drvInfo->driver->writemultiplesector(drvInfo->driver, media_ptr->fx_media_driver_buffer, media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors, media_ptr->fx_media_driver_sectors);
        media_ptr -> fx_media_driver_status =  norResult;
        break;
    }

    case FX_DRIVER_FLUSH:
    {
        if(drvInfo->driver->ioctl)
            norResult = drvInfo->driver->ioctl(drvInfo->driver, F_IOCTL_MSG_FLUSH, NULL, NULL);
        else
            norResult = 0;
        media_ptr -> fx_media_driver_status = norResult;
        break;
    }

    case FX_DRIVER_ABORT:
    {
        /* Nothing to do in iTE driver, successful driver request.  */
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

        /* Nothing to do in iTE driver, successful driver request.  */
        media_ptr -> fx_media_driver_write_protect = drvInfo->writeProtect;        
        media_ptr -> fx_media_driver_status =  FX_SUCCESS;
        break;
    }

    case FX_DRIVER_UNINIT:
    {
        /* Nothing to do in iTE driver, successful driver request.  */
        media_ptr -> fx_media_driver_status =  FX_SUCCESS;
        break;
    }

    case FX_DRIVER_BOOT_READ:
    {
        F_PHY phy;
        drvInfo->driver->getphy(drvInfo->driver, &phy);
        bytes_per_sector = phy.bytes_per_sector;
        if (bytes_per_sector > media_ptr -> fx_media_memory_size)
        {
            media_ptr -> fx_media_driver_status =  FX_BUFFER_ERROR;
            printf("Media memory too small: driver size %d, memory size %d\r\n", bytes_per_sector, media_ptr -> fx_media_memory_size);
            break;
        }
       
        norResult = drvInfo->driver->readsector(drvInfo->driver, media_ptr->fx_media_driver_buffer, drvInfo->sectorStart);
        media_ptr -> fx_media_driver_status =  norResult;
        break;
    }

    case FX_DRIVER_BOOT_WRITE:
    {
        FX_DRV_INFO *drvInfo = (FX_DRV_INFO *)media_ptr -> fx_media_driver_info;
        norResult = drvInfo->driver->writesector(drvInfo->driver, media_ptr->fx_media_driver_buffer, media_ptr->fx_media_hidden_sectors);
        media_ptr -> fx_media_driver_status =  norResult;
        break;
    }

    default:
    {
        /* Invalid driver request.  */
        printf("Invalid driver request. %d\r\n", media_ptr -> fx_media_driver_request);
        media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
        break;
    }
    }
}

