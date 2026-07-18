#include <stdarg.h>
#include <string.h>
#include "ite/ith.h"
#include "arm_lite_dev/armlite_dev_utility.h"
#include "arm_lite_dev/can_bus_lite/can_bus_lite.h"
#include "arm_lite_dev/can_bus_lite/clock.h"


CAN_HANDLE can1  = {0};

unsigned long	startTime = 0;
unsigned long   tickPerUs = 0;
unsigned long   txTimeOut = 0;


ARMLITE_CANBUS_STATUS armlite_status = CANBUS_UNKNOWN;

ARMLITE_RING_BUFFER armlite_rb = {0};

CAN_FILTEROBJ CAN1FilterTable[16]= { {true,  0x0, 0x1FFFFFFF, false, false},//FILTER0
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

static void set_status_type(unsigned int type)
{
	unsigned int value = ARMLITE_COMMAND_REG_READ(CAN_LITE_ERROR_REG);
	value = (value | type);
	ARMLITE_COMMAND_REG_WRITE(CAN_LITE_ERROR_REG, value);
}

static unsigned int get_status_type(unsigned int type)
{
	unsigned int value = ARMLITE_COMMAND_REG_READ(CAN_LITE_ERROR_REG);
	value = (value & (type));
	return value;
}

static void clear_status_type(unsigned int type)
{
	unsigned int value = ARMLITE_COMMAND_REG_READ(CAN_LITE_ERROR_REG);
	value = (value & (~type));
	ARMLITE_COMMAND_REG_WRITE(CAN_LITE_ERROR_REG, value);
}

static void clear_all_status_type()
{
	ARMLITE_COMMAND_REG_WRITE(CAN_LITE_ERROR_REG, 0x0);
}


static void ringbuf_add(CAN_RXOBJ* data)
{
	if(armlite_rb.count > 0)
	{
		if(armlite_rb.end == armlite_rb.start)
		{
			//overflow
			set_status_type(CAN_LITE_SW_FIFO_FULL);
		}		
	}

	//copy data
	armlite_rb.endAddr = (armlite_rb.end * sizeof(CAN_RXOBJ)) + CAN_RX_BUFFER_OFFSET;
	memcpy((void*)armlite_rb.endAddr, (void*)data, sizeof(CAN_RXOBJ));
	#ifdef CFG_CPU_WB
    ithFlushDCacheRange((void*)armlite_rb.endAddr, sizeof(CAN_RXOBJ));
    ithFlushMemBuffer();
	#endif
	armlite_rb.end = (armlite_rb.end + 1) % RX_MAX_DATA_BUFFER;
	armlite_rb.count++;
	ARMLITE_COMMAND_REG_WRITE(CAN_LITE_RX_WR_PTR_REG, ((armlite_rb.count << 16) | armlite_rb.end));
}

static int ringbuf_update_info()
{
	unsigned int rd_ptr_reg  = 0;
	unsigned short cur_start = 0;
	if(armlite_status == CANBUS_OPEN)
	{
		rd_ptr_reg = ARMLITE_COMMAND_REG_READ(CAN_LITE_RX_RD_PTR_REG);
		cur_start  = (unsigned short)(rd_ptr_reg & 0xFFFF);
		
		if((cur_start - armlite_rb.start) >= 0)
		{
			armlite_rb.count -= (cur_start - armlite_rb.start);
		}
		else
		{
			armlite_rb.count -= ((unsigned short)(RX_MAX_DATA_BUFFER - armlite_rb.start) + cur_start);
		}

		armlite_rb.start = cur_start;

		ARMLITE_COMMAND_REG_WRITE(CAN_LITE_RX_WR_PTR_REG, ((armlite_rb.count << 16) | armlite_rb.end));
		return 0;
	}
	return -1; 
}

static void PollingInterruptFlag()
{
	CAN_RXOBJ _rxObj = {0};
	unsigned int flagstatus = 0;


	if(armlite_status == CANBUS_OPEN)
	{
		flagstatus = ithCANGetIntrFlag(&can1);
		if (flagstatus & CAN_RIF)//Receive interrupt Flag
	    {
	        ithCANClearIntrFlag(&can1, Receive_Interrupt_Flag);
			 
			while(ithCANRead(&can1, &_rxObj) == 0)
			{	
				ringbuf_add(&_rxObj);
			}
	    }

		//RB Full Interrupt Flag:All RB FIFO is full.
	    if (flagstatus & CAN_RFIF)
	    {
	        ithCANClearIntrFlag(&can1, RB_Full_Interrupt_Flag);

			set_status_type(CAN_LITE_HW_FIFO_FULL);
	    }	

		//PTB Transmission interrupt:the requested transmission has been successfully completed.
	    if (flagstatus & CAN_TPIF)//Transmission Primary Interrupt Flag
	    {	
	        ithCANClearIntrFlag(&can1, TP_Interrupt_Flag);
			clear_status_type(CAN_LITE_TX_BUSY);
	    }
		
		if(get_status_type(CAN_LITE_TX_BUSY) == CAN_LITE_TX_BUSY)
		{
			if(getDurationInUs(0, startTime, tickPerUs) > (txTimeOut * 1000))
			{
				ithCANReset(&can1);
				set_status_type(CAN_LITE_TX_TIMEOUT);
				clear_status_type(CAN_LITE_TX_BUSY);
			}
		}

	    //Error interrupt:the border of error warning limit(EWL) has been crossed by RECNT or TECNT or BUSOFF bit has bit changed
	    if (flagstatus & CAN_EIF)///Error Interrupt Flag
	    {
	        ithCANClearIntrFlag(&can1, Error_Interrupt_Flag);
	    }
		
		//Abort Interrupt Flag: After setting TPA or TSA, the messages have been aborted.
	    if (flagstatus & CAN_AIF)//Abort Interrupt Flag
	    {
	        ithCANClearIntrFlag(&can1, Abort_Interrupt_Flag);
	    }	
	}

}

int main(int argc, char **argv)
{
    int inputCmd = 0;
	CAN_BTATTR sb_attr = {0};
	CAN_BTATTR fb_attr = {0};
	ARMLITE_TX_OBJ* txobj = NULL;
	ARMLITE_CAN_INFO info = {0};
	unsigned int infoaddr = 0;
	//start Timer
	startTimer(0);

	tickPerUs = ARMLite_GetBusClock() / (1000 * 1000);

    while(1)
    {
        inputCmd = ARMLITE_COMMAND_REG_READ(REQUEST_CMD_REG);
        if (inputCmd && ARMLITE_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)
        {
            switch(inputCmd)
            {
            	case CAN_LITE_OPEN_ID:

					//init ring buffer info
					memset(&armlite_rb, 0, sizeof(ARMLITE_RING_BUFFER));
					//clear write ptr
					ARMLITE_COMMAND_REG_WRITE(CAN_LITE_RX_WR_PTR_REG, 0x0);
					//clear status
					clear_all_status_type();
					
					//init can driver
					can1.Instance				 = 1;
					can1.ADDR					 = CAN1_BASE_ADDRESS;
					can1.BaudRate				 = CAN_500K_2M;
					can1.SourceClock			 = CAN_SRCCLK_80M;
					can1.ProtocolType			 = protocol_FD_ISO;
					can1.ExternalLoopBackMode	 = false;
					can1.InternalLoopBackMode	 = false;
					can1.ListenOnlyMode		     = false;
					can1.TPtr					 = CAN1FilterTable;
					can1.SlowBitRate             = sb_attr;
					can1.FastBitRate             = fb_attr;
					can1.InterruptTable		     = (Receive_INT | RB_Full_INT | TP_INT | Error_INT);
					can1.InterruptHD			 = NULL;
					ithCANOpen(&can1);
					//change status
					armlite_status = CANBUS_OPEN;
					
					break;
					
				case CAN_LITE_CLOSED_ID:
					
					ithCANClose(&can1);
					//change status
					armlite_status = CANBUS_CLOSE;
					
					break;

				case CAN_LITE_READ_ID:
					
					ringbuf_update_info();
					
					break;
				case CAN_LITE_WRITE_ID:
					#ifdef CFG_CPU_WB
					ithInvalidateDCacheRange((void*)CAN_TX_BUFFER_OFFSET, sizeof(ARMLITE_TX_OBJ));
					#endif

					clear_status_type(CAN_LITE_TX_FIFO_FULL);
					clear_status_type(CAN_LITE_TX_TIMEOUT);
					set_status_type(CAN_LITE_TX_BUSY);
					
					txobj = (ARMLITE_TX_OBJ*)CAN_TX_BUFFER_OFFSET;
					if(ithCANWrite(&can1, &txobj->txObj, &txobj->data[0]) != 0)
					{
						set_status_type(CAN_LITE_TX_FIFO_FULL);
						clear_status_type(CAN_LITE_TX_BUSY);
					}

					txTimeOut = (unsigned long)txobj->txObj.Timeout;
					startTime = getCurTimer(0);
					
					break;

				case CAN_LITE_GETINFO_ID:
					info.ErrorStatus = ithCANGetErrorState(&can1);
					info.KindOfError = ithCANGetKOER(&can1);
					info.TEC = ithCANGetTEC(&can1);
					info.REC = ithCANGetREC(&can1);
					infoaddr = CAN_TX_BUFFER_OFFSET + sizeof(ARMLITE_TX_OBJ);
					memcpy((void*)infoaddr, (void*)&info, sizeof(ARMLITE_CAN_INFO));
					#ifdef CFG_CPU_WB
				    ithFlushDCacheRange((void*)infoaddr, sizeof(ARMLITE_CAN_INFO));
				    ithFlushMemBuffer();
					#endif
					break;
                default:
                    break;
            }
            ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, (unsigned int) inputCmd);
        }

		PollingInterruptFlag();
    }
    
    return 0;
}
