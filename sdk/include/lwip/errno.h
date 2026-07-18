# ifdef CFG_NET_LWIP_2

#include "lwip_2/errno.h"

# else
#include <sys/errno.h>
# endif /* For LWIP 2.1.2 only */