#ifndef __PING_H__
#define __PING_H__

/**
 * PING_USE_SOCKETS: Set to 1 to use sockets, otherwise the raw api is used
 */
//#ifndef PING_USE_SOCKETS
//#define PING_USE_SOCKETS    LWIP_SOCKET
//#endif

#define PING_USE_SOCKETS 1
#define PING_USE_RAW     0

#define PING_PRINTF

void ping_init(void);
void ping_raw_init(u32_t time);


#if PING_USE_RAW
void ping_send_now(u32_t time);
#endif /* !PING_USE_RAW */

void ping_set_target(const char* ip);
void ping_set_name(const char* name);
extern void (*ping_result)(int ping_ok);
void ping_set_receive_timeout(int ms);
void ping_set_delay(int ms);

#endif /* __PING_H__ */
