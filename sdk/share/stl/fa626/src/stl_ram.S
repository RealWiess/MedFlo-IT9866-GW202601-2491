.global stl_ram_test
.global stl_ram_test_without_backup
.global stl_ram_test_start
.global stl_ram_test_end

stl_ram_test_start:

.section    .text.stl_ram_test
    .code   32  /* choose to generate ARM instructions. */
    .type   stl_ram_test, %function
    .size   stl_ram_test, .-stl_ram_test

/*
 * R0: start address.
 * R1: end address. The end address will not be tested.
 *     If you want to test the memory range from 0x0 ~ 0x10,
 *     but you don't want to include 0x10, just specifies
 *     the end address as 0x10.
 * R2: background pattern.
 * R3: start address of backup region.
 */
stl_ram_test:
    stmia   r3!,{r4-r9}     /* Save content of preserved registers R4-R9 */

    movs    r4, #0x1        /* Test success status by default */

    movs    r7, r2          /* setup inverted background pattern */
    rsbs    r7, r7, #0      /* R7 = 0 - R7 */
    subs    r7, r7, #1      /* R7 = R7 - 1 */

__full_loop:
    adds    r8, r0, #256    /* R8 = R0 + 256 */
    cmp     r8, r1          /* if (r8 > r1) */
    movgt   r8, r1          /*     r8 = r1  */

    /* *** Step 1 *** */
    /* BACKUP at most 256 bytes */
__fullstep_1:
    movs    r5, r0          /* R5 = R0 */
    movs    r9, r3          /* R9 = R3 */
__full1_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
    bhs     __fullstep_2    /*     goto __fullstep_2; # complete backup */

    ldr     r6, [r5, #+0]
    str     r6, [r9, #+0]
    adds    r5, r5, #+4     /* R5 = R5 + 4 */
    adds    r9, r9, #+4     /* R9 = R9 + 4 */

    b       __full1_loop

    /* *** Step 2 *** */
    /* Write background pattern with addresses increasing */
__fullstep_2:
    movs    r5, r0          /* R5 = R0 */
__full2_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
    bhs     __fullstep_3    /*     goto __fullstep_3; # complete step 1 test */

    /* write background with addresses increasing */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __full2_loop

    /* *** Step 3 *** */
    /* Verify background and write inverted background with addresses increasing */
__fullstep_3:
    movs    r5, r0          /* R5 = R0 */
__full3_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
#ifndef USE_MARCHX_TEST
    bhs     __fullstep_4    /*     goto __fullstep_4; # complete step 2 test */
#else
    bhs     __fullstep_6    /*     goto __fullstep_6; # complete step 2 test */
#endif

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __full_err      /*     goto __full_err; */

    /* write inverted background with addresses increasing */
    str     r7, [r5, #+0]   /* *((uint32_t*)R5) = R7 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __full3_loop

#ifndef USE_MARCHX_TEST
    /* *** Step 4 *** (not used at March-X test) */
    /* Verify inverted background and write background with addresses increasing */
__fullstep_4:
    movs    r5, r0          /* R5 = R0 */
__full4_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
    bhs     __fullstep_5    /*     goto __fullstep_5; # complete step 3 test */

    /* Verify inverted background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r7          /* if (R6 != R7) */
    bne     __full_err      /*     goto __full_err; */

    /* write background pattern with addresses increasing */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __full4_loop

    /* *** Step 5 *** (not used at March-X test) */
    /* Verify background and write inverted background with addresses decreasing */
__fullstep_5:
    movs    r5, r8          /* R5 = R8 */
__full5_loop:
    subs    r5, r5, #+4     /* R5 = R5 - 4 */
    cmp     r5, r0          /* if (R5 < R0) */
    blt     __fullstep_6    /*     goto __fullstep_6; # complete step 4 test */

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __full_err      /*     goto __full_err; */

    /* write inverted background */
    str     r7, [r5, #+0]   /* *((uint32_t*)R5) = R7 */

    b       __full5_loop
#endif /* March-X  */

    /* *** Step 6 *** */
    /* Verify inverted background and write background with addresses decreasing */
__fullstep_6:
    movs    r5, r8          /* R5 = R8 */
__full6_loop:
    subs    r5, r5, #+4     /* R5 = R5 - 4 */
    cmp     r5, r0          /* if (R5 < R0) */
    blt     __fullstep_7    /*     goto __fullstep_7; # complete step 5 test */

    /* Verify inverted background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r7          /* if (R6 != R7) */
    bne     __full_err      /*     goto __full_err; */

    /* write background pattern */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */

    b       __full6_loop

    /* *** Step 7 *** */
    /* Verify background with addresses increasing */
__fullstep_7:
    movs    r5, r0          /* R5 = R0 */
__full7_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
    bhs     __fullstep_8    /*     goto __fullstep_8; # complete step 7 test */

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __full_err      /*     goto __full_err; */

    ldr     r6, [r3, #+0]
    str     r6, [r5, #+0]
    adds    r5, r5, #+4     /* R5 = R5 + 4 */
    b       __full7_loop


    /* *** Step 8 *** */
    /* RESTORE data */
__fullstep_8:
    movs    r5, r0          /* R5 = R0 */
    movs    r9, r3          /* R9 = R3 */
__full8_loop:
    cmp     r5, r8          /* if (R5 >= R8) */
    bhs     __chk_complete  /*     goto __chk_complete; # check if complete all tests */

    ldr     r6, [r9, #+0]
    str     r6, [r5, #+0]

    adds    r5, r5, #+4     /* R5 = R5 + 4 */
    adds    r9, r9, #+4     /* R5 = R5 + 4 */
    b       __full8_loop

__chk_complete:
    cmp     r8, r1          /* if (R8 < R1)  */
    blt     __next_256_bytes/*     goto __fullstep_1; # test the next 256 bytes */
    b       __full_ret      /* else goto __full_ret;  # complete all the tests */

__next_256_bytes:
    adds    r0, r0, #256    /* r0 = r0 + 256 */
    b       __full_loop    /* goto __fullstep_1 */

__full_err:
    movs    r4, #0          /* error result */

__full_ret:
    movs    r0, r4
    ldmdb   r3!, {r4-r9}    /* Restore R4-R9 content */

    bx      lr              /* return to the caller */
stl_ram_test_end:


.section    .text.stl_ram_test_without_backup
    .code   32  /* choose to generate ARM instructions. */
    .type   stl_ram_test_without_backup, %function
    .size   stl_ram_test_without_backup, .-stl_ram_test_without_backup

/*
 * R0: start address.
 * R1: end address.
 *     The end address will not be tested.
 *     If you want to test the memory range from 0x0 ~ 0x10,
 *     but you don't want to include 0x10, just specifies
 *     the end address as 0x10.
 * R2: background pattern.
 * R3: start address of backup region.
 */
stl_ram_test_without_backup:
    stmia   r3!,{r4-r7}     /* Save content of preserved registers R4-R7 */

    movs    r4, #0x1        /* Test success status by default */

    movs    r7, r2          /* setup inverted background pattern */
    rsbs    r7, r7, #0      /* R7 = 0 - R7 */
    subs    r7, r7, #1      /* R7 = R7 - 1 */

    /* *** Step 1 *** */
    /* Write background pattern with addresses increasing */
    movs    r5, r0          /* R5 = R0 */
__wb_full1_loop:
    cmp     r5, r1          /* if (R5 >= R1) */
    bhs     __wb_fullstep_2 /*     goto __wb_fullstep_2; # complete step 1 test */

    /* write background with addresses increasing */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __wb_full1_loop

    /* *** Step 2 *** */
    /* Verify background and write inverted background with addresses increasing */
__wb_fullstep_2:
    movs    r5, r0          /* R5 = R0 */
__wb_full2_loop:
    cmp     r5, r1          /* if (R5 >= R1) */

#ifndef USE_MARCHX_TEST
    bhs     __wb_fullstep_3 /*     goto __wb_fullstep_3; # complete step 2 test */
#else
    bhs     __wb_fullstep_5 /*     goto __wb_fullstep_5; # complete step 2 test */
#endif

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __wb_full_err   /*     goto __wb_full_err; */

    /* write inverted background with addresses increasing */
    str     r7, [r5, #+0]   /* *((uint32_t*)R5) = R7 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __wb_full2_loop

#ifndef USE_MARCHX_TEST
    /* *** Step 3 *** (not used at March-X test) */
    /* Verify inverted background and write background with addresses increasing */
__wb_fullstep_3:
    movs    r5, r0          /* R5 = R0 */
__wb_full3_loop:
    cmp     r5, r1          /* if (R5 >= R1) */
    bhs     __wb_fullstep_4 /*     goto __wb_fullstep_4; # complete step 3 test */

    /* Verify inverted background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r7          /* if (R6 != R7) */
    bne     __wb_full_err   /*     goto __wb_full_err; */

    /* write background pattern with addresses increasing */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */
    adds    r5, r5, #+4     /* R5 = R5 + 4 */

    b       __wb_full3_loop

    /* *** Step 4 *** (not used at March-X test) */
    /* Verify background and write inverted background with addresses decreasing */
__wb_fullstep_4:
    movs    r5, r1          /* R5 = R1 */
__wb_full4_loop:
    subs    r5, r5, #+4     /* R5 = R5 - 4 */
    cmp     r5, r0          /* if (R5 < R0) */
    blt     __wb_fullstep_5 /*     goto __wb_fullstep_5; # complete step 4 test */

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __wb_full_err   /*     goto __wb_full_err; */

    /* write inverted background */
    str     r7, [r5, #+0]   /* *((uint32_t*)R5) = R7 */

    b       __wb_full4_loop
#endif

    /* *** Step 5 *** */
    /* Verify inverted background and write background with addresses decreasing */
__wb_fullstep_5:
    movs    r5, r1          /* R5 = R1 */
__wb_full5_loop:
    subs    r5, r5, #+4     /* R5 = R5 - 4 */
    cmp     r5, r0          /* if (R5 < R0) */
    blt     __wb_fullstep_6 /*     goto __wb_fullstep_6; # complete step 5 test */

    /* Verify inverted background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r7          /* if (R6 != R7) */
    bne     __wb_full_err   /*     goto __wb_full_err; */

    /* write background pattern */
    str     r2, [r5, #+0]   /* *((uint32_t*)R5) = R2 */

    b       __wb_full5_loop

    /* *** Step 6 *** */
    /* Verify background with addresses increasing */
__wb_fullstep_6:
    movs    r5, r0          /* R5 = R0 */
__wb_full6_loop:
    cmp     r5, r1          /* if (R5 >= R1) */
    bhs     __wb_full_ret   /*     goto __wb_full_ret; # complete all tests */

    /* Verify background pattern */
    ldr     r6, [r5, #+0]   /* R6 = *((uint32_t*)R5) */
    cmp     r6, r2          /* if (R6 != R2) */
    bne     __wb_full_err   /*     goto __wb_full_err; */

    adds    r5, r5, #+4     /* R5 = R5 + 4 */
    b       __wb_full6_loop

__wb_full_err:
    movs    r4, #0          /* error result */

__wb_full_ret:
    movs    r0, r4
    ldmdb   r3!, {r4-r7}    /* Restore R4-R7 content */

    bx      lr              /* return to the caller */