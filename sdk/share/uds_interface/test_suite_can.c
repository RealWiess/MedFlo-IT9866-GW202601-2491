/**
 * @file        test_suite_can.c
 *
 * @brief       User CAN API.
 */

#include <stdio.h>
#include <inttypes.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "can_bus/it9860/can_api.h"
#include "test_suite.h"
#include "user_config.h"

#if 0
#define EXAMPLE_CAN1_ID 0x765
#define EXAMPLE_CAN2_ID 0x123
#define EXAMPLE_CAN1_PAYLOAD 0x41424344
#define EXAMPLE_CAN2_PAYLOAD 0x81828384
#define GLOBAL_MASK 0

int canfd_test()
{
    int ret = 0;

    //printf("\r\ncan tx ->\n\r");

    return ret;
}
#endif

#define UDSCANQUEUE_SIZE 100

CAN_HANDLE g_canfdHandle = {0};
QueueHandle_t g_UDSCANqueue = NULL;

CAN_FILTEROBJ FilterTable[16] = {
    {true, 0x0, 0x1FFFFFFF, false, false},  // FILTER0
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER1
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER2
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER3
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER4
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER5
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER6
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER7
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER8
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER9
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER10
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER11
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER12
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER13
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER14
    {false, 0x0, 0x1FFFFFFF, false, false}, // FILTER15
};


static void tja1043_setmode_(int mode)
{
	ithGpioSetOut(TJA1043_STB_N);
	ithGpioSetMode(TJA1043_STB_N, ITH_GPIO_MODE0);
	ithGpioSetOut(TJA1043_EN);
	ithGpioSetMode(TJA1043_EN, ITH_GPIO_MODE0);

	switch(mode)
	{
	case 0: 
		printf("NORMAL MODE\n");
		ithGpioSet(TJA1043_STB_N);
		ithGpioSet(TJA1043_EN);			
		break;
	case 1:
		printf("STANBY MODE\n");
		ithGpioClear(TJA1043_STB_N);
		ithGpioClear(TJA1043_EN);	
		break;
	case 2:
	 	printf("SLEEP MODE\n");
		ithGpioClear(TJA1043_STB_N);
		ithGpioSet(TJA1043_EN);	
		break;
	default:
		printf("LISTEN-ONLY MODE\n");
		ithGpioSet(TJA1043_STB_N);
		ithGpioClear(TJA1043_EN);	
		break;

	}
}

static void CAN0IntrHandler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    CAN_RXOBJ _rxObj = {0};
    CAN_Tx CAN_req = {0};
    uint32_t flagstatus = ithCANGetIntrFlag(&g_canfdHandle);

    // Receive interrupt:data or a remote frame has been received.
    if (flagstatus & CAN_RIF) // Receive interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, Receive_Interrupt_Flag);

        while (ithCANRead(&g_canfdHandle, &_rxObj) == 0)
        {
            if (_rxObj.Identifier == GET_PHYSICAL_ECU_ADDRESS() || _rxObj.Identifier == GET_FUNCTIONAL_ECU_ADDRESS())
            {
                CAN_req.id = _rxObj.Identifier;
                CAN_req.tx_dl = ithCANDlcToBytes(_rxObj.Control.DLC);
                memcpy((void *)CAN_req.data, (void *)_rxObj.RXData, 8);

                if (xQueueSendToFrontFromISR(g_UDSCANqueue, (void *)&CAN_req, &xHigherPriorityTaskWoken) != pdPASS)
                {
                    // error print
                    ithPrintf("UDSCANQueue is full\n");
                }
            }
        }
    }

    // RB Full Interrupt Flag:All RB FIFO is full.
    if (flagstatus & CAN_RFIF)
    {
        ithCANClearIntrFlag(&g_canfdHandle, RB_Full_Interrupt_Flag);
        ithPrintf("RB buffer full\n");
    }

    // PTB Transmission interrupt:the requested transmission has been successfully completed.
    if (flagstatus & CAN_TPIF) // Transmission Primary Interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, TP_Interrupt_Flag);
    }

    // STB Transmission interrupt:the requested transmission has been successfully completed.
    if (flagstatus & CAN_TSIF) // Transmission Secondary Interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, TS_Interrupt_Flag);
    }

    // BUS Error interrupt: a can bus error can be signaled, every new error event overwites the prvious stored value of KORE
    if (flagstatus & CAN_BEIF) /// BUS ERROR Interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, Bus_Error_Interrupt_Flag);
    }

    // Error interrupt:the border of error warning limit(EWL) has been crossed by RECNT or TECNT or BUSOFF bit has bit changed
    if (flagstatus & CAN_EIF) /// Error Interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, Error_Interrupt_Flag);
    }

    // Abort Interrupt Flag: After setting TPA or TSA, the messages have been aborted.
    // This interrupt is always enabled
    if (flagstatus & CAN_AIF) // Abort Interrupt Flag
    {
        ithCANClearIntrFlag(&g_canfdHandle, Abort_Interrupt_Flag);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

uint8_t UdsCanWriteMsg(uint16_t canChannel, CAN_Tx *req)
{
    CAN_TXOBJ txObj = {0};
    uint8_t ret = -1;

    // printf("\n\r the CAN req id in UDSwrite is 0x%x",req->id);
    // printf("\n\r the CAN req data length is %d",req->tx_dl);

    txObj.Identifier = req->id;
    if (req->tx_dl >= 0 && req->tx_dl <= 8)
    {
        txObj.Control.DLC = req->tx_dl;
    }
#if CFG_CANBUS_FD_ENABLE
    else if (req->tx_dl > 8 && req->tx_dl <= 12)
    {
        txObj.Control.DLC = CAN_DLC_12;
    }
    else if (req->tx_dl > 12 && req->tx_dl <= 16)
    {
        txObj.Control.DLC = CAN_DLC_16;
    }
    else if (req->tx_dl > 16 && req->tx_dl <= 20)
    {
        txObj.Control.DLC = CAN_DLC_20;
    }
    else if (req->tx_dl > 20 && req->tx_dl <= 24)
    {
        txObj.Control.DLC = CAN_DLC_24;
    }
    else if (req->tx_dl > 24 && req->tx_dl <= 32)
    {
        txObj.Control.DLC = CAN_DLC_32;
    }
    else if (req->tx_dl > 32 && req->tx_dl <= 48)
    {
        txObj.Control.DLC = CAN_DLC_48;
    }
    else if (req->tx_dl > 48 && req->tx_dl <= 64)
    {
        txObj.Control.DLC = CAN_DLC_64;
    }
#endif
    txObj.Control.RTR = 0;
    txObj.Control.IDE = 0; // STD type
#if CFG_CANBUS_FD_ENABLE
    txObj.Control.BRS = 1;
    txObj.Control.EDL = 1;
#endif
    txObj.Timeout = 100;

    if (ithCANWrite(&g_canfdHandle, &txObj, req->data) == 0)
    {
        // printf("can_write SUCCESS %d\n\r");
        ret = 0;
    }
    else
    {
        // printf("can_write FAIL %d\n\r");
    }
    return ret;
}

void UdsCanInit(uint32_t canChannel)
{
    CAN_BTATTR DefaultBitRate = {0};

    g_UDSCANqueue = xQueueCreate(UDSCANQUEUE_SIZE, (unsigned portBASE_TYPE)sizeof(CAN_Tx));

    if (canChannel == 0)
    {
        // set GPIO by target board.
        //Ref IT9866_Standard EVB
        //<can0 rx pin 14 tx pin 13>
        ithCANSetGPIO(0, 14, 13);
        //Ref IT9866_Standard EVB End

        tja1043_setmode_(0);

        g_canfdHandle.Instance = canChannel;
        g_canfdHandle.ADDR = CAN0_BASE_ADDRESS;
        g_canfdHandle.BaudRate = CAN_500K_1M;
        g_canfdHandle.SourceClock = ithGetAPB2clk();
#if CFG_CANBUS_FD_ENABLE
        g_canfdHandle.ProtocolType = protocol_FD_ISO;
#else
        g_canfdHandle.ProtocolType = protocol_CAN_2_0B;
#endif
        g_canfdHandle.ExternalLoopBackMode = false;
        g_canfdHandle.InternalLoopBackMode = false;
        g_canfdHandle.ListenOnlyMode = false;
        g_canfdHandle.TPtr = FilterTable;
        g_canfdHandle.SlowBitRate = DefaultBitRate;
        g_canfdHandle.FastBitRate = DefaultBitRate;
        g_canfdHandle.InterruptTable = (Receive_INT | RB_Full_INT | TS_INT | TP_INT | Error_INT);
        g_canfdHandle.InterruptHD = CAN0IntrHandler;

        ithCANOpen(&g_canfdHandle);
    }
    else
    {
        // CAN 1 TODO...
    }
}

void UdsCanDeInit(uint32_t canChannel)
{

    if (canChannel == 0)
    {
        ithCANClose(&g_canfdHandle);
        tja1043_setmode_(1);
    }
    else
    {
    }


    vQueueDelete(g_UDSCANqueue);
    g_UDSCANqueue = NULL;
}
