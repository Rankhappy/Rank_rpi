/*
*Name: timer.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef TIMER_H
#define TIMER_H

typedef void (*timeout_handle_t)(void *);

extern int set_timer(uint32_t, timeout_handle_t, void *, int);
extern uint64_t get_timer_counter(void);
extern void set_timer_counter(uint64_t);
extern uint32_t get_timer_lcounter(void);
extern void enable_timer_irq(void);
extern void disable_timer_irq(void);
extern void timer_init(void);

#endif


