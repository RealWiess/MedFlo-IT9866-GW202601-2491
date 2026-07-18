#ifndef _AIC_SKBUFF_H_
#define _AIC_SKBUFF_H_

#include "compiler.h"

struct aic_sk_buff {
    unsigned int len;
    unsigned int max_len;
    unsigned char *tail;
    unsigned char *end;
    unsigned char *head;
    unsigned char *data;
};

static inline unsigned char *aic_skb_tail_pointer(const struct aic_sk_buff *skb)
{
	return skb->tail;
}

static inline void aic_skb_reset_tail_pointer(struct aic_sk_buff *skb)
{
	skb->tail = skb->data;
}

static inline void aic_skb_set_tail_pointer(struct aic_sk_buff *skb, const int offset)
{
	skb->tail = skb->data + offset;
}

/**
 *	skb_headroom - bytes at buffer head
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the head of an &sk_buff.
 */
static inline unsigned int aic_skb_headroom(const struct aic_sk_buff *skb)
{
	return skb->data - skb->head;
}

/**
 *	skb_tailroom - bytes at buffer end
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the tail of an sk_buff
 */
static inline int aic_skb_tailroom(const struct aic_sk_buff *skb)
{
#if 0
	return skb_is_nonlinear(skb) ? 0 : skb->end - skb->tail;
#else
    return skb->end - skb->tail;
#endif
}

#if 0
/**
 *	skb_availroom - bytes at buffer end
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the tail of an sk_buff
 *	allocated by sk_stream_alloc()
 */
static inline int skb_availroom(const struct sk_buff *skb)
{
	if (skb_is_nonlinear(skb))
		return 0;

	return skb->end - skb->tail - skb->reserved_tailroom;
}
#endif

/**
 *	skb_reserve - adjust headroom
 *	@skb: buffer to alter
 *	@len: bytes to move
 *
 *	Increase the headroom of an empty &sk_buff by reducing the tail
 *	room. This is only allowed for an empty buffer.
 */
static inline void aic_skb_reserve(struct aic_sk_buff *skb, int len)
{
	skb->data += len;
	skb->tail += len;
}


struct aic_sk_buff *aic_dev_alloc_skb(unsigned int length);
void aic_dev_kfree_skb_any(struct aic_sk_buff *skb);
void aic_skb_reset(struct aic_sk_buff *skb);
void *aic_skb_put(struct aic_sk_buff *skb, unsigned int len);
void *aic_skb_push(struct aic_sk_buff *skb, unsigned int len);
void *aic_skb_pull(struct aic_sk_buff *skb, unsigned int len);

#endif