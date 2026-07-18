.equ    MODE_USR, 0x10
.equ    MODE_FIQ, 0x11
.equ    MODE_IRQ, 0x12
.equ    MODE_SVC, 0x13
.equ    MODE_ABT, 0x17
.equ    MODE_UND, 0x1B
.equ    MODE_SYS, 0x1F
.equ    MODE_MSK, 0x1F

.global stl_cpu_startup_test

.macro TEST_CPU_REG r:req
    movs    \r, #0xaa
    cmp     \r, #0xaa
    bne     fail
    movs    \r, #0x55
    cmp     \r, #0x55
    bne     fail
    movs    \r, #0xaa00
    cmp     \r, #0xaa00
    bne     fail
    movs    \r, #0x5500
    cmp     \r, #0x5500
    bne     fail
    movs    \r, #0xaa0000
    cmp     \r, #0xaa0000
    bne     fail
    movs    \r, #0x550000
    cmp     \r, #0x550000
    bne     fail
    movs    \r, #0xaa000000
    cmp     \r, #0xaa000000
    bne     fail
    movs    \r, #0x55000000
    cmp     \r, #0x55000000
    bne     fail
    .endm

.macro SWITCH_TO_MODE m:req
    mov     r1, r0                  /* r1 = r0 */
    bic     r1, #(MODE_MSK)         /* Clear MODE bits */
    orr     r1, \m
    msr     CPSR, r1                /* Switch to IRQ mode */
    .endm

.macro TEST_CPU_REG13_14 m:req
    SWITCH_TO_MODE \m

    movs    r1, r13
    TEST_CPU_REG r13
    movs    r13, r1                 /* Restore r13 */

    movs    r1, r14
    TEST_CPU_REG r14
    movs    r14, r1                 /* Restore r14 */
    .endm

.section    .text.stl_cpu_startup_test
    .code   32  // choose to generate ARM instructions.
    .type   stl_cpu_startup_test, %function
    .size   stl_cpu_startup_test, .-stl_cpu_startup_test

stl_cpu_startup_test:
     /* Save preserved registers values into stack */
    stmdb   sp!, {r4, r5, r6, r7, r8, r9, r10, r11}

    movs    r0, #0x00000000     /* Set Z(ero) Flag */
    bne     fail                /* Fails if Z clear */
    subs    r0, #1              /* Set N(egative) Flag */
    bpl     fail                /* Fails if N clear */
    adds    r0, #2              /* Set C(arry) Flag and do not set Z */
    bcc     fail                /* Fails if C clear */
    movs    r0, #0x80000000     /* Prepares Overflow test */
    adds    r0, r0, r0          /* Set V(overflow) Flag */
    bvc     fail                /* Fails if V clear */

    /* Register R0 (holds value returned by the function) */
    TEST_CPU_REG r0
    movs    r0, #0              /* For ramp test */

    /* Register R1 */
    TEST_CPU_REG r1
    movs    r1, #1              /* For ramp test */

    /* Register R2 */
    TEST_CPU_REG r2
    movs    r2, #2              /* For ramp test */

    /* Register R3 */
    TEST_CPU_REG r3
    movs    r3, #3              /* For ramp test */

    /* Register R4 */
    TEST_CPU_REG r4
    movs    r4, #4              /* For ramp test */

    /* Register R5 */
    TEST_CPU_REG r5
    movs    r5, #5              /* For ramp test */

    /* Register R6 */
    TEST_CPU_REG r6
    movs    r6, #6              /* For ramp test */

    /* Register R7 */
    TEST_CPU_REG r7
    movs    r7, #7              /* For ramp test */

    /* Register R8 */
    TEST_CPU_REG r8
    movs    r8, #8              /* For ramp test */

    /* Register R9 */
    TEST_CPU_REG r9
    movs    r9, #9              /* For ramp test */

    /* Register R10 */
    TEST_CPU_REG r10
    movs    r10, #10            /* For ramp test */

    /* Register R11 */
    TEST_CPU_REG r11
    movs    r11, #11            /* For ramp test */

    /* Register R12 */
    TEST_CPU_REG r12
    movs    r12, #12            /* For ramp test */

    /* Ramp pattern verification */
    cmp     r0, #0
    bne     fail
    cmp     r1, #1
    bne     fail
    cmp     r2, #2
    bne     fail
    cmp     r3, #3
    bne     fail
    cmp     r4, #4
    bne     fail
    cmp     r5, #5
    bne     fail
    cmp     r6, #6
    bne     fail
    cmp     r7, #7
    bne     fail
    cmp     r8, #8
    bne     fail
    cmp     r9, #9
    bne     fail
    cmp     r10, #10
    bne     fail
    cmp     r11, #11
    bne     fail
    cmp     r12, #12
    bne     fail

    /*
     * Test SP (R13) and LR (R14) registers in different modes.
     */
    mrs     r0, CPSR        /* r0 = cpsr, Backup the current mode */

    /* Register R13 (SP) & R14 (LR) - IRQ mode */
    TEST_CPU_REG13_14 #(MODE_IRQ)

    /* Register R13 (SP) & R14 (LR) - FIQ mode */
    TEST_CPU_REG13_14 #(MODE_FIQ)

    /* Register R13 (SP) & R14 (LR) - ABT mode */
    TEST_CPU_REG13_14 #(MODE_ABT)

    /* Register R13 (SP) & R14 (LR) - UND mode */
    TEST_CPU_REG13_14 #(MODE_UND)

    /* Register R13 (SP) & R14 (LR) - SYS mode */
    TEST_CPU_REG13_14 #(MODE_SYS)

    /* Register R13 (SP) & R14 (LR) - SVC mode */
    TEST_CPU_REG13_14 #(MODE_SVC)

    msr     CPSR, r0        /* Switch back to the original mode */

    movs    r0, #0x1        /* CPUTEST_SUCCESS */
    b       end
fail:
    mov     r0, #0x0        /* CPUTEST_FAIL */
end:
    /* Restore preserved registers values from stack */
    ldmia   sp!, {r4, r5, r6, r7, r8, r9, r10, r11}

    bx      lr              /* return to the caller */