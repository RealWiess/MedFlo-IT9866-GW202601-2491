#ifndef _SYS_UIO_H
#define _SYS_UIO_H

#include <sys/types.h>
#ifdef CFG_NET_LWIP_2
#include "lwip/sockets.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per POSIX
 */

#ifndef CFG_NET_LWIP_2
struct iovec {
  void   *iov_base;
  size_t  iov_len;
};

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
#endif

#ifdef __cplusplus
};
#endif

#endif
