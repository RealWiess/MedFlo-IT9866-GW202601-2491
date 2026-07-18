
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
#include "fx_system.h"
#include "fx_directory.h"
#include "fx_utility.h"

#ifndef FX_NO_LOCAL_PATH
FX_LOCAL_PATH_SETUP
#endif


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _fx_directory_path_first_entry_find                 PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Jack Yu, iTE Corporation                                            */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function returns the first directory entry of the input        */
/*    directory.                                                          */
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
/*    _fx_directory_path_next_entry_find    Find next directory entry     */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    FileX System Functions                                              */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-12-2024        Jack Yu               Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _fx_directory_path_first_entry_find(FX_MEDIA *media_ptr, FX_PATH *search_path, CHAR *directory_name)
{

UINT status;


#ifndef FX_MEDIA_STATISTICS_DISABLE

    /* Increment the number of times this service has been called.  */
    media_ptr -> fx_media_directory_first_entry_finds++;
#endif

    /* Check the media to make sure it is open.  */
    if (media_ptr -> fx_media_id != FX_MEDIA_ID)
    {

        /* Return the media not opened error.  */
        return(FX_MEDIA_NOT_OPEN);
    }

    /* If trace is enabled, insert this event into the trace buffer.  */
    FX_TRACE_IN_LINE_INSERT(FX_TRACE_DIRECTORY_FIRST_ENTRY_FIND, media_ptr, directory_name, 0, 0, FX_TRACE_DIRECTORY_EVENTS, 0, 0)

    /* Protect against other threads accessing the media.  */
    FX_PROTECT

    /* Set the current entry to 0 in order to pickup the first entry.  */
    search_path->fx_path_current_entry =  0;

    /* Release media protection.  */
    FX_UNPROTECT

    /* Call the next directory entry to pickup the first entry.  */
    status =  _fx_directory_path_next_entry_find(media_ptr, search_path, directory_name);

    /* Return status to the caller.  */
    return(status);
}

