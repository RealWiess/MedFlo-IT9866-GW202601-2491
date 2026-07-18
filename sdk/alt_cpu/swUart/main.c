#include <stdarg.h>
#include <string.h>

#include "alt_cpu/swUart/swUart.h"
#include "alt_cpu/alt_cpu_utility.h"

#define ENDIAN_SWAP16(x) \
        (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

#define ENDIAN_SWAP32(x) \
        (((x & 0x000000FF) << 24) | \
        ((x & 0x0000FF00) <<  8) | \
        ((x & 0x00FF0000) >>  8) | \
        ((x & 0xFF000000) >> 24))

typedef enum
{
    RX_IDLE = 0,
    RX_PROCESSING,
} MONITOR_RX_STATE;

typedef enum
{
    TX_IDLE = 0,
    TX_START_BIT = TX_IDLE,
    TX_DATA_BITS,
    TX_PARITY_BIT,
    TX_STOP_BIT,
    TX_STOP_END,
} MONITOR_TX_STATE;

typedef struct
{
    uint32_t txState;
    uint32_t rxState;
    uint32_t txInit;
    uint32_t rxInit;
    uint32_t txGpio;
    uint32_t rxGpio;
    // uint32_t txEnableGpio;
    uint32_t txWriteLen;
    uint32_t txWriteIdx;
    uint32_t txReadIdx;
    uint32_t rxReadLen;
    uint32_t rxWriteIdx;
    uint32_t rxReadIdx;
    uint32_t tickPerbit;
    uint32_t rxChkTime;
    uint32_t rxNextReadTime;
    uint32_t txChkTime;
    uint32_t txNextWriteTime;

    // uint8_t mode;
    uint8_t parity;
    uint8_t readByte;
    uint8_t rxBitIdx;
    uint8_t rxPreReadBit;
    uint8_t writeByte;
    uint8_t txBitIdx;
    uint8_t parity_num;

    uint8_t pWriteData[MAX_UART_BUFFER_SIZE];
    uint8_t pReadData[MAX_UART_BUFFER_SIZE];
} SWUART_HANDLE;

static SWUART_HANDLE gptSwUartHandle = { 0 };

static void swUartProcessWriteCmd(void)
{
	SW_UART_WRITE_DATA* pWriteData = (SW_UART_WRITE_DATA*) CMD_DATA_BUFFER_OFFSET;
	SWUART_HANDLE *ptHandle = &gptSwUartHandle;
    if(ptHandle->txInit == 0) {
        // Tx un-init
        return;
    }
    else if(ptHandle->txWriteIdx != ptHandle->txReadIdx)
    {
        pWriteData->len = 0;
        return;
    }
    else
    {
        ptHandle->txWriteLen = ENDIAN_SWAP32(pWriteData->len);
        memcpy(ptHandle->pWriteData, pWriteData->pWriteBuffer, ptHandle->txWriteLen);
        ptHandle->txReadIdx = 0;
        ptHandle->txWriteIdx = ptHandle->txWriteLen;
    }
}

static void swUartProcessReadCmd(void)
{
	SW_UART_READ_DATA* pReadData = (SW_UART_READ_DATA*) CMD_DATA_BUFFER_OFFSET;
	int remainSize = 0;
	int cpySize = 0;
	int tailSize = 0;

	SWUART_HANDLE *ptHandle = &gptSwUartHandle;
    if(ptHandle->rxWriteIdx == ptHandle->rxReadIdx) {
        // No Rx Data
        pReadData->len = 0;
        return;
    }

    if(ptHandle->rxInit == 0) {
        // Rx un-init
        pReadData->len = 0;
        return;
    }

    ptHandle->rxReadLen = ENDIAN_SWAP32(pReadData->len);

    if(ptHandle->rxWriteIdx > ptHandle->rxReadIdx)
        remainSize = ptHandle->rxWriteIdx - ptHandle->rxReadIdx;
    else
        remainSize = MAX_UART_BUFFER_SIZE - ptHandle->rxReadIdx + ptHandle->rxWriteIdx;

    if(ptHandle->rxReadLen <= remainSize)
        cpySize = ptHandle->rxReadLen;
    else
        cpySize = remainSize;

    tailSize = MAX_UART_BUFFER_SIZE - ptHandle->rxReadIdx;
    if(cpySize <= tailSize) {
        memcpy(pReadData->pReadBuffer, &ptHandle->pReadData[ptHandle->rxReadIdx], cpySize);
    }
    else {
        memcpy(pReadData->pReadBuffer, &ptHandle->pReadData[ptHandle->rxReadIdx], tailSize);
        memcpy(&pReadData->pReadBuffer[tailSize], &ptHandle->pReadData[0], cpySize - tailSize);
    }

    ptHandle->rxReadIdx += cpySize;
    if(ptHandle->rxReadIdx >= MAX_UART_BUFFER_SIZE) {
        ptHandle->rxReadIdx -= MAX_UART_BUFFER_SIZE;
    }

    // The return Rx's Length
    pReadData->len = ENDIAN_SWAP32(cpySize);
}

static void swUartProcessInitCmd(void)
{
	SW_UART_INIT_DATA* pInitData = (SW_UART_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
	uint32_t cpuClk = ENDIAN_SWAP32(pInitData->cpuClock);
	uint32_t baudrate = ENDIAN_SWAP32(pInitData->baudrate);
	SWUART_HANDLE *ptHandle = &gptSwUartHandle;

    ptHandle->tickPerbit = (cpuClk / baudrate);
    ptHandle->txGpio = ENDIAN_SWAP32(pInitData->uartTxGpio);
    ptHandle->rxGpio = ENDIAN_SWAP32(pInitData->uartRxGpio);
    ptHandle->parity = ENDIAN_SWAP32(pInitData->parity);

	//Tx GPIO init
    if(ptHandle->txGpio) {
        setGpioMode(ptHandle->txGpio, 0);
        setGpioDir(ptHandle->txGpio, 0);  //output mode
        setGpioValue(ptHandle->txGpio, 1);
        ptHandle->txInit = 1;
    }

    //Rx GPIO init
    if(ptHandle->rxGpio) {
        setGpioMode(ptHandle->rxGpio, 0);
        setGpioDir(ptHandle->rxGpio, 1); //input mode
        ptHandle->rxInit = 1;
    }
}

static void swUartMonitorRx(void)
{
	uint8_t CurReadBit = 0;
    SWUART_HANDLE *ptHandle = &gptSwUartHandle;
    
    if(ptHandle->rxInit == 0) return;
    
    if(ptHandle->rxState == RX_IDLE) {
        CurReadBit = getGpioValue(ptHandle->rxGpio, 1);  //Get data from Rx GPIO
        if(CurReadBit == 0 && ptHandle->rxPreReadBit != CurReadBit)
        {
            ptHandle->rxChkTime = getCurTimer(0);
            ptHandle->rxNextReadTime = ptHandle->tickPerbit;
            ptHandle->readByte = 0x0;
            ptHandle->rxState = RX_PROCESSING;
        }
	}
    else if(getDuration(0, ptHandle->rxChkTime) >= ptHandle->rxNextReadTime && ptHandle->rxState == RX_PROCESSING) {		
        CurReadBit = getGpioValue(ptHandle->rxGpio, 1);  //Get data from Rx GPIO
        if(ptHandle->rxBitIdx < 8) {			
			ptHandle->readByte |= (CurReadBit << ptHandle->rxBitIdx);	
			ptHandle->rxBitIdx++;
		}
		else{
			// Receive 1 Byte done
			ptHandle->rxBitIdx = 0;
            ptHandle->rxState = RX_IDLE;
			ptHandle->pReadData[ptHandle->rxWriteIdx] = ptHandle->readByte;
			ptHandle->rxWriteIdx++;
            if(ptHandle->rxWriteIdx >= MAX_UART_BUFFER_SIZE) {
	        	ptHandle->rxWriteIdx -= MAX_UART_BUFFER_SIZE;
        	}
		}
        ptHandle->rxNextReadTime += ptHandle->tickPerbit;
	}
	ptHandle->rxPreReadBit = CurReadBit;
}

static void swUartMonitorTx(void)
{
    SWUART_HANDLE *ptHandle = &gptSwUartHandle;

    if(ptHandle->txInit == 0) return;

    switch (ptHandle->txState)
    {
        case TX_START_BIT: //start bit
            if(ptHandle->txWriteIdx == ptHandle->txReadIdx)
            {
                // No Tx Data
                return;        
            }

            ptHandle->txChkTime = getCurTimer(0);
            ptHandle->txNextWriteTime = ptHandle->tickPerbit;
            ptHandle->writeByte = ptHandle->pWriteData[ptHandle->txReadIdx];
            ptHandle->txState = TX_DATA_BITS;
            setGpioValue(ptHandle->txGpio, 0);
            break;
        case TX_DATA_BITS: //data bits
            if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime)
            {
                if(ptHandle->writeByte & (0x1 << ptHandle->txBitIdx)) {
                    setGpioValue(ptHandle->txGpio, 1);
                    ptHandle->parity_num++;
                }
                else {
                    setGpioValue(ptHandle->txGpio, 0);
                }
                ptHandle->txBitIdx++;
                if(ptHandle->txBitIdx == 8)
                {
                    ptHandle->txBitIdx = 0;
                    ptHandle->txState = TX_PARITY_BIT;
                }
                ptHandle->txNextWriteTime += ptHandle->tickPerbit;
            }
            break;
        case TX_PARITY_BIT: //parity bit
            if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime)
            {
                switch(ptHandle->parity) {
                    case EVEN:
                        if(ptHandle->parity_num % 2) {
                            setGpioValue(ptHandle->txGpio, 1);
                        }
                        else {
                            setGpioValue(ptHandle->txGpio, 0);
                        }
                        break;
                    case ODD:
                        if(ptHandle->parity_num % 2) {
                            setGpioValue(ptHandle->txGpio, 0);
                        }
                        else {
                            setGpioValue(ptHandle->txGpio, 1);
                        }
                        break;
                    case NONE:
                        setGpioValue(ptHandle->txGpio, 0);
                    default:
                        break;
                }
                ptHandle->parity_num = 0;
                ptHandle->txState = TX_STOP_BIT;
                ptHandle->txNextWriteTime += ptHandle->tickPerbit;
            }
            break;
        case TX_STOP_BIT: //stop bit
            if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime)
            {
                setGpioValue(ptHandle->txGpio, 1);
                ptHandle->txState = TX_STOP_END;
                ptHandle->txNextWriteTime += ptHandle->tickPerbit;
            }
            break;
        case TX_STOP_END:
            if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime)
            {
                ptHandle->txState = TX_IDLE;
                ptHandle->txReadIdx++;
            }
            break;
    }
}

int main(int argc, char **argv)
{
    memset(&gptSwUartHandle, 0X0, sizeof(gptSwUartHandle));
    startTimer(0);

    while(1)
    {
        int inputCmd = ALT_CPU_COMMAND_REG_READ(REQUEST_CMD_REG);
        if (inputCmd && ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)
        {
            switch(inputCmd)
            {
                case INIT_CMD_ID:
                    swUartProcessInitCmd();
                    break;
				case WRITE_DATA_CMD_ID:
					swUartProcessWriteCmd();
					break;
				case READ_DATA_CMD_ID:
					swUartProcessReadCmd();
					break;
                default:
                    break;
            }
            ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, (uint16_t) inputCmd);
        }
		swUartMonitorTx();
        swUartMonitorRx();
    }
}
