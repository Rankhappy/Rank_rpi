/*
*Name: sys_timer.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include "type.h"
#include "timer.h"
#include "heap.h"

typedef struct
{
	timeout_handle_t handle;
	void *arg;
	uint32_t timeout;
	heap_node_t node;
}timer_entity_t;

extern void sys_timer_init(void);
extern void init_timer_entity(timer_entity_t *);
extern void sys_timer_add(timer_entity_t *);

#endif

