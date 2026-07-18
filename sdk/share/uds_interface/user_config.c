/**
 * @file        User Config function.
 *
 */

#include <stdio.h>
#include <string.h>
#include "user_config.h"
#include "uds_server.h"

unsigned int SID_Function_Disable(uint8_t sid)
{
	if(sid == UDS_SID_AUTHENTICATION) //disable 0x29 services
		return 1;
	
	return 0;
}

unsigned int GET_PHYSICAL_ECU_ADDRESS()
{
	return PHYSICAL_ECU_ADDRESS;
}

unsigned int GET_FUNCTIONAL_ECU_ADDRESS()
{
	return FUNCTIONAL_ECU_ADDRESS;
}

unsigned int GET_DESTINATION_ECU_ADDRESS()
{
	return DESTINATION_ECU_ADDRESS;
}

unsigned char GET_TARGET_TYPE()
{
	return TARGET_TYPE;
}

unsigned char GET_ISOTP_PADDING_BYTE()
{
	return 0x00;
}
