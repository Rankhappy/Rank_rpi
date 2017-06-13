#include "arm.h"

.extern sp_test

.text
.align	5
.global __start
.func   __start
.type	__start, %function

__start:
	b	reset
	b	.			/* Undef */
	b	smc_handle	/* Syscall */
	b	.			/* Prefetch abort */
	b	.			/* Data abort */
	b	.			/* Reserved */
	b	.			/* IRQ */
	b	.			/* FIQ */
	
.size	__start, .-__start
.endfunc

.func reset
.type reset, %function
reset:
#if 0
	/*only support armv7 architecture*/
	mrc p15, 0, r0, c0, c0, 0 
	and r0, r0, #0xf0000 
	cmp r0, #0xf0000
	bne .
#endif

	/*
	*swich to svc mode, for raspberry pi, it is on hyp mode when booting.
	*on armv8, 
	*it seems can not modify the mode of cpsr directly,
	*or use cps instruction is also forbidden.
	*/
	mrs r0, cpsr
    bic r0,r0,#(PSR_MODE_MASK)
    orr r0,r0,#(PSR_MODE_SVC)
    orr r0, r0, #(PSR_I_BIT | PSR_F_BIT | PSR_A_BIT)
    msr spsr_cxsf, r0
    add r0, pc, #4
    msr ELR_hyp, r0
    eret

	/*set vbar temporarily*/
	adr r0, __start
	mcr	p15, 0, r0, c12, c0, 0
	dsb

	/*Switch to monitor mode*/
	//smc #0
    
	/*set sctlr, disable mmu, enable icache&bp*/
	mrc p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(SCTLR_M | SCTLR_C)
	orr r0, r0, #(SCTLR_I | SCTLR_Z)
	mcr p15, 0, r0, c1, c0, 0
	dsb

	adr	r6, LC0
	/*
	*r0 = LC0
	*r1 = _sbss
	*r8 = _ebss
	*r10 = p1 virtual address
	*r11 = p2 virtual address
	*/
	ldmia r6, {r0, r1, r8, r10, r11}
	sub r12, r0, r6  //r12 = offset of vir-phy
	sub r1, r1, r12
	sub r4, r8, r12
	/*clear bss, r1 = _sbss, r8 = _ebss*/
	mov	r2, #0
	mov	r3, #0
clear_bss:
	stmia r1!, {r2, r3}
	cmp	r1, r4
	bls	clear_bss
	dmb

	/*set page table*/
	mov r1, pc
	bic r5, r1, #0xff 
	bic r5, r5, #0xf00 //r5 = phy addr
	add r4, r5, r12 //r4 = vir addr
	sub r6, r11, r12 //r6 = p2 phy addr
	sub r7, r10, r12 //r7 = p1 phy addr	
	ldr r2, =0xfff
	add r3, r8, r2
	sub r3, r3, r4
	mov r3, r3, lsr #12 //r3 = n pages

	/*
	*do coarse page map, maximize size = 1M
	*fill the 2nd level page table
	*r3 =  n pages
	*r4 = start vir addr
	*r5 = start phy addr
	*r6 = p2 phy addr
	*/
	mov r0, r4, lsr #12
	and r0, r0, #0xff
	add r1, r6, r0, lsl #2
	bic r2, r5, #0xff
	bic r2, r2, #0xf00
	orr r2, r2, #((1<<1)|(3<<2)|(1<<4))
	orr r2, r2, #(1<<10)
p2Loop:
	str r2, [r1], #4
	add r2, r2, #0x1000
	subs r3, r3, #1
	bne p2Loop
	dmb
	
	/*
	*fill the 1st level page table
	*r4 = start vir addr
	*r6 = p2 phy addr
	*r7 = p1 phy addr
	*/
	bic r0, r6, #0xff
	bic r0, r0, #0x300
	orr r0, r0, #((1<<0)/*|(1<<3)*/)
	str r0, [r7, r4, lsr #18]
	dmb
	
	/*
	*do section identity map, size = 1M.
	*r5 = start phy addr
	*r5 = start vir addr
	*r7 = p1 phy addr
	*/
	mov r0, r5, lsr #20
	mov r0, r0, lsl #20
	orr r0, r0, #((1<<1)|(3<<2))
	orr r0, r0, #(1<<10)
	orr r0, r0, #((1<<16)/*|(1<<19)*/)
	str r0, [r7, r5, lsr #18]
	dmb

#if 1  // temporary for uart, disable later.
	ldr r1, =0x3f200000
	/*
	*do sectionio map, size = 1M.
	*r1 = start phy addr
	*r1 = start vir addr
	*r7 = p1 phy addr
	*/
	mov r0, r1, lsr #20
	mov r0, r0, lsl #20
	orr r0, r0, #(1<<1)
	orr r0, r0, #(1<<10)
	str r0, [r7, r1, lsr #18]
	dmb
#endif

	/*
	*set ttbr ttbcr
	*r7 = p1 phy addr
	*/
	orr	r0, r7, #((1<<3)|(1<<6)|(1<<1)|(1<<5))
	mcr p15, 0, r0, c2, c0, 0
	mcr p15, 0, r0, c2, c0, 1
	mov r0, #0
	mcr p15, 0, r0, c2, c0, 2 //use ttbr0 for all of the 4g size.
	mcr p15, 0, r0, c13, c0, 1 //set contextidr
	dsb
	isb
	
	/*set dacr*/
	ldr	r0, =0x05050505
	mcr p15, 0, r0, c3, c0, 0


	/*save the booting param*/
	ldr r0, =g_boot_param
	sub r0, r0, r12
	stmia r0, {r8, r10, r12}

	/*invalidate icache bp and tlbs*/
	mov	r0, #0
	mcr p15, 0, r0, c8, c7, 0 //Invalidate TLBs
	mcr p15, 0, r0, c8, c6, 0// Invalidate Data TLBs.
	mcr p15, 0, r0, c8, c5, 0 // Invalidate Instruction TLBs.
	mcr p15, 0, r0, c7, c5, 6 //Invalidate all branch predictors.
	mcr p15, 0, r0, c7, c5, 0 //Invalidate all instruction caches to PoU, invalidate branch predictors.
	dsb
	isb

	/*invalidate and clean the entire dcache*/
	mrc p15, 1, r0, c0, c0, 1     //Read CLIDR into R0
	ands r3, r0, #0x07000000 
	mov r3, r3, lsr #23           //Cache level value (naturally aligned)
	beq finished
	mov r10, #0
loop1:
	add r2, r10, r10, lsr #1    //Work out 3 x cachelevel
	mov r1, r0, lsr r2       //bottom 3 bits are the Cache type for this level
	and r1, r1, #7       //get those 3 bits alone
	cmp r1, #2
	blt skip       //no cache or only instruction cache at this level
	mcr p15, 2, r10, c0, c0, 0 //write CSSELR from R10
	isb      //ISB to sync the change to the CCSIDR
	mrc p15, 1, R1, c0, c0, 0 //read current CCSIDR to R1
	and r2, r1, #7 //extract the line length field
	add r2, r2, #4 //add 4 for the line length offset (log2 16 bytes)
	ldr r4, =0x3FF
	ands r4, r4, r1, lsr #3 //R4 is the max number on the way size (right aligned)
	clz r5, r4 ; //R5 is the bit position of the way size increment
	ldr r7, =0x00007FFF
	ands r7, r7, r1, lsr #13 //R7 is the max number of the index size (right aligned)
loop2:
	mov r9, r4  //R9 working copy of the max way size (right aligned)
loop3:
	orr r11, r10, r9, lsl r5 //factor in the way number and cache number into R11
	orr r11, r11, r7, lsl r2  //factor in the index number
	mcr p15, 0, r11, c7, c14, 2 //DCCISW, clean&invalidate by set/way
	subs r9, r9, #1 //decrement the way number
	bge loop3
	subs r7, r7, #1 //decrement the index
	bge loop2
skip:
	add r10, r10, #2 //increment the cache number
	cmp r3, r10
	bgt loop1
	dsb
finished:

	/*set sctlr, enable mmu*/
	mrc p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(SCTLR_A | SCTLR_C)
	bic	r0, r0, #(SCTLR_SW | SCTLR_I | SCTLR_V | SCTLR_RR | SCTLR_HA)
	bic	r0, r0, #(SCTLR_EE | SCTLR_TRE | SCTLR_AFE | SCTLR_TE)
	orr	r0, r0, #(SCTLR_M)
	orr r0, r0, #(SCTLR_A)
	orr	r0, r0, #(SCTLR_RR)
	orr	r0, r0, #(SCTLR_C)
	orr	r0, r0, #(SCTLR_I)
	orr	r0, r0, #(SCTLR_AFE)
	mcr p15, 0, r0, c1, c0, 0
	nop
	nop
	isb

	ldr r0, =vstart
	mov pc, r0
	
vstart:
	nop
	nop
	ldr sp, =sp_idle
	ldr r0, =g_boot_param
	//stmia r0, {r8, r10, r12}
	bl rank_main
	b .
.size reset, .-reset
.endfunc

.func smc_handle
.type smc_handle, %function
smc_handle:
	/*Set to  Non-Secure.*/
	mrc p15, 0, r1, c1, c1, 0
	bic r1, r1, #1
	mcr p15, 0, r1, c1, c1, 0
	movs pc, lr
.size	smc_handle, .-smc_handle
.endfunc

	.align	2
	.type	LC0, #object
LC0:
	.word	LC0
	.word	_sbss
	.word	_ebss
	.word	pt1
	.word	pt2
	.size	LC0, .-LC0

	.section .bss
	.align	14
	.type	pt1, #object
pt1:
	.space	0x4000

	.align	10
	.type	pt2, #object
pt2:
	.space	0x400


