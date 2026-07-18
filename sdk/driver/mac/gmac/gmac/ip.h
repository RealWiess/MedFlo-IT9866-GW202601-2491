
#ifndef MAC_IP_H
#define MAC_IP_H

#include "skb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct iphdr {
	u8	reserved1[2];
	__be16	tot_len;
	u8	reserved2[5];
	u8	protocol;
	u16	check;
	__be32	saddr;
	__be32	daddr;
	/*The options start here. */
};

static inline struct iphdr *ip_hdr(const struct sk_buff *skb)
{
	return (struct iphdr *)(skb->data+ETH_HLEN);
}


#ifdef __cplusplus
}
#endif

#endif //MAC_IP_H
