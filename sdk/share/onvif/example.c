#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "onvif/onvif.h"


//=============================================================================
//                              Onvif Example
//=============================================================================

void Onvif_Example(void)
{
    onvifConn *onvif_conn = (onvifConn *)malloc(sizeof(onvifConn));
    onvifData *onvif_data = (onvifData *)malloc(sizeof(onvifData));
    int num_camera = 0, num_xaddrs = 0;
    int i = 0, j = 0;
    char ip[128] = {0};

    printf("Onvif test start\n");

    // inif Onvif Conntrack
    OnviInit(onvif_conn);

    // get the number of IPCam
    num_camera = OnvifDiscovery(onvif_conn);
    printf("The total numbers of IPCam: %d\n", num_camera);

    for(i = 0; i < num_camera; i++) {
        // get IPCam's device info from conntrack
        OnvifDataParser(i, onvif_conn, onvif_data);
        //printf("Cam Name: %s ,Xaddrs: %s\n", onvif_data->camera_name, onvif_data->xaddrs);

        // get IPCam's device IPaddr from Xaddrs
        num_xaddrs = OnvifGetxAddrsNum(onvif_data->xaddrs);
        for(j = 0; j < num_xaddrs; j++) {
            OnvifTransferAddr(j, ip, onvif_data->xaddrs);
            printf("ip: %s\n", ip);
            memset(ip, 0, 128);
        }

        // need auth data for GetProfiles form IPCam
        if(strcmp(onvif_data->camera_name, "Dahua IPC-HDW1025C") == 0) {
            strcpy(onvif_data->username, "admin");
            strcpy(onvif_data->password, "htc123456");
        }
        else if(strcmp(onvif_data->camera_name, "Dahua IPC-HFW1105D") == 0) {
            strcpy(onvif_data->username, "admin");
            strcpy(onvif_data->password, "admin");
        }
        else if(strcmp(onvif_data->camera_name, "HIKVISION DS-2CD1203D-I3") == 0) {
            strcpy(onvif_data->username, "admin");
            strcpy(onvif_data->password, "12345");
        }
        else {
            printf("No support Camera(%s)\n", onvif_data->camera_name);
            break;
        }

        // get encode_type, width, height, and frame_rate from GetProfiles request
        if(OnvifGetProfile(onvif_data) == 0) {
            //printf("encode_type: %s\n", onvif_data->encode_type);
            //printf("w= %d h= %d frame_rate= %d\n", onvif_data->width, onvif_data->height, onvif_data->frame_rate);
            OnvifDataPrint(onvif_data);
        }
        else {
            printf("error: %s\n", onvif_data->error_msg);
        }

    }

    // deinit Onvif
    OnvifDeinit();

    // free onvif conntrack and data
    free(onvif_conn);
    free(onvif_data);
    
    printf("Onvif test end\n");

}


