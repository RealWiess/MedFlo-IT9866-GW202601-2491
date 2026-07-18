#ifndef STL_FA626_SRC__STL_CPU_H_
#define STL_FA626_SRC__STL_CPU_H_

#include <stdint.h>

__attribute__((always_inline))
static inline uint32_t fa626_get_cp15_c1()
{
    uint32_t value;
    __asm__ __volatile__ (
        "mrc p15,0,%[value],c1,c0,0\n"     /* read control register */ \
        : [value] "=r" (value)
    );
    return value;
}

__attribute__((always_inline))
static inline void fa626_set_cp15_c1(uint32_t value)
{
    __asm__ __volatile__ (
        "mcr p15,0,%[value],c1,c0,0\n"     /* write control register */ \
        :
        : [value] "r" (value)
    );
}

__attribute__((always_inline))
static inline void fa626_irq_local_disable_save(uint32_t *state)
{
    uint32_t tmp, tmp2;
    __asm__ __volatile__ (
        "mrs %0, cpsr_fc \n"    // tmp = cpsr_fc
        "orr %1, %0, #0x80 \n"
        "msr cpsr_fc, %1 \n"
        : "=&r" (tmp), "=r" (tmp2)
        :
        : "cc"
    );
    *state = tmp;
}

__attribute__((always_inline))
static inline void fa626_irq_local_restore(uint32_t state)
{
    __asm__ __volatile__ (
        "msr cpsr_fc, %0\n"
        :
        : "r" (state)
        : "cc"
    );
}

__attribute__((always_inline))
static inline uint32_t fa626_get_ctr()
{
    // CR0-1 Cache Type Register (CTR)
    //
    // Bits[20:18]: DSIZE. Data cache size.
    // | Size Field | Cache Size |
    // |------------|------------|
    // | 3'b000     | 512 B      |
    // | 3'b001     | 1 kB       |
    // | 3'b010     | 2 kB       |
    // | 3'b011     | 4 kB       |
    // | 3'b100     | 8 kB       |
    // | 3'b101     | 16 kB      |
    // | 3'b110     | 32 kB      |
    // | 3'b111     | 64 kB      |
    //
    // Bits[17:15]: DASS. Data cache associability.
    // | ASS Field | Associativity |
    // |-----------|---------------|
    // | 3'b000    | Direct mapped |
    // | 3'b001    | 2-way         |
    // | 3'b010    | 4-way         |
    // | 3'b011    | 8-way         |
    // | 3'b100    | 16-way        |
    // | 3'b101    | 32-way        |
    // | 3'b110    | 64-way        |
    // | 3'b111    | 128-way       |
    //
    // Bits[13:12]: DLEN. Data cache line length.
    // | LEN Field | Cache Line Length       |
    // |-----------|-------------------------|
    // | 2'b00     | Two words (Eight bytes) |
    // | 2'b01     | Four words (16 bytes)   |
    // | 2'b10     | Eight words (32 bytes)  |
    // | 2'b11     | 16 words (64 bytes)     |
    //
    // Bits[8:6]: ISIZE (Instruction cache size).
    //            The encoding is the same as DSIZE.
    // Bits[5:3]: IASS (Instruction cache associability).
    //            The encoding is the same as DASS.
    // Bits[1:0]: ILEN (Instruction cache line length).
    //            The encoding is the same as DLEN.
    uint32_t value;
    __asm__ __volatile__ (
        "mrc p15,0,%[value],c0,c0,1\n"     /* read CTR */ \
        : [value] "=r" (value)
    );
    return value;
}


__attribute__((always_inline))
static inline void fa626_dcache_flush(void)
{
    __asm__ __volatile__ (
        "mov %%r3,#0\n"
        "mcr p15,0,%%r3,c7,c10,0\n"     /* flush d-cache all */
        "mcr p15,0,%%r3,c7,c10,4\n"     /* flush d-cache write buffer */
        :
        :
        : "%r3" /* clobber list */
    );
}

__attribute__((always_inline))
static inline void fa626_idcache_disable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "bic %%r0, %%r0, #0x00001000 \n"    // clear bit 12 (I) I-Cache
        "bic %%r0, %%r0, #0x00000004 \n"    // clear bit  2 (D) D-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

__attribute__((always_inline))
static inline void fa626_icache_disable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "bic %%r0, %%r0, #0x00001000 \n"    // clear bit 12 (I) I-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

__attribute__((always_inline))
static inline void fa626_dcache_disable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "bic %%r0, %%r0, #0x00000004 \n"    // clear bit  2 (D) D-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

__attribute__((always_inline))
static inline void fa626_idcache_enable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "orr %%r0, %%r0, #0x00001000 \n"    // set bit 12 (I) I-Cache
        "orr %%r0, %%r0, #0x00000004 \n"    // set bit  2 (D) D-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

__attribute__((always_inline))
static inline void fa626_icache_enable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "orr %%r0, %%r0, #0x00001000 \n"    // set bit 12 (I) I-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

__attribute__((always_inline))
static inline void fa626_dcache_enable(void)
{
    __asm__ __volatile__ (
        "mrs %%r0, cpsr_fc \n"              // R0 = cpsr_fc
        "orr %%r0, %%r0, #0x00000004 \n"    // set bit  2 (D) D-Cache
        "msr cpsr_fc, %%r0 \n"              // cpsr_fc = R0
        :
        :
        : "%r0"
    );
}

#endif