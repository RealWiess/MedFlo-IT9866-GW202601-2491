#ifndef ITP_ARPA_INET_H
#define ITP_ARPA_INET_H

#include <stdint.h>
#include "lwip/inet.h"
#include "lwip/sockets.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef CFG_NET_LWIP_2
typedef uint32_t in_addr_t;
#endif

const char  *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif // ITP_ARPA_INET_H