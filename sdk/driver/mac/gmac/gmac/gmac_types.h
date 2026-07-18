
#ifndef GMAC_TYPES_H
#define GMAC_TYPES_H

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned int		__le32;
typedef unsigned short		__be16;
typedef unsigned int		__be32;
typedef unsigned int		dma_addr_t;
typedef unsigned long long	u64;
typedef signed int			s32;

#define jiffies		itpGetTickCount()

static inline void *vzalloc(u32 size)
{
    void *tmp = malloc(size);
    if (tmp)
        memset(tmp, 0, size);
    return tmp;
}
#define vfree(x)					do { if((x)) free((x)); } while (false)

static inline void *kzalloc(u32 size)
{
    void *tmp = (void*)memalign(64, ITH_ALIGN_UP(size, 32U));
    if (tmp)
        memset(tmp, 0, size);
    return tmp;
}
#define kfree(x)					do { if((x)) free((void*)(x)); } while (false)

static inline void *kcalloc(size_t n, size_t size)
{
    return kzalloc((n*size));
}

#define min_t(type, x, y) ({			\
    type __min1 = (x);			\
    type __min2 = (y);			\
    __min1 < __min2 ? __min1 : __min2; })

#define time_after(a,b)   ((long)(b) - (long)(a) < 0)
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define BUG_ON(x)	do { if(x) ithPrintf(" %s:%d => BUG?? \n", __FILE__, __LINE__); } while (false)

#define WARN_ON(condition)							\
({ \
    int __ret_warn_on = !!(condition);			\
    if (unlikely(__ret_warn_on))				\
        ithPrintf(" %s:%s():%d => WARN_ON?? \n", __FILE__, __FUNCTION__, __LINE__);		\
    unlikely(__ret_warn_on);				\
})

#define udelay  ithDelay
#define min		ITH_MIN
#define max		ITH_MAX
#define ALIGN	ITH_ALIGN_UP
#define likely(x)	(x)
#define unlikely(x)	(x)

//===============
static inline int
//test_bit(int nr, const volatile void * addr)
test_bit(int nr, const volatile unsigned long * addr)
{
    return (1UL & ((addr)[nr >> 5] >> (nr & 31))) != 0UL;
}

static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    unsigned int mask, retval;
    unsigned int *adr = (unsigned int *)addr;

    mask = 1 << (nr & 0x1f);
    ithEnterCritical();
    retval = (mask & *adr) != 0;
    *adr |= mask;
    ithExitCritical();
    return (int)retval;
}

static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned int mask, retval;
    unsigned int *adr = (unsigned int *)addr;

    mask = 1 << (nr & 0x1f);
    ithEnterCritical();
    retval = (mask & *adr) != 0;
    *adr &= ~mask;
    ithExitCritical();
    return (int)retval;
}

//static inline void set_bit(unsigned long nr, volatile void *addr)
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    test_and_set_bit(nr, addr);
}

#define clear_bit(nr, addr)  (void)test_and_clear_bit(nr, addr)
//================

#endif
