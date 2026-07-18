#include <string.h>
#include <stdlib.h>
#include "arm_lite_dev/can_bus_lite/can_bus_lite.h"


#define MTL_REMAP(x) (x - iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET))
#define LTM_REMAP(x) (x + iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET))


static uint8_t gpCAN1Image[] =
{
    #include "can_bus_lite.hex"
};

static unsigned char engine_opened = 0;

static void canbus1ProcessCommand(int cmdId)
{
    int i = 0;

    ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, cmdId);
    while(1)
    {
        usleep(1000);

        if (ARMLITE_COMMAND_REG_READ(RESPONSE_CMD_REG) != cmdId)
            continue;
        else
            break;
    }
    ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);

    for (i = 0; i < 1024; i++)
    {
        asm("");
    }

    ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);
}

static int canbus1Read(int file, char *ptr, int len, void* info)
{
	ARMLITE_RING_BUFFER rb = {0};
	unsigned int     wr_ptr_reg = 0;
	unsigned short   max_frame_count = 0;
	unsigned short   rev_frame_count = 0;
	unsigned char* 	 pCurRDADDR = 0;

	max_frame_count = (unsigned short)(len / sizeof(CAN_RXOBJ));

	wr_ptr_reg = ARMLITE_COMMAND_REG_READ(CAN_LITE_RX_WR_PTR_REG);
	rb.end = (unsigned short)(wr_ptr_reg & 0xFFFF);
	rb.count = (unsigned short)((wr_ptr_reg >> 16) & 0xFFFF);
	rb.start = ARMLITE_COMMAND_REG_READ(CAN_LITE_RX_RD_PTR_REG);

	if((rb.count > 0) || (rb.start != rb.end))
	{
		if(rb.count < max_frame_count)
			rev_frame_count = rb.count;
		else
			rev_frame_count = max_frame_count;


		pCurRDADDR = (unsigned char*)LTM_REMAP(CAN_RX_BUFFER_OFFSET + (rb.start * sizeof(CAN_RXOBJ)));
		#ifdef CFG_CPU_WB
		//Invalid D-Cache
		ithInvalidateDCacheRange((void*)pCurRDADDR, (rev_frame_count * sizeof(CAN_RXOBJ)));
		#endif		
		//copy data
		memcpy((void*)ptr, (void*)pCurRDADDR, (rev_frame_count * sizeof(CAN_RXOBJ)));
		//update read ptr
		rb.start = (rb.start + rev_frame_count) % RX_MAX_DATA_BUFFER;
		ARMLITE_COMMAND_REG_WRITE(CAN_LITE_RX_RD_PTR_REG, rb.start);
		//send command
		canbus1ProcessCommand(CAN_LITE_READ_ID);
	}

    return rev_frame_count;
}

static int canbus1Write(int file, char *ptr, int len, void* info)
{

	unsigned char* pTxBufferADDR = (unsigned char*)iteRiscGetTargetMemAddress(ARMLITE_CPU_SHARE_MEM0_TARGET);
	//copy data to memory
	memcpy((void*) pTxBufferADDR, (void*)ptr, sizeof(ARMLITE_TX_OBJ));
	
	//flush D-Cache data
	#ifdef CFG_CPU_WB
	ithFlushDCacheRange((void*) pTxBufferADDR, sizeof(ARMLITE_TX_OBJ));
	ithFlushMemBuffer();
	#endif
	//send command
	canbus1ProcessCommand(CAN_LITE_WRITE_ID);

}

static int canbus1Ioctl(int file, unsigned long request, void *ptr, void *info)
{
    unsigned char* pExchangeAddress = (unsigned char*) (iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET) + CAN_RX_BUFFER_OFFSET);
    switch (request)
    {
        case ITP_IOCTL_INIT:
        {
            //Stop CPU
            iteRiscResetCpu(ARMLITE_CPU);

            //Clear Commuication Engine and command buffer
            memset(pExchangeAddress, 0x0, RX_MAX_DATA_BUFFER_SIZE);
            ithFlushDCacheRange((void*)pExchangeAddress, RX_MAX_DATA_BUFFER_SIZE);
            ithFlushMemBuffer();
			
            ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
            ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);

            //Load Engine First
            iteRiscLoadData(ARMLITE_CPU_IMAGE_MEM_TARGET,gpCAN1Image,sizeof(gpCAN1Image));

            //Fire CPU
            iteRiscFireCpu(ARMLITE_CPU);

            break;
        }
		
		case ITP_IOCTL_ENABLE:
		{
			ARMLITE_COMMAND_REG_WRITE(CAN_LITE_RX_RD_PTR_REG, 0);
			canbus1ProcessCommand(CAN_LITE_OPEN_ID);
			engine_opened = 0x1;
			break;
		}
		
		case ITP_IOCTL_DISABLE:
		{
			canbus1ProcessCommand(CAN_LITE_CLOSED_ID);
			engine_opened = 0x0;
			break;
		}

		case ITP_IOCTL_IS_AVAIL:
		{
			if(engine_opened == 0x1)
				*(unsigned char*)ptr = 0x1;
			else
				*(unsigned char*)ptr = 0x0;
			
			break;
		}

		case ITP_IOCTL_GET_INFO:
		{
			canbus1ProcessCommand(CAN_LITE_GETINFO_ID);
			unsigned char* pErrorInfoADDR = (unsigned char*)(iteRiscGetTargetMemAddress(ARMLITE_CPU_SHARE_MEM0_TARGET) + sizeof(ARMLITE_TX_OBJ));
			#ifdef CFG_CPU_WB
			//Invalid D-Cache
			ithInvalidateDCacheRange((void*)pErrorInfoADDR, sizeof(ARMLITE_CAN_INFO));
			#endif			
			//copy data
			memcpy((void*)ptr, (void*)pErrorInfoADDR, sizeof(ARMLITE_CAN_INFO));

			
			break;
		}
		
        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceCAN1 =
{
    ":canbuslite",
    itpOpenDefault,
    itpCloseDefault,
    canbus1Read,
    canbus1Write,
    itpLseekDefault,
    canbus1Ioctl,
    NULL
};
