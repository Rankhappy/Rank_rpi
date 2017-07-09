/*
*Name: core.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef CORE_H
#define CORE_H

#include "type.h"

extern uint32_t get_cpuid(void);
extern void spin_lock(uint32_t *);
extern void spin_unlock(uint32_t *);
extern void invclean_all_dcache(void);
extern void invclean_dcache_byva(uint32_t, uint32_t);
extern uint32_t get_sp(void);
extern uint32_t get_thread_id(void);
extern void set_thread_id(uint32_t);
extern void context_switch(uint32_t, uint32_t);
extern void thread_entry(void);
extern void enable_local_irq(void);
extern void restore_local_irq(uint32_t flag);
extern uint32_t disable_local_irq(void);

static void spin_lock_irqsave(uint32_t *lock, uint32_t *pflag)
{
	uint32_t flag;

	flag = disable_local_irq();

	spin_lock(lock);

	*pflag = flag;
}

static void spin_unlock_irqrestore(uint32_t *lock, uint32_t flag)
{
	spin_unlock(lock);

	restore_local_irq(flag);
}

#endif

