/*
*Name: entry.S
*Author: Rank 
*Contact:<441552318@qq.com>
*/

/*
*for 32bits on armv8 architecture.
*/

#include "arm.h"

.text
.align	5
.global __start
.func   __start
.type	__start, %function

__start:
	b	reset
	b	.							/* Undef */
	b	smc_handle					/* Syscall */
	b	.							/* Prefetch abort */
	b	data_abort_handle			/* Data abort */
	b	.							/* Reserved */
	b	.							/* IRQ */
	b	.							/* FIQ */
	
.size	__start, .-__start
.endfunc

.global start_secondary_cpu
.func start_secondary_cpu
.type start_secondary_cpu, %function
start_secondary_cpu:
	ldr r1, =reset
	ldr r2, =g_v2p_off
	ldr r2, [r2]
	sub r1, r1, r2
	mov r2, #0xf0000000
	cmp r0, #1
	streq r1, [r2,#0x9c]
	beq skip_3
	cmp r0, #2
	streq r1, [r2,#0xac]
	beq skip_3
	cmp r0, #3
	streq r1, [r2,#0xbc]
skip_3:
	mov pc, lr
.size start_secondary_cpu, .-start_secondary_cpu
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
	*it seems can not modify the mode of cpsr directly to switch to svc,
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
	ldmia r6, {r0, r5, r8, r10, r11}
	sub r12, r0, r6  //r12 = offset of vir-phy
	sub r7, r10, r12 //r7 = p1 phy addr	
	
	bl get_cpuid
	cmp r0, #0
	bne skip_1
	
	sub r5, r5, r12
	sub r4, r8, r12
	/*clear bss, r1 = _sbss, r8 = _ebss*/
	mov	r2, #0
	mov	r3, #0
clear_bss:
	stmia r5!, {r2, r3}
	cmp	r5, r4
	bls	clear_bss
	dmb

	/*set page table*/
	mov r1, pc
	bic r5, r1, #0xff 
	bic r5, r5, #0xf00 //r5 = phy addr
	add r4, r5, r12 //r4 = vir addr
	sub r6, r11, r12 //r6 = p2 phy addr
	//sub r7, r10, r12 //r7 = p1 phy addr	
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

	/*save the booting param*/
	ldr r0, =g_boot_param
	sub r0, r0, r12
	stmia r0, {r8, r10, r12}

	/*
	*set ttbr ttbcr
	*r7 = p1 phy addr
	*/
skip_1:
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

	/*enable actlr smp*/
	mrc p15, 0, r0, c1, c0, 1
	orr r0, r0, #(1<<6)
	mcr p15, 0, r0, c1, c0, 1

	/*invalidate icache bp and tlbs*/
	mov	r0, #0
	mcr p15, 0, r0, c8, c7, 0 //Invalidate TLBs
	mcr p15, 0, r0, c8, c6, 0// Invalidate Data TLBs.
	mcr p15, 0, r0, c8, c5, 0 // Invalidate Instruction TLBs.
	mcr p15, 0, r0, c7, c5, 6 //Invalidate all branch predictors.
	mcr p15, 0, r0, c7, c5, 0 //Invalidate all instruction caches to PoU, invalidate branch predictors.
	dsb
	isb

	adr sp, sp_temp
	bl get_cpuid
	sub sp, sp, r0, lsl #6
	bl invclean_all_dcache

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
	bl get_cpuid
	cmp r0, #0
	ldreq sp, =sp_idle
	ldreq r0, =g_boot_param
	ldrne sp, =sp_secondary
	subne sp, sp, r0, lsl #6
	bleq rank_main
	blne secondary_cpu
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

.func data_abort_handle
.type data_abort_handle, %function
data_abort_handle:
	ldr sp, =sp_except
	sub lr, lr, #8
	str lr, [sp]
	mrs lr, spsr
	str lr, [sp, #4]

	bic	lr, lr, #(PSR_MODE_MASK)
	orr	lr, lr, #(PSR_MODE_SVC | PSR_I_BIT | PSR_F_BIT)
	msr	cpsr_c, lr

	sub	sp, sp, #(4*17) //r0~r14, abt_r14(pc), abt_spsr(cpsr)
	stmia sp, {r0-r12}

	ldr	r0, =sp_except
	ldmia r0, {r3, r4}

	add	r1, sp, #(4*17)
	mov	r2, lr
	add	r0, sp, #(4*13)
	stmia r0, {r1-r4}

	mov	fp, #0
	mov	r0, sp
	mrc	p15, 0, r1, c6, c0, 0 //dfar
	mrc	p15, 0, r2, c5, c0, 0 //dfsr
	mov	r4, sp
	bic	sp, sp, #7
	bl data_abort_dump
	mov	sp, r4
	ldr	r1, [r4, #(4*16)]
	msr	spsr_cxsf, r1
	b . //stop as now.
	ldmia r0, {r1-r15}^
.size data_abort_handle, .-data_abort_handle
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
	
	.align	3
	.type	sp_temp, #object
	.space	4*64	
sp_temp:
	.size sp_temp, .-sp_temp
	
	.section .bss
	.align	3
	.type	sp_idle, #object
	.space	4096	
sp_idle:
	.size sp_idle, .-sp_idle

	.align	3
	.type	sp_except, #object
	.space	8	
sp_except:
	.size sp_except, .-sp_except

	.align	3
	.type	sp_secondary, #object
	.space	4*64	
sp_secondary:
	.size sp_secondary, .-sp_secondary

	.align	14
	.type	pt1, #object
pt1:
	.space	0x4000
	.size pt1, .-pt1

	.align	10
	.type	pt2, #object
pt2:
	.space	0x400
	.size pt2, .-pt2


