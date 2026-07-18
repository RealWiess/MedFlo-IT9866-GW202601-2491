.section .text, "ax"
.code 32
.align 2

.global atan_cordic

atan_cordic:
	stmfd r13!,{r4-r11,r14}
	mov r2,r0
	mov r3,r1
	mov r0,r3
	mov r1,r2
	ldr r2,=cossintab
	mov r14,#16384
	mov r7,#0
	mov r8,#15
	cmp r1,#0
	bne atan2cordic_1
	cmp r0,#0
	ldr r9,=0x7fff
	movge r0,#0
	movlt r0,r9
	b final

atan2cordic_1:
	cmp r0,#0
	bne cordic
	cmp r1,#0
	mov r0,#16384
	rsblt r0,r0,#0 @-pi/2
	b final

cordic:
	ldr r3,[r2],#4
	mov r4,#0
	cmp r1,#0
	blt cordic_neg
	beq cordic_eq
	smlabb r4,r0,r3,r4
	smlabt r4,r1,r3,r4
	smulbb r5,r1,r3
	smulbt r6,r0,r3
	sub r5,r5,r6
	add r7,r7,r14
	mov r0,r4,asr #15
	mov r1,r5,asr #15
	mov r14,r14,asr #1
	subs r8,r8,#1
	bgt cordic
	mov r0,r7
	ble final
	
cordic_neg:
	mov r5,#0
	smulbb r4,r0,r3
	smulbt r6,r1,r3
	sub r4,r4,r6
	smlabb r5,r1,r3,r5
	smlabt r5,r0,r3,r5
	sub r7,r7,r14
	mov r0,r4,asr #15
	mov r1,r5,asr #15
	mov r14,r14,asr #1
	subs r8,r8,#1
	bgt cordic
	mov r0,r7

cordic_eq:
	mov r14,r14,asr #1
	subs r8,r8,#1
	bgt cordic
	mov r0,r7

final:
	ldmfd r13!,{r4-r11,r15}

cossintab:
	.hword 0,32767,23169,23169
	.hword 30272,12539,32137,6392
	.hword 32609,3211,32727,1607
	.hword 32757,804,32764,402
	.hword 32766,201,32766,100
	.hword 32766,50,32766,25
	.hword 32766,12,32766,6
	.hword 32766,3,32766,1

.end
