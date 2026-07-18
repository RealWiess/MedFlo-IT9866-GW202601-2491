#ifndef __ISO_TP__
#define __ISO_TP__
#include <stdint.h>
#include "obd2stack.h"
#if 0
extern uint32_t g_Ide;
#endif
/*function Prototype*/

int frame_single_frame_packet(void);
int frame_first_frame_packet(void);
int frame_single_wait_packet(void);
int frame_consecutive_frame_packet(void);
int CAN_send_wait_request(CAN_resp_wait_pkt *CAN_response_wait_pack);
int CAN_send_consecutive_frame(uint8_t *flow_ctl_req);
#endif
