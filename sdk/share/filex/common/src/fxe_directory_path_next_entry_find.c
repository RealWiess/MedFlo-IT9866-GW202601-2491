
/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** FileX Component                                                       */
/**                                                                       */
/**   Directory                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define FX_SOURCE_CODE


/* Include necessary system files.  */

#include "fx_api.h"
#include "fx_directory.h"

FX_CALLER_CHECKING_EXTERNS


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _fxe_directory_path_next_entry_find                 PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Jack Yu, iTE Corporation                                            */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks the next directory entry find call for errors. */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    media_ptr                             Media control block pointer   */
/*    search_path                           Target search directory path  */
/*    directory_name                        Destination for directory     */
/*                                            name                        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    return status                                                       */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _fx_directory_path_next_entry_find    Actual entry find service     */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-12-2024        Jack Yu               Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _fxe_directory_path_next_entry_find(FX_MEDIA *media_ptr, FX_PATH *search_path, CHAR *directory_name)
{

UINT status;


    /* Check for a null media pointer.  */
    if (media_ptr == FX_NULL || search_path == FX_NULL)
    {
        return(FX_PTR_ERROR);
    }

    /* Check for a valid caller.  */
    FX_CALLER_CHECKING_CODE

    /* Call actual next directory entry find service.  */
    status =  _fx_directory_path_next_entry_find(media_ptr, search_path, directory_name);

    /* Return status to the caller.  */
    return(status);
}

