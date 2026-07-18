#include <stdarg.h>
#include <string.h>

#include "alt_cpu/swUartLIN/swUartLIN.h"
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
    RX_BREAK_FIELD,
    RX_SYNC,
    RX_PID,
    RX_PROCESSING,
    RX_START,
    RX_STOP
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

typedef enum
{
    BREAK_FIELD_INIT = 0,
    BREAK_FIELD_TRANS,
    BREAK_FIELD_STOP,
    BREAK_FIELD_IDLE,
} BREAK_FIELD_STATE;

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

    //breakfield
    bool isbreakfield;
    uint8_t txBreakfieldbit;
    uint32_t breakfield_state;
    uint32_t rx_accumulated_zero_bits;
    bool rx_breakfield_dectect;
} SWUART_HANDLE;

static SWUART_HANDLE gptSwUartHandle = { 0 };
uint8_t *pDebugAddr = (uint8_t*)(16 * 1024 - 10);
uint8_t *pDebugAddr2 = (uint8_t*)(16 * 1024 - 14);
uint32_t *pDebugAddr3 = (uint32_t*)(16 * 1024 - 4);

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
        ptHandle->txWriteIdx = ptHandle->txWriteLen; //txWriteIdx相當於最後面的idx

        ptHandle->isbreakfield = pWriteData->isbreakfield;//決定是否為breakfield
        if(ptHandle->isbreakfield){
            ptHandle->breakfield_state = BREAK_FIELD_INIT;
            ptHandle->txBreakfieldbit = 0;
        }
        
    }
}

static void swUartProcessReadCmd(void)
{
	SW_UART_READ_DATA* pReadData = (SW_UART_READ_DATA*) CMD_DATA_BUFFER_OFFSET;
	int remainSize = 0;
	int cpySize = 0;
	int tailSize = 0;
    SWUART_HANDLE *ptHandle = &gptSwUartHandle;

    if(ptHandle->rx_breakfield_dectect == true){
        memcpy(&(pReadData->rx_breakfield_dectect), &(ptHandle->rx_breakfield_dectect), sizeof(bool));
        ptHandle->rx_breakfield_dectect = false;
    }

    //這邊有可能有問題，若是一開始一直write進來直到他=rxReadIdx，
    //他其實是滿的，但會被判為沒有data。
	// SWUART_HANDLE *ptHandle = &gptSwUartHandle;
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

    //要讀的資料筆數如果小於目前data內有的,則讀取要讀得比數,
    //否則讀取目前data內有的總筆數。
    //remainSize代表uart中目前已經讀了幾筆資料在buffer內。
    //cpysize代表我們最後要從裡面讀幾筆資料出來。
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
    //rxReadIdx是指目前ptHandle->pReadData內以讀取到到的index,
    //而rxWriteIdx是從外部讀取進來的資料目前放到哪個idx了，
    //所以按理說rxWriteIdx應該要大於rxReadIdx，但因為是ring cycle，

    // The return Rx's Length
    pReadData->len = ENDIAN_SWAP32(cpySize);
}

static void swUartProcessInitCmd(void)
{
	SW_UART_INIT_DATA* pInitData = (SW_UART_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
	uint32_t cpuClk = ENDIAN_SWAP32(pInitData->cpuClock);
	uint32_t baudrate = ENDIAN_SWAP32(pInitData->baudrate);
	SWUART_HANDLE *ptHandle = &gptSwUartHandle;

    ptHandle->isbreakfield = 0;
    ptHandle->tickPerbit = (cpuClk / baudrate); //根據cpuclk和baudrate來計算出一個bit要幾個clk
    ptHandle->txGpio = ENDIAN_SWAP32(pInitData->uartTxGpio);
    ptHandle->rxGpio = ENDIAN_SWAP32(pInitData->uartRxGpio);
    ptHandle->parity = ENDIAN_SWAP32(pInitData->parity);

    // (*pDebugAddr) = 100;
    // (*pDebugAddr2) = ENDIAN_SWAP32(ptHandle->tickPerbit);
    // (*pDebugAddr3) = ENDIAN_SWAP32(baudrate);

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

    setGpioMode(41, 0);
    setGpioDir(41, 0);  //output mode
    setGpioValue(41, 1);

    setGpioMode(42, 0);
    setGpioDir(42, 0);  //output mode
    setGpioValue(42, 1);
}


static void swUartMonitorRx(void){
    SWUART_HANDLE *ptHandle = &gptSwUartHandle;
    uint8_t CurReadBit = getGpioValue(ptHandle->rxGpio, 1);
    // setGpioValue(41,CurReadBit);
    

    switch(ptHandle->rxState){
        case RX_IDLE:
            if((CurReadBit == 0) && (CurReadBit != ptHandle->rxPreReadBit)){
                ptHandle->rxChkTime = getCurTimer(0);
                ptHandle->rxNextReadTime = (ptHandle->tickPerbit / 3);//中間取樣
                ptHandle->readByte = 0x0;
                ptHandle->rxBitIdx = 0;
                ptHandle->rxState = RX_START;
            }
            break;
        case RX_START:
            if(getDuration(0, ptHandle->rxChkTime) >= ptHandle->rxNextReadTime){
                /*1bit 0*/
                ptHandle->rxNextReadTime += ptHandle->tickPerbit;
                ptHandle->rxState = RX_PROCESSING;
                setGpioValue(41, CurReadBit);
            }
            break;
        case RX_PROCESSING:
            if(getDuration(0, ptHandle->rxChkTime) >= ptHandle->rxNextReadTime){
                setGpioValue(41,CurReadBit);

                if(CurReadBit == 0){
                    ptHandle->rx_accumulated_zero_bits++;
                }

                if(ptHandle->rx_accumulated_zero_bits >= 13){
                    // >=13其實才對 因為他代表第14個bit的值，
                    // 如果寫>=14會把之後的1給拿走導致錯誤。
                    ptHandle->rx_accumulated_zero_bits = 0;
                    ptHandle->rxState = RX_STOP;
                    ptHandle->rx_breakfield_dectect = true;
                }else{
                    if(ptHandle->rxBitIdx < 8){
                        ptHandle->readByte |= (CurReadBit << ptHandle->rxBitIdx);
                        ptHandle->rxBitIdx++;
                    }else{
                        if(CurReadBit == 1 ){
                            /*finish 1 byte data*/
                            ptHandle->rx_accumulated_zero_bits = 0;
                            ptHandle->rxState = RX_IDLE;
                            /*
                            不能跳到RX_STOP, 因為要進入那邊的if的話，代表下一個bit已經開始，(下一筆data的start_bit)
                            也就是rxPreReadBit會變為0，之後在跳到RX_IDLE的話會發生錯誤，
                            因為rxPreReadBit和current bit都是0，所以進不去，造成整個data出錯。
                            */

                            ptHandle->pReadData[ptHandle->rxWriteIdx] = ptHandle->readByte;
                            ptHandle->rxWriteIdx++;
                            if(ptHandle->rxWriteIdx >= MAX_UART_BUFFER_SIZE) {
                                ptHandle->rxWriteIdx -= MAX_UART_BUFFER_SIZE;
                            }
                            *pDebugAddr = *pDebugAddr + 1;
                        }
                    }
                }

                ptHandle->rxNextReadTime += ptHandle->tickPerbit;
            }
            break;
        case RX_STOP:
            if(getDuration(0, ptHandle->rxChkTime) >= ptHandle->rxNextReadTime){
                setGpioValue(41,CurReadBit);
                if(CurReadBit == 1){
                    ptHandle->rxState = RX_IDLE;
                }
            }
            break;
        default : 
            *pDebugAddr3 = ENDIAN_SWAP32(104);
            break;
    }

    ptHandle->rxPreReadBit = CurReadBit;
}

static void swUartMonitorTx(void)
{
    SWUART_HANDLE *ptHandle = &gptSwUartHandle;

    if(ptHandle->txInit == 0) return;

    if(ptHandle->isbreakfield == true){
        // setGpioValue(ptHandle->txGpio, 0);
        switch (ptHandle->breakfield_state)
        {
            case BREAK_FIELD_INIT:
                /* code */
                ptHandle->txChkTime = getCurTimer(0);
                ptHandle->txNextWriteTime = ptHandle->tickPerbit;
                ptHandle->breakfield_state = BREAK_FIELD_TRANS;
                setGpioValue(ptHandle->txGpio, 0);
                setGpioValue(42, 0);
                break;
            case BREAK_FIELD_TRANS: 
                if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime){
                    ptHandle->txBreakfieldbit++;
                    ptHandle->txNextWriteTime += ptHandle->tickPerbit;
                }

                if(ptHandle->txBreakfieldbit >= 14){
                    setGpioValue(ptHandle->txGpio, 1);
                    setGpioValue(42, 1);
                    ptHandle->breakfield_state = BREAK_FIELD_STOP;
                }
                break;
            case BREAK_FIELD_STOP:
                //送完13個bit0之後,至少要維持在高位1bit
                if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime){
                    ptHandle->txReadIdx = ptHandle->txWriteIdx;
                    ptHandle->isbreakfield = false;
                    ptHandle->txBreakfieldbit = 0;
                    ptHandle->breakfield_state = BREAK_FIELD_IDLE;
                }
                break;
            case BREAK_FIELD_IDLE:
                /*do nothing*/
                break;
            default:
                break;
        }
    }else{
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
                setGpioValue(42, 0);
                break;
            case TX_DATA_BITS: //data bits
                if(getDuration(0, ptHandle->txChkTime) >= ptHandle->txNextWriteTime)
                {
                    if(ptHandle->writeByte & (0x1 << ptHandle->txBitIdx)) {
                        setGpioValue(ptHandle->txGpio, 1);
                        setGpioValue(42, 1);
                        ptHandle->parity_num++;
                    }
                    else {
                        setGpioValue(ptHandle->txGpio, 0);
                        setGpioValue(42, 0);
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
                if(ptHandle->parity == NONE ){
                    ptHandle->parity_num = 0;
                    ptHandle->txState = TX_STOP_BIT;
                    break;
                }

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
                    setGpioValue(42, 1);
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

}

int main(int argc, char **argv)
{
    memset(&gptSwUartHandle, 0X0, sizeof(gptSwUartHandle));
    startTimer(0);

    (*pDebugAddr) = 0;
    (*pDebugAddr2) = 0;
    (*pDebugAddr3) = 0;

    while(1)
    {
        int inputCmd = ALT_CPU_COMMAND_REG_READ(REQUEST_CMD_REG); //這邊處理由driver送過來的command
        if (inputCmd && ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)// ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0確保上一個命令有完成
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
