#if defined(_WIN32)
#include "wifiMgr.h"

int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    return 0;
}


int
WifiMgr_Get_Connect_State(int *conn_state, int *e_code)
{
    int nRet = WIFIMGR_ECODE_OK;

    *conn_state = 0;
    *e_code = 0;

    return nRet;
}


int WifiMgr_Sta_Connect_AP(char* ssid, char* password, char* secumode)
{
    return WIFIMGR_ECODE_OK;
}


int WifiMgr_Sta_Disconnect()
{
    return WIFIMGR_ECODE_OK;
}


void WifiMgr_Sta_Cancel_Connect()
{
    return;
}
#endif

