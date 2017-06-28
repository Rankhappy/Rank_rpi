/*
*Name: thread.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef THREAD_H
#define THREAD_H

typedef void (*entry_func_t)(void *);
typedef struct
{	
	int prio;
	uint32_t cpu_mask;
	entry_func_t entry;
	void *arg;
}thread_arg_t;

extern void thread_exit(void);
extern int idle_thread_create(thread_arg_t *);
extern int thread_create(thread_arg_t *);
extern void schedule(void);
#endif

