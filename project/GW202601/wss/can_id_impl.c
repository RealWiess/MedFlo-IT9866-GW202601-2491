#include "SDL/SDL.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "can_id_impl.h"
#include "util.h"
#include "canbus_fmt.h"
#include "mqueue_dashboard.h"

void printByte(unsigned char *start, int len)
{
    printf("byte is\n");
    for(int i = 0; i < len; i++) {
        printf("%2x ", *start);
        start++;
    }

    printf("\nend\n");
}

void findCanMsgImpl(unsigned char* input, int cmdLen, int bodyLen)
{
    unsigned int canId = 0;
    size_t i  = 0;
    DashboardEvent_t dbEvt= {0};
    uint32_t** canData;

    for (i = 0; i < cmdLen; i++)
        canId += (input[i] << (i * 8));
    input += cmdLen;
#if 0
    printf("can id = 0x%X\n", canId);
    for (i = 0; i < 8; i++) {
        printf("%X ", input[i]);
    }
    printf("\n");
#endif
    canData = CanFmtParser(canId, input);
    if (canData) {
        dbEvt.canid = canId;
        dbEvt.data = canData;
        dbEvt.length = CanFmtNodeCnt();
        dashboardQueueSend(&dbEvt);
    }
}

