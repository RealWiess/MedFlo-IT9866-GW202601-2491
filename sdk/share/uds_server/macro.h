#ifndef __CAN_STACK__MACRO__
#define __CAN_STACK__MACRO__
#include <stdint.h>

#ifndef PRINTF
#define PRINTF printf
//#define PRINTF
#endif

#define DTC_REQUEST_MODE 0x03
#define MODE1 0x41
#define MODE2 0x42
#define MODE3 0x43
#define MODE4 0x44
#define MODE5 0x45
#define MODE6 0x46
#define MODE7 0x47
#define MODE8 0x48
#define MODE9 0x49

#ifndef ZERO
#define ZERO 0
#endif

#define MODE_PID_COUNT 2
#if 0
#define CAN0 "can0"
#define CAN_INIT "ip link set can0 up type can bitrate"
#define CAN_DEINIT "ifconfig can0 down"
#endif
#define CAN_REQ_WRITE_BYTE 16
#define CAN_STANDARD_SRC_ID 0x7E0
#define CAN_STANDARD_DST_ID 0x7E8
#define CAN_EXTENDED_SRC_ID 0x18DA10F1	
#define CAN_EXTENDED_DST_ID 0x18DAF110 	
#define PCI_SF                0x00
#define PCI_FF                0x10
#define PCI_CF                0x20
#define PCI_FC                0x30

#ifndef N_OK
#define N_OK  0
#endif

#define NO_CAN_ID 0xFFFFFFFFU
#define SID 0x7DF
#define EID 0x98DB33F1
//#define DLC     64
#define DLC     8
#if 0
#define BAUD_RATE_250				250000
#define BAUD_RATE_500				500000
#endif
#define PID_OBD2_PID_LIST			0x00

#ifndef FAILURE
#define FAILURE				-1
#endif

#ifndef SUCCESS
#define SUCCESS 			 0
#endif

#define CAN_STACK_SOCKET_BIND_ERR 	-10
#define CAN_STACK_SOCKET_WRITE_ERR 	-11
#define CAN_STACK_SOCKET_READ_ERR 	-12
#define CAN_STACK_SOCKET_CREATE_ERR	-13
#define CAN_STACK_RESP_INVALID		-14
#define CAN_STACK_RQST_SEND_ERR	    -15
#define CAN_STACK_BAUDRATE_INIT_ERR	-16
#define CAN_STACK_INIT_ERR		    -17
#define CAN_STACK_DEINIT_ERR		-19
#define CAN_STACK_SOCKET_CLOSE_ERR	-20
#define CAN_STACK_INVALID_MODE_PID	-21
#define CAN_STACK_SOCKET_INDEX_ERR	-22
#define CAN_STACK_ARG_NULL_ERR		-23
#define CAN_STACK_RESP_RECV_ERR		-24
#define CAN_STACK_SEM_INIT_ERR      -25
#define CLOCK_GETTIME_ERR		    -26

#endif
