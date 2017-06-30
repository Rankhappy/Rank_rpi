/*
*Name: core.S
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "arm.h"

.text
.align	5
.global get_cpuid
.func get_cpuid
.type get_cpuid, %function
get_cpuid:
	mrc p15, 0, r0, c0, c0, 5
	and r0, r0, #0xff
	and r1, r0, #0xff00
	add r0, r0, r1, lsr #6
	mov pc, lr
.size get_cpuid, .-get_cpuid
.endfunc

.global spin_lock
.func spin_lock
.type spin_lock, %function
spin_lock:
	ldrex r1, [r0]
	cmp r1, #0
	wfene
	moveq r1, #1
	strexeq r2, r1, [r0]
	cmpeq r2, #0
	bne spin_lock
	dmb
	mov pc, lr
.size spin_lock, .-spin_lock
.endfunc

.global spin_unlock
.func spin_unlock
.type spin_unlock, %function
spin_unlock:
	isb
	dmb
	mov r1, #0
	str r1, [r0]
	dmb
	sev
	mov pc, lr
.size spin_unlock, .-spin_unlock
.endfunc

.global invclean_dcache_byva
.func invclean_dcache_byva
.type invclean_dcache_byva, %function
invclean_dcache_byva:
	cmp r0, r1
	bhi exit
	mov r2, #0
	mcr p15, 2, r2, c0, c0, 0 //set CSSELR, select	level1 cache&Data or unified cache.
	isb
	bic r0, r0, #0x1f
nextline:
	mcr p15, 0, r0, c7, c14, 1//Clean and Invalidate data or unified cache line by MVA to PoC
	add r0, r0, #(1<<5)
	cmp r1, r0
	bpl nextline
exit:
	dsb
	mov pc, lr
.size invclean_dcache_byva, .-invclean_dcache_byva
.endfunc

.global invclean_all_dcache
.func invclean_all_dcache
.type invclean_all_dcache, %function
invclean_all_dcache:
	push	{r4-r12}
	mrc p15, 1, r0, c0, c0, 1	  //Read CLIDR into R0
	ands r3, r0, #0x07000000 
	mov r3, r3, lsr #23 		  //Cache level value (naturally aligned)
	beq finished
	mov r10, #0
loop1:
	add r2, r10, r10, lsr #1	//Work out 3 x cachelevel
	mov r1, r0, lsr r2		 //bottom 3 bits are the Cache type for this level
	and r1, r1, #7		 //get those 3 bits alone
	cmp r1, #2
	blt skip	   //no cache or only instruction cache at this level
	mcr p15, 2, r10, c0, c0, 0 //write CSSELR from R10
	isb 	 //ISB to sync the change to the CCSIDR
	mrc p15, 1, R1, c0, c0, 0 //read current CCSIDR to R1
	and r2, r1, #7 //extract the line length field
	add r2, r2, #4 //add 4 for the line length offset (log2 16 bytes)
	ldr r4, =0x3FF
	ands r4, r4, r1, lsr #3 //R4 is the max number on the way size (right aligned)
	clz r5, r4 //R5 is the bit position of the way size increment
	ldr r7, =0x00007FFF
	ands r7, r7, r1, lsr #13 //R7 is the max number of the index size (right aligned)
loop2:
	mov r9, r4	//R9 working copy of the max way size (right aligned)
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
	pop {r4-r12}
	mov pc, lr
.size invclean_all_dcache, .-invclean_all_dcache
.endfunc

.global get_sp
.func get_sp
.type get_sp, %function
get_sp:
	mov r0, sp
	mov pc, lr
.size get_sp, .-get_sp
.endfunc

.global get_thread_id
.func get_thread_id
.type get_thread_id, %function
get_thread_id:
	mrc p15, 0, r0, c13, c0, 4
	mov pc, lr
.size get_thread_id, .-get_thread_id
.endfunc

.global set_thread_id
.func set_thread_id
.type set_thread_id, %function
set_thread_id:
	mcr p15, 0, r0, c13, c0, 4
	mov pc, lr
.size set_thread_id, .-set_thread_id
.endfunc

.global context_switch
.func context_switch
.type context_switch, %function
context_switch:
	stmia r0, {r4-r14}
	mrs r2, cpsr
	str r2, [r0, #(4*11)]
	mrc p15, 0, r2, c13, c0, 4
	str r2, [r0, #(4*12)]
	
	ldmia r1, {r4-r14}
	ldr r2, [r1, #(4*11)]
	msr cpsr_cxsf, r2
	ldr r2, [r1, #(4*12)]
	mcr p15, 0, r2, c13, c0, 4

	push {lr}
	bl cpu_unlock
	pop {lr}
	
	mov pc, lr
.size context_switch, .-context_switch
.endfunc	

.global thread_entry
.func thread_entry
.type thread_entry, %function
thread_entry:
	mov r0, r5
	ldr lr, =thread_exit
	mov pc, r4
.size thread_entry, .-thread_entry
.endfunc



