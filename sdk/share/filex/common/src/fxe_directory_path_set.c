
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
/*    _fxe_directory_path_set                          PORTABLE C         */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Jack Yu, iTE Corporation                                            */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks for errors in the directory set call.          */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    media_ptr                             Media control block pointer   */
/*    search_path                           Return for directory          */
/*    new_path_name                         New path to set current       */
/*                                            working directory to        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    return status                                                       */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _fx_directory_path_set                Actual directory set service  */
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
UINT  _fxe_directory_path_set(FX_MEDIA *media_ptr, FX_PATH *search_path, CHAR *new_path_name)
{

UINT status;


    /* Check for a null media pointer.  */
    if (media_ptr == FX_NULL || search_path == FX_NULL)
    {
        return(FX_PTR_ERROR);
    }

    /* Check for a valid caller.  */
    FX_CALLER_CHECKING_CODE

    /* Call actual default directory set service.  */
    status =  _fx_directory_path_set(media_ptr, search_path, new_path_name);

    /* Default directory set is complete, return status.  */
    return(status);
}

