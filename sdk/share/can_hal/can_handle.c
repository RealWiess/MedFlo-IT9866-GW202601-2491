#include <sys/ioctl.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can_handle.h"
#if (CFG_CHIP_FAMILY == 9860)
#include "can_bus/it9860/can_api.h"
#endif

#define CAN_TRANSCEIVER_1043 1 // 1043 TYPE
//#define CAN_TRANSCEIVER_1044 1

#define CAN_RX 14
#define CAN_TX 13

#if defined(CAN_TRANSCEIVER_1044)
#define CAN_TRV_STBY 11
#endif

#if defined(CAN_TRANSCEIVER_1043)
#define  TJA1043_STB_N   16
#define  TJA1043_EN      18
#endif

CAN_HANDLE      *can0;
static pthread_t canTask;
static bool canQuit;
static void* CANTask(void* arg);


CAN_FILTEROBJ FilterTable[16]= { {true,  0x0, 0x1FFFFFFF, false, false},//FILTER0
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER1
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER2
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER3
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER4
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER5
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER6
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER7
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER8
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER9
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER10
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER11 
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER12
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER13
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER14
                                 {false, 0x0, 0x1FFFFFFF, false, false},//FILTER15                                 
};


#if defined(CAN_TRANSCEIVER_1043)

void TJA1043_SETMODE(int mode)
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
#endif

void canExit(void)
{
	canQuit = true;

	pthread_join(canTask, NULL);
}

void canInit(void)
{
    pthread_create(&canTask, NULL, CANTask, NULL);
}

void canSleep(void)
{
#if defined(CFG_EXTERNAL_POWER_ON)
	//go to Sleep ,then Power down
	ioctl(ITP_DEVICE_POWER, ITP_IOCTL_OFF, NULL);
#endif	
#if defined(CAN_TRANSCEIVER_1043)
	TJA1043_SETMODE(2); //sleep mode
#endif
}

static void CAN0IntrHandler(void)
{
    uint32_t     flagstatus = ithCANGetIntrFlag(can0);

	//Receive interrupt:data or a remote frame has been received.
    if (flagstatus & CAN_RIF)//Receive interrupt Flag
    {
        ithCANClearIntrFlag(can0, Receive_Interrupt_Flag);
		CAN_RXOBJ _rxObj = {0}; 
		while(!ithCANRead(can0, &_rxObj))
		{	
			#if 1
	        ithPrintf("can0 ID[%x]: IDE = %x,DLC = %x,RTS[0] = %x\n"
			  , _rxObj.Identifier, _rxObj.Control.IDE, _rxObj.Control.DLC, _rxObj.RXRTS[0]);
			
			ithPrintf("data[0-7]=%x %x %x %x %x %x %x %x\n", _rxObj.RXData[0],  _rxObj.RXData[1],	_rxObj.RXData[2],  _rxObj.RXData[3]
				,  _rxObj.RXData[4],	_rxObj.RXData[5],  _rxObj.RXData[6],  _rxObj.RXData[7]);
			#endif
		}
    }

	//RB Full Interrupt Flag:All RB FIFO is full.
    if (flagstatus & CAN_RFIF)
    {
    	ithPrintf("RB buffer full\n");
        ithCANClearIntrFlag(can0, RB_Full_Interrupt_Flag);
    }

	//PTB Transmission interrupt:the requested transmission has been successfully completed.
    if (flagstatus & CAN_TPIF)//Transmission Primary Interrupt Flag
    {
        ithCANClearIntrFlag(can0, TP_Interrupt_Flag);
    }

    //STB Transmission interrupt:the requested transmission has been successfully completed.
    if (flagstatus & CAN_TSIF)//Transmission Secondary Interrupt Flag
    {
        ithCANClearIntrFlag(can0, TS_Interrupt_Flag);
    }

	//BUS Error interrupt: a can bus error can be signaled, every new error event overwites the prvious stored value of KORE
    if (flagstatus & CAN_BEIF)///BUS ERROR Interrupt Flag
    {
        ithCANClearIntrFlag(can0, Bus_Error_Interrupt_Flag);
    }

	//Error interrupt:the border of error warning limit(EWL) has been crossed by RECNT or TECNT or BUSOFF bit has bit changed
    if (flagstatus & CAN_EIF)///Error Interrupt Flag
    {
        ithCANClearIntrFlag(can0, Error_Interrupt_Flag);
    }
	
	//Arbitration_Lost interrupt
	if (flagstatus & CAN_ALIF)
    {
        ithCANClearIntrFlag(can0, Arbitration_Lost_Interrupt_Flag);
    }

    //Abort Interrupt Flag: After setting TPA or TSA, the messages have been aborted.
	//This interrupt is always enabled
    if (flagstatus & CAN_AIF)//Abort Interrupt Flag
    {
        ithCANClearIntrFlag(can0, Abort_Interrupt_Flag);
    }	
}

static void* CANTask(void* arg)
{
	CAN_TXOBJ _txObj = {0};
	uint8_t  txbuffer[8] = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};	

	//<can0 rx pin ,tx pin >
	ithCANSetGPIO(0, CAN_RX, CAN_TX);

#if defined(CAN_TRANSCEIVER_1044)	
	//<transceiver stby pin>
	ithGpioSetOut(CAN_TRV_STBY);
	ithGpioSetMode(CAN_TRV_STBY, ITH_GPIO_MODE0);
	ithGpioSet(CAN_TRV_STBY);
	printf("Set RX GPIO(%d) TX GPIO(%d) STBY GPIO(%d)\n",CAN_RX, CAN_TX, CAN_TRV_STBY);
#endif

#if defined(CAN_TRANSCEIVER_1043)
	TJA1043_SETMODE(0); //normal mode
#endif


	//define can0
	can0						  = (CAN_HANDLE *)malloc(sizeof(CAN_HANDLE));
	can0->Instance 			  	  = 0;
	can0->ADDR 				      = CAN0_BASE_ADDRESS;
	can0->BaudRate 			      = CAN_500K_1M;
	can0->SourceClock			  = ithGetAPB2clk();
	can0->ProtocolType 		      = protocol_CAN_2_0B;
	can0->ExternalLoopBackMode    = false;
	can0->InternalLoopBackMode    = false;
	can0->ListenOnlyMode		  = false;
	can0->TPtr 				      = FilterTable;
	can0->SlowBitRate.Bit_Time    = 0;
	can0->SlowBitRate.Prescaler   = 0;
	can0->SlowBitRate.Seg_1	      = 0;
	can0->SlowBitRate.Seg_2	      = 0;
	can0->SlowBitRate.SJW		  = 0;
	can0->SlowBitRate.SSPOFF	  = 0;
	can0->InterruptTable          = (Receive_INT | RB_Full_INT | TS_INT | TP_INT);
	can0->InterruptHD			  = CAN0IntrHandler;

	//try open & init can ctrl
	ithCANOpen(can0);

#if 0 // first shot for test
	_txObj.Identifier  = 0xFF;
	_txObj.Control.DLC = CAN_DLC_8;
	_txObj.Control.RTR = 0x0;
	_txObj.Control.IDE = 0x0;
	_txObj.SingleShot  = 0x0;
	_txObj.TTSENSEL    = 0x0;
	_txObj.Timeout     = 1000;		 
	if(ithCANWrite(can0, &_txObj, txbuffer))
	{
		printf("can0 write timeout\n");
	}
#endif
		 
	for (;;)
	{		
		if(canQuit)
		{
			canQuit = false;
			break;
		}
		usleep(100*1000);

	}

	ithCANClose(can0);

#if defined(CAN_TRANSCEIVER_1044)
	//<transceiver stby pin>
	ithGpioClear(CAN_TRV_STBY);
#endif
#if defined(CAN_TRANSCEIVER_1043)
	TJA1043_SETMODE(1); //standby mode
#endif

	free(can0);
	
}



