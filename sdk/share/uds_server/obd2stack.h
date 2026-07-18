#ifndef __CAN_STACK__
#define __CAN_STACK__
#if 0
#include <FreeRTOS.h>
#include <semphr.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifndef NO_CAN_ID
#define NO_CAN_ID        0xFFFFFFFFU
#endif

#ifndef ZERO
#define ZERO             0
#endif

#if 0
#define CANFD_ON         1
#endif

#define CAN_STACK_THREAD_INIT_ERR	-18
#if 0
#ifdef ENABLE_IOBD_DEBUG_LEVEL
#define IOBD_DEBUG_LEVEL(...)\
{\
        printf("DEBUG:   %s L#%d ", __func__, __LINE__);  \
        printf(__VA_ARGS__); \
        printf("\n"); \
}
#else
#define IOBD_DEBUG_LEVEL(...)
#endif

extern uint32_t g_Ide;
#endif
int frame_single_frame_packet(void);
int frame_first_frame_packet(void);
int frame_consecutive_frame_packet(void);
int CAN_send_consecutive_frame(uint8_t *flow_ctl_req);

typedef struct CAN_Transmit
{
	uint32_t id;
	uint32_t tx_dl;
	uint8_t data[64] __attribute__((aligned(8)));
}CAN_Tx;
/** Global structure to store filtered response*/
typedef struct g_filter
{
	uint8_t ar_data[4080];
	int length;
	int multi_frame_length;
	int total_length;
	uint32_t can_addr;
}g_filter;
/*multoframe addition*/
typedef struct tp_request
{
        uint8_t N_Mtype;
        uint32_t N_SA;
        uint32_t N_TA;
        uint8_t N_TAtype;
        uint8_t N_data[4080];
        int len_of_data;
        int  N_result;
}CAN_req_pkt;
#if 1
typedef struct resp_indication
{
        uint8_t N_Mtype;
        uint32_t N_SA;
        uint32_t N_TA;
        uint8_t N_TAtype;
        uint8_t N_data[4080];
        int len_of_data;
        int  N_result;
}CAN_resp_pkt;

typedef struct resp_indication_wait
{
        uint8_t N_Mtype;
        uint32_t N_SA;
        uint32_t N_TA;
        uint8_t N_TAtype;
        uint8_t N_data[4080];
        int len_of_data;
        int  N_result;
}CAN_resp_wait_pkt;
#endif
#if 0
int CAN_bps_init(int);
int CAN_deinit(void);
int CAN_check_id (uint32_t);
void CAN_pkt_frame(uint8_t,uint8_t);
void CAN_server_Response_Receive (void);
void CAN_flow_control_frame (CAN_Tx *);
void CAN_dtc_pkt_frame (void);
int CAN_request_dtc(void);
int CAN_decode_dtc_resp(uint8_t *,int,char *);
int CAN_id_validation(void);
int CAN_resp_valid(CAN_Tx *);
int CAN_test_id(uint32_t);
int check_vehicle_fuel_type(uint64_t *,int);
int check_vehicle_fuel_status(uint64_t *,int);
int CAN_decode_VIN(uint8_t *,int, char *, char *, char *);
int get_vehicle_details(void);
void find_vehicle_model_year(char, char *);
void find_vehicle_manufacturer(char *,char *);
void find_vehicle_origin_geo_area(char, char *);
int check_valid_mode_pid(uint16_t );
int CAN_init(void);
int start_can_receive_thread(void);
int CAN_request(uint16_t);
int Frame_the_packet(uint8_t service_id, uint8_t sub_fun);
int CAN_id_test(uint32_t ID);
int CAN_request_send(CAN_Tx *);
/*function Prototype*/
int CAN_send_request(CAN_resp_pkt *CAN_response_pack);
void CAN_id_configuration();
int CAN_resp_recv(CAN_resp_pkt *CAN_resp_data);
int CAN_send_consecutive_frame(uint8_t *flow_ctl_req);
#else
/*function Prototype*/
void CAN_server_Response_Receive(void);
int CAN_send_request(CAN_resp_pkt *CAN_response_pack);
int CAN_resp_recv(CAN_resp_pkt *CAN_resp_data);
int CAN_send_consecutive_frame(uint8_t *flow_ctl_req);
void CAN_flow_control_frame(CAN_Tx *);
#endif
#endif
