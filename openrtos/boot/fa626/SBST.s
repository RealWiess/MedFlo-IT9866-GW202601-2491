.section .text, "ax"
.code 32
.align 2
.global TestCache
.global DisICache
.global EnaICache
.global MATS_ICache
.global MATS_ICache_TAG

MATS_ICache_TAG:
    stmfd r13!,{r4-r11,r14}

    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs
    nop
    nop

    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x00001000 @DC
    mcr p15,0,r12,c1,c0,0
    nop @FIC
    nop @FIC

    ldr r3,=AAA @32B-aligned
    mov r4,#1024
1:
    ldr r2,=0xe2800005 @andeq r0,r0,r1,mla r0,r5,r6,r0
    str r2,[r3]
    ldr r2,=0xe2800005
    str r2,[r3,#4]
    ldr r2,=0xe2800005
    str r2,[r3,#8]
    @ldr r2,=0xee11cf10 @mrc p15,0,r12,c1,c0,0
    ldr r2,=0xe1a00000 @nop
    str r2,[r3,#12]
    @ldr r2,=0xe3ccca01 @bic r12,r12,#0x1000
    ldr r2,=0xe1a00000 @nop
    str r2,[r3,#16]
    @ldr r2,=0xee01cf10 @mcr p15,0,r12,c1,c0,0
    ldr r2,=0xe1a00000 @nop
    str r2,[r3,#20]
    ldr r2,=0xe1a0f00e @mov r15,r14
    str r2,[r3,#24]
    ldr r2,=0xe1a00000 @nop
    str r2,[r3,#28]

    mov r2,#0
    mcr p15,0,r2,c7,c14,0 @Clean & invalid DCache
    mcr p15,0,r2,c7,c10,4 @Drain write-buffer,victim
    mcr p15,0,r2,c7,c5,0 @Invalid ICache
    nop
    nop

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0
    nop
    nop

    mov r0,#0
    mov r2,r3
    mov r14,r15
    bx r2
    nop @FIC
    nop @FIC
    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0
    nop
    nop

    ldr r2,=0xe2800001
    str r2,[r3]
    ldr r2,=0xe2800002
    str r2,[r3,#4]
    ldr r2,=0xe2800003
    str r2,[r3,#8]

    mov r2,#0
    mcr p15,0,r2,c7,c14,0 @Clean &invalid DCache
    mcr p15,0,r2,c7,c10,4 @Drain write-buffer,victim
    nop
    nop

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0
    nop
    nop
    mov r0,#0
    mov r2,r3
    mov r14,r15
    bx r2
    nop @FIC
    nop @FIC
    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0
    nop
    nop

    teq r0,#15
    bne 2f
    add r3,r3,#32
    subs r4,r4,#1
    bgt 1b

2:
    mov r0,r2

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000 @Fails
    mcr p15,0,r12,c1,c0,0 @EIC

    mrs r2,cpsr
    bic r2,r2,#0x000000c0
    msr cpsr_c,r2

    ldmfd r13!,{r4-r11,r15}

MATS_ICache:
    stmfd r13!,{r4-r11,r14}

    mov r5,#3
    mov r6,#4

    mrs r0,cpsr
    orr r0,r0,#0x000000c0
    msr cpsr_c,r0 @Dis irqs

    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#0x00001000 @Fails
    mcr p15,0,r0,c1,c0,0 @Dis-IC
    nop @FIC
    nop @FIC

    ldr r3,=AAA+32768 @cache-line base-address
    mov r4,#1024
Ctrl_Loop:
    mov r12,#1024
    ldr r1,=AAA
Ctrl_Inner:
    ldr r2,=0xe280000f
    str r2,[r1],#4 @add r0,r0,#15
    str r2,[r1],#4 @add r0,r0,#15
    str r2,[r1],#4 @add r0,r0,#15
    ldr r2,=0xe1a00000 @mrc p15,0,r12,c1,c0,0
    str r2,[r1],#4
    ldr r2,=0xe1a00000 @bic r12,r12,#0x1000
    str r2,[r1],#4
    ldr r2,=0xe1a00000 @mcr p15,0,r12,c1,c0,0
    str r2,[r1],#4
    ldr r2,=0xe1a0f00e @mov r15,r14
    str r2,[r1],#4
    ldr r2,=0xe1a00000 @nop
    str r2,[r1],#4
    subs r12,r12,#1
    bgt Ctrl_Inner

    ldr r2,=0xe1a00000 @nop
    str r2,[r3,#-4]!
    ldr r2,=0xe1a0f00e @mov r15,r14
    str r2,[r3,#-4]!
    ldr r2,=0xe1a00000 @mcr p15,0,r12,c1,c0,0
    str r2,[r3,#-4]!
    ldr r2,=0xe1a00000 @bic r12,r12,#0x1000
    str r2,[r3,#-4]!
    ldr r2,=0xe1a00000 @mrc p15,0,r12,c1,c0,0
    str r2,[r3,#-4]!
    ldr r2,=0xe0200695 @add r0,r0,#3
    str r2,[r3,#-4]!
    ldr r2,=0xe0200695 @add r0,r0,#2
    str r2,[r3,#-4]!
    ldr r2,=0xe0200695 @add r0,r0,#1 @same-tag
    str r2,[r3,#-4]!

    mov r2,#0
    mcr p15,0,r2,c7,c14,0 @Clean DCache,(dirty&valid)
    mcr p15,0,r2,c7,c10,4 @Drain write-buffer,victim
    mcr p15,0,r2,c7,c5,0 @Invalid ICache

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000
    mov r2,r3
    mcr p15,0,r12,c1,c0,0 @EnIC
    mov r14,r15 @NI
    bx r2 @NI
    nop @FIC,diff-ways
    nop @FIC,diff-ways

    @mrc p15,0,r12,c1,c0,0
    @orr r12,r12,#0x00001000
    mov r0,#0 @sum=0
    @mcr p15,0,r12,c1,c0,0 @EnIC
    mov r14,r15 @NI
    bx r2 @NI
    nop @fic
    nop @fic
    mrc p15,0,r12,c1,c0,0
    bic r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0 @DIC
    nop
    nop

    teq r0,#36
    mov r0,#0
    movne r0,#1
    bne Ctrl_Error
    subs r4,r4,#1
    bgt Ctrl_Loop

Ctrl_Error:
    mov r12,r0

    sub r13,r13,#28
    mov r0,#12
    str r0,[r13],#4
    str r0,[r13],#4
    str r0,[r13],#4
    str r0,[r13],#4
    str r0,[r13],#4
    mov r0,#13
    str r0,[r13],#4
    str r0,[r13],#4

    ldr r3,[r13,#-4]!@13
    ldr r0,[r13,#-4]!@13
    ldr r2,[r13,#-4]!@12
    ldr r10,[r13,#-4]!@12
    ldr r9,[r13,#-4]!@12
    mla r2,r3,r2,r0@169
    ldr r0,[r13,#-4]!@12
    @str r3,[r13,#-4]!
    str r4,[r13,#-4]!
    mla r0,r9,r0,r2@12*12+169
    mov r1,r10
    add r13,r13,#28

    bl idiv_func

    ldr r2,=0x13d
    teq r0,r2
    mov r0,#0
    movne r0,#1
    orr r0,r0,r12

    mrs r2,cpsr
    bic r2,r2,#0x000000c0
    msr cpsr_c,r2

    ldmfd r13!,{r4-r11,r15}

idiv_func: add r0,r0,#4
    mov r15,r14

EnaICache:
    mov r0,#0
    mcr p15,0,r0,c7,c5,0 @Invalid IC-Alls
    nop
    nop
    mrc p15,0,r2,c1,c0,0
    orr r2,r2,#0x00001000
    mcr p15,0,r2,c1,c0,0
    nop
    nop
    mov r15,r14

DisICache:
    mrc p15,0,r2,c1,c0,0
    bic r2,r2,#0x00001000 @Fails
    mcr p15,0,r2,c1,c0,0 @Dis-IC
    nop
    nop
    mov r15,r14

TestCache:
    mrs r2,cpsr
    orr r2,r2,#0x000000c0
    msr cpsr_c,r2

    mrc p15,0,r2,c1,c0,0
    bic r2,r2,#0x00001000 @Fails
    mcr p15,0,r2,c1,c0,0 @Dis-IC
    nop @fic
    nop @fic

LineFills:
	ldr r2,=0xe2800001
    str r2,[r0],#4
    ldr r2,=0xe2800002
    str r2,[r0],#4
    ldr r2,=0xe2800004
    str r2,[r0],#4
    ldr r2,=0xe2800008
    str r2,[r0],#4
    ldr r2,=0xe2800010
    str r2,[r0],#4
    ldr r2,=0xe2800020
    str r2,[r0],#4
    ldr r2,=0xe2800040
    str r2,[r0],#4
    ldr r2,=0xe2800080
    str r2,[r0],#4
    subs r1,r1,#8
    bgt LineFills

    mov r1,#1
    mov r2,#0
    mcr p15,0,r2,c7,c14,0 @Clean DCache,(dirty&valid)
    mcr p15,0,r2,c7,c10,4 @Drain write-buffer,victim
    mcr p15,0,r2,c7,c5,0 @Invalid ICache
    mov r3,r14

    ldr r2,=AAA
    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0 @EnIC
    mov r14,r15 @NI
    bx r2 @NI

    mov r0,#0 @sum=0
    mov r14,r15 @NI
    bx r2 @NI
    nop @IC
    nop @IC
    teq r0,#261120
    movne r0,#1 @return error
    bxne r3

    ldr r0,=AAA
    ldr r2,=0xe1e00000 @mvn r0,r0
    mov r12,#8192
loop2:
    str r2,[r0],#4
    sub r12,r12,#1
    cmp r12,#4096
    bgt loop2

    ldr r2,=0xe2800001
loop4:
    str r2,[r0],#4
    subs r12,r12,#1
    bgt loop4

    mov r0,#0
    mov r2,#0
    mcr p15,0,r2,c7,c14,0 @Clean DCache,(dirty&valid)
    mcr p15,0,r2,c7,c10,4 @Drain write-buffer,victim
    mcr p15,0,r2,c7,c5,0 @Invalid ICache

    mrc p15,0,r12,c1,c0,0
    orr r12,r12,#0x00001000
    mcr p15,0,r12,c1,c0,0 @EnIC
    ldr r2,=AAA
    mov r14,r15
    bx r2 @IC
    nop
    nop
    mov r14,r15
    bx r2
    nop
    nop
    teq r0,#8192
    mov r0,#0
    movne r0,#1

    mrs r2,cpsr
    bic r2,r2,#0x000000c0
    msr cpsr_c,r2

	bx r3

.section .TestICache, "awx"
.code 32
.align 5
.global AAA

AAA:
.space 32768,0xff
    mov r15,r14
.end
