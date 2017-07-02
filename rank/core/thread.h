/*
*Name: thread.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef THREAD_H
#define THREAD_H

#include "list.h"
#include "type.h"
//#include "atomic.h"

typedef void (*entry_func_t)(void *);
typedef struct
{	
	int prio;
	entry_func_t entry;
	void *arg;
}thread_arg_t;

typedef struct
{
	spin_lock_t lock;
	list_t list;
}waitq_t;

typedef struct
{
	spin_lock_t lock;
	int counter;
	uint32_t owner;
	waitq_t wq;
}mutex_t;

extern void thread_exit(void);
extern int idle_thread_create(void);
extern int thread_create(thread_arg_t *);
extern int schedule(void);

extern void waitq_init(waitq_t *); 
extern void wait(waitq_t *);
extern void wake_up(waitq_t *);

extern void mutex_init(mutex_t *);
extern int mutex_lock(mutex_t *);
extern void mutex_unlock(mutex_t *);

#endif

