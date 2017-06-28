/*
*Name: core.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

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
#endif

