#ifndef IR_H
#define IR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "openrtos/stream_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IR_JUDGE_PORT(port) ((port - ITH_IR0) >> 20 & 3)

#if defined(CFG_IR0_RX_MOD) || defined(CFG_IR1_RX_MOD)\
	|| defined(CFG_IR2_RX_MOD) || defined(CFG_IR3_RX_MOD)
#define ENABLE_IR_RX_MOD
#endif

#if defined(CFG_IR0_TX_MOD) || defined(CFG_IR1_TX_MOD)\
	|| defined(CFG_IR2_TX_MOD) || defined(CFG_IR3_TX_MOD)
#define ENABLE_IR_TX_MOD
#endif

#if defined(CFG_IR0_RX_SAMPLE) || defined(CFG_IR1_RX_SAMPLE)\
	|| defined(CFG_IR2_RX_SAMPLE) || defined(CFG_IR3_RX_SAMPLE)
#define ENABLE_IR_RX_SAMPLE
#endif

#ifdef CFG_GPIO_IR0_RX
#define IR0_TX CFG_GPIO_IR0_TX
#define IR0_RX CFG_GPIO_IR0_RX
#else
#define IR0_TX -1
#define IR0_RX -1
#endif

#ifdef CFG_IR0_RX_MOD
#define IR0_RX_MOD 1
#else
#define IR0_RX_MOD 0
#endif

#ifdef CFG_IR0_TX_MOD
#define IR0_TX_MOD 1
#else
#define IR0_TX_MOD 0
#endif

#ifdef CFG_IR0_RX_SAMPLE
#define IR0_RX_SAMPLE 1
#else
#define IR0_RX_SAMPLE 0
#endif

#ifdef CFG_GPIO_IR1_RX
#define IR1_TX CFG_GPIO_IR1_TX
#define IR1_RX CFG_GPIO_IR1_RX
#else
#define IR1_TX -1
#define IR1_RX -1
#endif

#ifdef CFG_IR1_RX_MOD
#define IR1_RX_MOD 1
#else
#define IR1_RX_MOD 0
#endif

#ifdef CFG_IR1_TX_MOD
#define IR1_TX_MOD 1
#else
#define IR1_TX_MOD 0
#endif

#ifdef CFG_IR1_RX_SAMPLE
#define IR1_RX_SAMPLE 1
#else
#define IR1_RX_SAMPLE 0
#endif

#ifdef CFG_GPIO_IR2_RX
#define IR2_TX CFG_GPIO_IR2_TX
#define IR2_RX CFG_GPIO_IR2_RX
#else
#define IR2_TX -1
#define IR2_RX -1
#endif

#ifdef CFG_IR2_RX_MOD
#define IR2_RX_MOD 1
#else
#define IR2_RX_MOD 0
#endif

#ifdef CFG_IR2_TX_MOD
#define IR2_TX_MOD 1
#else
#define IR2_TX_MOD 0
#endif

#ifdef CFG_IR2_RX_SAMPLE
#define IR2_RX_SAMPLE 1
#else
#define IR2_RX_SAMPLE 0
#endif

#ifdef CFG_GPIO_IR3_RX
#define IR3_TX CFG_GPIO_IR3_TX
#define IR3_RX CFG_GPIO_IR3_RX
#else
#define IR3_TX -1
#define IR3_RX -1
#endif

#ifdef CFG_IR3_RX_MOD
#define IR3_RX_MOD 1
#else
#define IR3_RX_MOD 0
#endif

#ifdef CFG_IR3_TX_MOD
#define IR3_TX_MOD 1
#else
#define IR3_TX_MOD 0
#endif

#ifdef CFG_IR3_RX_SAMPLE
#define IR3_RX_SAMPLE 1
#else
#define IR3_RX_SAMPLE 0
#endif

#define IR_DMA_BUFFER_SIZE      1024
#define IR_FIFO_LIMIT           64

#define STR_BUF_LEN             256
#define STATE_NUM               (10)
#define THRESHOLD_NUM			(7)
#define MAX_VAL                 ((1<<14)-1)

#define PRECISION               500000	// 2 micro second in a period

#define INIT_IR_OBJECT(port) \
	{\
	ITH_IR##port, \
	ITH_INTR_IR##port##RX, \
	IR##port##_TX, \
	IR##port##_RX, \
	PRECISION, \
	IR##port##_RX_MOD, \
	IR##port##_TX_MOD, \
	IR##port##_RX_SAMPLE, \
	NULL, \
	"dma_ir"#port"_write", \
	ITH_DMA_IR_CAP##port##_TX, \
	0, \
	0, \
	NULL, \
	NULL, \
	}

typedef void(*IntrHandler)(void*);

typedef struct _IR_OBJ IR_OBJ;

typedef struct _IR_OBJ {
	ITHIrPort port;
	ITHIntr	RxIntr;                 // Rx interrupt number
	int TxGpio;
	int RxGpio;
	int precision;
	_Bool irRxMod;
	_Bool irTxMod;
	_Bool irRxSample;
	IntrHandler intrHandler;
	/* DMA definaton start */
	char *TxChName;                 // Tx DMA channel name
	int TxChannelReq;
	int TxChannel;
	_Bool RxBufFull;
	/* DMA definaton end */
	uint32_t *TxBuffer;
	//QueueHandle_t RxQueue;
	StreamBufferHandle_t	xRxStrBuf;
} IR_OBJ;

/**
* @brief Get IR fatest frequency recorded since IR sampling function started.
*
* @param port IR device port.
*
* @return int fatest frequency.
*
*/
int iteIrGetFreqFast(ITHIrPort port);

/**
* @brief Get IR slowest frequency recorded since IR sampling function started.
*
* @param port IR device port.
*
* @return int slowest frequency.
*
*/
int iteIrGetFreqSlow(ITHIrPort port);

/**
* @brief Get IR average frequency recorded since IR sampling function started.
*
* @param port IR device port.
*
* @return int average frequency.
*
*/
int iteIrGetFreqAvg(ITHIrPort port);

/**
* @brief Get IR newest frequency recorded since IR sampling function started.
*
* @param port IR device port.
*
* @return int newest frequency.
*
*/
int iteIrGetFreqNew(ITHIrPort port);

/**
* @brief Get the fastest IR waveform in high level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the fastest IR waveform in high level duty cycle.
*
*/
int iteIrGetHighDCFast(ITHIrPort port);

/**
* @brief Get the fastest IR waveform in low level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the fastest IR waveform in low level duty cycle.
*
*/
int iteIrGetLowDCFast(ITHIrPort port);

/**
* @brief Get the slowest IR waveform in high level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the slowest IR waveform in high level duty cycle.
*
*/
int iteIrGetHighDCSlow(ITHIrPort port);

/**
* @brief Get the slowest IR waveform in low level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the slowest IR waveform in low level duty cycle.
*
*/
int iteIrGetLowDCSlow(ITHIrPort port);

/**
* @brief Get average IR waveform in high level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int average IR waveform in high level duty cycle.
*
*/
int iteIrGetHighDCAvg(ITHIrPort port);

/**
* @brief Get average IR waveform in low level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int average IR waveform in low level duty cycle.
*
*/
int iteIrGetLowDCAvg(ITHIrPort port);

/**
* @brief Get the newest IR waveform in high level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the newest IR waveform in high level duty cycle.
*
*/
int iteIrGetHighDCNew(ITHIrPort port);

/**
* @brief Get the fastest IR waveform in low level duty cycle sampled by chip since IR sampling function started.
*
* @param port IR device port.
*
* @return int the newest IR waveform in low level duty cycle.
*
*/
int iteIrGetLowDCNew(ITHIrPort port);

/**
* @brief Clear sampled data and restart sampling.
* If ITH_IR_RX_MOD_FILTER_RST_BIT is '1', register won't sample any data.
* If ITH_IR_RX_MOD_FILTER_RST_BIT is '0', register will start sample data.
*
* @param port  IR device port.
*
*/
void iteIrClearClkSample(ITHIrPort port);

/**
* @brief Initialize IR port. All detail are set in Kconfig setting time.
*
* @param port IR device port.
*
*/
void iteIrInit(ITHIrPort port, IR_OBJ* pIrObj);

/**
* @brief Fetch data from receive queue.
*
* @param port IR device port.
*
* @return size of ITPKeypadEvent. 
*
*/
int iteIrRead(ITHIrPort port, uint32_t* prt, int len);

/**
* @brief Deliver data to transmit queue.
*
* @param port IR device port.
*
* @return 0.
*
*/
int iteIrWrite(ITHIrPort port, uint32_t* prt, int len);

/**
* @brief Deinitialize IR port. All detail are set in Kconfig setting time.
*
* @return ITHIrPort IR device port.
*
*/
void iteIrDeinit(ITHIrPort port);

#ifdef __cplusplus
}
#endif

#endif
