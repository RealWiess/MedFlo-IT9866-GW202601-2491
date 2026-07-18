.global stl_cpu_runtime_test_

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

.section    .text.stl_cpu_runtime_test_
    .code   32  // choose to generate ARM instructions.
    .type   stl_cpu_runtime_test_, %function
    .size   stl_cpu_runtime_test_, .-stl_cpu_runtime_test_

stl_cpu_runtime_test_:
    stmdb   sp!, {r4, r5, r6, r7, r8, r9, r10, r11}

    /* Register R0 (holds value returned by the function) */
    TEST_CPU_REG r0
    movs    r0, #0          /* For ramp test */

    /* Register R1 */
    TEST_CPU_REG r1
    movs    r1, #1          /* For ramp test */

    /* Register R2 */
    TEST_CPU_REG r2
    movs    r2, #2          /* For ramp test */

    /* Register R3 */
    TEST_CPU_REG r3
    movs    r3, #3          /* For ramp test */

    /* Register R4 */
    TEST_CPU_REG r4
    movs    r4, #4          /* For ramp test */

    /* Register R5 */
    TEST_CPU_REG r5
    movs    r5, #5          /* For ramp test */

    /* Register R6 */
    TEST_CPU_REG r6
    movs    r6, #6          /* For ramp test */

    /* Register R7 */
    TEST_CPU_REG r7
    movs    r7, #7          /* For ramp test */

    /* Register R8 */
    TEST_CPU_REG r8
    movs    r8, #8          /* For ramp test */

    /* Register R9 */
    TEST_CPU_REG r9
    movs    r9, #9          /* For ramp test */

    /* Register R10 */
    TEST_CPU_REG r10
    movs    r10, #10        /* For ramp test */

    /* Register R11 */
    TEST_CPU_REG r11
    movs    r11, #11        /* For ramp test */

    /* Register R12 */
    TEST_CPU_REG r12
    movs    r12, #12        /* For ramp test */

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

    movs    r0, #0x1        /* CPUTEST_SUCCESS */
    b       end
fail:
    mov     r0, #0x0        /* CPUTEST_FAIL */
end:
    ldmia   sp!, {r4, r5, r6, r7, r8, r9, r10, r11}

    bx      lr              /* return to the caller */