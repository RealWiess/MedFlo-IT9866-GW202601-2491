.section .text, "ax"
.code 32
.align 2
.global MATS_DCache_WB
.global MATS_DCache_Clear
.global MATS_DCache_Invalid
.global MATS_DCache_TAG
.global MATS_DCache_Data
.global MATS_DCache_TAG_SINGULAR

.extern AAA

MATS_DCache_TAG_SINGULAR: //R0:ADDR(32B Aligned)
    mrs r12,cpsr
    orr r12,r12,#0x000000c0
    msr cpsr_c,r12 @Dis irqs

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x4
    bic r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @DC
    nop
    nop

    mov r2,#0xa5
    orr r2,r2,lsl #8
    orr r2,r2,lsl #16
    mvn r3,r2

    str r2,[r0]

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    mcr p15,0,r12,c1,c0,0 @EC
    nop
    nop

    ldr r1,[r0]

    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x4
    mcr p15,0,r12,c1,c0,0 @DC
    nop
    nop

    str r3,[r0]

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    mcr p15,0,r12,c1,c0,0 @EC
    nop
    nop

    ldr r1,[r0]

    cmp r1,r3
    mov r0,#0
    moveq r0,#1

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @EC
    nop
    nop

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @Dis irqs

    bx r14

MATS_DCache_WB:
    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    bic r0,r0,#0x8
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA
    mov r2,#0xa5
    orr r2,r2,r2,lsl #8
    orr r2,r2,r2,lsl #16
    ldr r3,=AAA+32768
    mvn r12,r2

1:
    str r2,[r1],#4
    cmp r1,r3
    bne 1b

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

2:
    ldr r0,[r1] @Miss
    str r0,[r1],#16 @Dirty
    cmp r1,r3
    bne 2b

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop
    ldr r1,=AAA

3:
    str r12,[r1],#4
    cmp r1,r3
    bne 3b

    ldr r1,=BBB
    ldr r3,=BBB+32768

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

4:
    ldr r0,[r1],#4
    cmp r3,r1
    bne 4b

    mov r0,#0
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC

    ldr r1,=AAA
    ldr r3,=AAA+32768

5:
    ldr r0,[r1],#4
    cmp r0,r12
    beq 6f
    cmp r1,r3
    bne 5b
6:
    cmp r1,r3
    mov r0,#0
    movne r0,#1

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    orr r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @EC

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @EIRQs

    mov r15,r14

MATS_DCache_Clear:
    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    bic r0,r0,#0x8
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA
    mov r2,#0xa5
    orr r2,r2,r2,lsl #8
    orr r2,r2,r2,lsl #16
    ldr r3,=AAA+32768
    mvn r12,r2

1:
    str r2,[r1],#4
    cmp r1,r3
    bne 1b

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

2:
    ldr r0,[r1] @Miss
    str r0,[r1],#16 @Dirty
    cmp r1,r3
    bne 2b

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop
    ldr r1,=AAA

3:
    str r12,[r1],#4
    cmp r1,r3
    bne 3b

    ldr r1,=AAA
    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

4:
    mov r0,#0
    mcr p15,0,r0,c7,c10,0 @Clean DCache Alls,a5->5a
    nop
    nop

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop
    ldr r1,=AAA

5:
    ldr r0,[r1],#4
    cmp r0,r12
    beq 6f
    cmp r1,r3
    bne 5b
6:
    cmp r1,r3
    mov r0,#0
    movne r0,#1

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    orr r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @EC

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @EIRQs

    mov r15,r14

MATS_DCache_Invalid:
    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    bic r0,r0,#0x8
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA
    mov r2,#0xa5
    orr r2,r2,r2,lsl #8
    orr r2,r2,r2,lsl #16
    ldr r3,=AAA+32768
    mvn r12,r2

1:
    str r2,[r1],#4
    cmp r1,r3
    bne 1b

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

2:
    ldr r0,[r1],#4
    cmp r1,r3
    bne 2b

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop
    ldr r1,=AAA

3:
    str r12,[r1],#4
    cmp r1,r3
    bne 3b

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

    mov r0,#0
    mcr p15,0,r0,c7,c6,0 @Invalid DCache Alls
    nop
    nop

4:
    ldr r0,[r1]
    ldr r0,[r1],#4
    cmp r0,r12
    bne 5f
    cmp r1,r3
    bne 4b
5:
    cmp r1,r3
    mov r0,#0
    movne r0,#1

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    orr r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @EC

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @EIRQs

    mov r15,r14

MATS_DCache_TAG:
    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    bic r0,r0,#0x8
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA
    mov r2,#0xa5
    orr r2,r2,r2,lsl #8
    orr r2,r2,r2,lsl #16
    ldr r3,=AAA+32768
    mvn r12,r2

WDB_1:
    str r2,[r1],#4
    cmp r1,r3
    bne WDB_1

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

WDB_2:
    ldr r0,[r1],#4
    cmp r1,r3
    bne WDB_2

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA

WDB_3:
    str r12,[r1],#4
    cmp r1,r3
    bne WDB_3

    ldr r1,=AAA

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

WDB_4:
    ldr r0,[r1],#4
    cmp r0,r12
    beq fatal_errs
    cmp r1,r3
    bne WDB_4

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop
    ldr r1,=AAA
    mov r0,#0
    mcr p15,0,r0,c7,c6,0 @Invalid DCache Alls

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

WDB_5:
    ldr r0,[r1]
    ldr r0,[r1],#4
    cmp r0,r12
    bne fatal_errs
    cmp r1,r3
    bne WDB_5

fatal_errs:
    cmp r1,r3
    mov r0,#0
    movne r0,#1

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x4
    orr r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @EC

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @EIRQs

    mov r15,r14

MATS_DCache_Data:
    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    bic r0,r0,#0x8
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA+32768
    mov r2,#0xa5
    orr r2,r2,r2,lsl #8
    orr r2,r2,r2,lsl #16
    ldr r3,=AAA
    mvn r12,r2

wwdb_1:
    str r2,[r1,#-4]!
    cmp r1,r3
    bne wwdb_1

    ldr r1,=AAA+32768

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

wwdb_2:
    ldr r0,[r1,#-4]
    ldr r0,[r1,#-4]!
    cmp r0,r2
    bne FatalErrors
    cmp r1,r3
    bne wwdb_2

    mov r0,#0
    mcr p15,0,r0,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r0,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @DC
    nop
    nop

    ldr r1,=AAA+32768

wwdb_3:
    str r12,[r1,#-4]!
    cmp r1,r3
    bne wwdb_3

    ldr r1,=AAA+32768

    mrc p15,0,r0,c1,c0,0
    orr r0,r0,#0x4
    mcr p15,0,r0,c1,c0,0 @EC
    nop
    nop

wwdb_4:
    ldr r0,[r1,#-4]
    ldr r0,[r1,#-4]!
    cmp r0,r12
    bne FatalErrors
    cmp r1,r3
    bne wwdb_4

FatalErrors:
    cmp r1,r3
    mov r0,#0
    movne r0,#1

    mov r12,#0
    mcr p15,0,r12,c7,c14,0 @Clean&Invalid-alls
    mcr p15,0,r12,c7,c10,4 @Drain write-buffer,victim

    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x4
    bic r12,r12,#0x8
    mcr p15,0,r12,c1,c0,0 @DC

    mrs r12,cpsr
    bic r12,r12,#0x000000c0
    msr cpsr_c,r12 @EIRQs

    mov r15,r14

.section .TestICache, "awx"
.code 32
.align 5
.global BBB

BBB:
.space 32768,0xff
.end
