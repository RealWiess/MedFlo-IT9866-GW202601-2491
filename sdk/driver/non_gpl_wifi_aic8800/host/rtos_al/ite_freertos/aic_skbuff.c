#include "aic_skbuff.h"
#include "rtos_al.h"
#include "log.h"
#include <string.h>

struct aic_sk_buff *aic_dev_alloc_skb(unsigned int length)
{
    struct aic_sk_buff *skb = (struct aic_sk_buff *)rtos_malloc(sizeof(struct aic_sk_buff) + length + ALIGN_SIZE);

    if (skb != NULL) {
        memset(skb, 0, sizeof(struct aic_sk_buff));
        unsigned char *data = (unsigned char *)skb + sizeof(struct aic_sk_buff);
        if((size_t)data & (ALIGN_SIZE - 1)) {
            data += ALIGN_SIZE - ((size_t)data & (ALIGN_SIZE - 1));
        }
        skb->head = data;
        skb->data = data;
        skb->tail = data;
        skb->end = data + length;
        skb->len = 0;
        skb->max_len = length;
    }
    return skb;
}

void aic_dev_kfree_skb_any(struct aic_sk_buff *skb)
{
    rtos_free((void *)skb);
}

void aic_skb_reset(struct aic_sk_buff *skb)
{
    unsigned int length = skb->max_len;
    unsigned char *data = (unsigned char *)skb + sizeof(struct aic_sk_buff);
    if((size_t)data & (32 -1)) {
        data += 32 - ((size_t)data & (32 -1));
    }
    skb->head = data;
    skb->data = data;
    skb->tail = data;
    skb->end = data + length;
    skb->len = 0;
    skb->max_len = length;
}

/**
 *	skb_put - add data to a buffer
 *	@skb: buffer to use
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer. If this would
 *	exceed the total buffer size the kernel will panic. A pointer to the
 *	first byte of the extra data is returned.
 */
void *aic_skb_put(struct aic_sk_buff *skb, unsigned int len)
{
    void *tmp = aic_skb_tail_pointer(skb);
    //SKB_LINEAR_ASSERT(skb);
    skb->tail += len;
    skb->len  += len;
    #if 0
    if (unlikely(skb->tail > skb->end))
        skb_over_panic(skb, len, __builtin_return_address(0));
    #else
    if (skb->tail > skb->end) {
        aic_dbg("%s error, tail:%p end:%p", __func__, skb->tail, skb->end);
    }
    #endif
    return tmp;
}

/**
 *	skb_push - add data to the start of a buffer
 *	@skb: buffer to use
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer at the buffer
 *	start. If this would exceed the total buffer headroom the kernel will
 *	panic. A pointer to the first byte of the extra data is returned.
 */
void *aic_skb_push(struct aic_sk_buff *skb, unsigned int len)
{
    skb->data -= len;
    skb->len  += len;
    #if 0
    if (unlikely(skb->data < skb->head))
        skb_under_panic(skb, len, __builtin_return_address(0));
    #else
    if (skb->data < skb->head) {
        aic_dbg("%s error, data:%p head:%p", __func__, skb->data, skb->head);
    }
    #endif
    return skb->data;
}

/**
 *	skb_pull - remove data from the start of a buffer
 *	@skb: buffer to use
 *	@len: amount of data to remove
 *
 *	This function removes data from the start of a buffer, returning
 *	the memory to the headroom. A pointer to the next data in the buffer
 *	is returned. Once the data has been pulled future pushes will overwrite
 *	the old data.
 */
void *aic_skb_pull(struct aic_sk_buff *skb, unsigned int len)
{
    #if 0
    return skb_pull_inline(skb, len);
    #else
    skb->len -= len;
    //BUG_ON(skb->len < skb->data_len);
    return skb->data += len;
    #endif
}
