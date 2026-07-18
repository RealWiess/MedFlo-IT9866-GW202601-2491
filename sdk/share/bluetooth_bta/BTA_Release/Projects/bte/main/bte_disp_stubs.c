/*****************************************************************************
 * Copyright (c) 2018-2019, Broadcom Inc.                                    *
 *                                                                           *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.              *
 * the contents of this file may not be disclosed to third parties, copied   *
 * or duplicated in any form, in whole or in part, without the prior         *
 * written permission of Broadcom Inc.                                       *
 *****************************************************************************/
#include "bt_target.h"
#include "stdio.h"
#include "bt_trace.h"

void DispLMDiagEvent (BT_HDR *p_hdr)
{
}

void thru_acl_data(UINT16 len, UINT8 *p, BOOLEAN is_rcvd)
{
}

void thru_acl_change(UINT8 *p, BOOLEAN is_new)
{
}

char *HCIGetVendorSpecDesc(UINT16 opcode)
{
    return "Vendor Specific Command";
}

UINT8 *scru_dump_hex (UINT8 *p_data, char *p_title, UINT16 len, UINT32 trace_layer, UINT32 trace_type)
{
#if defined (BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
        UINT16  xx, yy;
        char buff1[60];
        char buff2[20];

        if (p_title)
        {
            ScrLog (TRACE_CTRL_GENERAL | trace_layer | TRACE_ORG_PROTO_DISP | trace_type,
                "%s:", p_title);
        }


        memset (buff2, ' ', 16);
        buff2[16] = 0;

        yy = snprintf (buff1, sizeof(buff1), "%04x: ", 0);
        for (xx = 0; xx < len; xx++)
        {
            if ( (xx) && ((xx & 15) == 0) )
            {
                ScrLog (TRACE_CTRL_GENERAL | trace_layer | TRACE_ORG_PROTO_DISP | trace_type,
                        "    %s  %s", buff1, buff2);
                yy = snprintf(buff1, sizeof(buff1), "%04x: ", xx);
                memset (buff2, ' ', 16);
            }
            if(sizeof(buff1)>yy)
            {
                yy += snprintf (&buff1[yy], sizeof(buff1)-yy, "%02x ", *p_data);
            }
            else
            {
                ScrLog (TRACE_CTRL_GENERAL | trace_layer | TRACE_ORG_PROTO_DISP | trace_type,
                        "scru_dump_hex ERROR");

            }

            if ((*p_data >= ' ') && (*p_data <= 'z'))
                buff2[xx & 15] = *p_data;
            else
                buff2[xx & 15] = '.';

            p_data++;
        }

        /* Pad out the remainder */
        for ( ; ; xx++)
        {
            if ((xx & 15) == 0)
            {
                ScrLog (TRACE_CTRL_GENERAL | trace_layer | TRACE_ORG_PROTO_DISP | trace_type,
                        "    %s  %s", buff1, buff2);
                break;
            }
            if(sizeof(buff1)>yy)
            {
                yy += snprintf (&buff1[yy], sizeof(buff1)-yy, "   ");
            }
            else
            {
                ScrLog (TRACE_CTRL_GENERAL | trace_layer | TRACE_ORG_PROTO_DISP | trace_type,
                        "scru_dump_hex ERROR");

            }
        }
#endif
        return (p_data);
    }


void DispSmpMsg(BT_HDR *p_buf, BOOLEAN is_recv)
{

}

