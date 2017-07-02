/*
*Name: thread.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "arm.h"
#include "type.h"
#include "heap.h"
#include "core.h"
#include "thread.h"
#include "mm.h"
#include "assert.h"
//#include "atomic.h"

#define STACK_SIZE 1024
#define CPU_NUM 4

#define MUTEX_UNLOCK 1
#define MUTEX_LOCKED 0

#define thdbg(fmt, args...) //printf("[RANK][THREAD][DEBUG]"fmt, ##args)
#define therr(fmt, args...) printf("[RANK][THREAD][ERROR]"fmt, ##args)

typedef struct
{
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t cpsr;
	uint32_t tpidrprw;
}cpu_ctx_t;

typedef enum
{
	STAT_READYTORUN = 0,
	STAT_RUNNING,
	STAT_SLEEP,
	STAT_DEAD
}thread_stat_t;

typedef enum
{
	FLAG_IDLE = 0,
	FLAG_NORMAL,
}thread_flag_t;

typedef struct
{
	int prio;
	thread_stat_t stat;
	thread_flag_t flag;
	heap_node_t rnode;
	list_node_t wnode;
	cpu_ctx_t cpu_ctx;
}thread_t;

static heap_t g_thead_heap;
static thread_t g_idle_thread[CPU_NUM];
spin_lock_t g_heap_lock;

static int thread_prio_comp(heap_node_t *n1, heap_node_t *n2)
{
	thread_t *th1, *th2;

	th1 = heap_data(n1, thread_t, rnode);
	th2 = heap_data(n2, thread_t, rnode);

	if(th1->prio < th2->prio)
	{
		return -1;
	}
	
	if(th1->prio > th2->prio)
	{
		return 1;
	}

	return 0;
}

void thread_heap_lock(void)
{
	//spin_lock(&g_heap_lock);
}

void thread_heap_unlock(void)
{
	//spin_unlock(&g_heap_lock);
}

void thread_exit(void)
{
	thread_t *th;
	
	th = (thread_t *)get_thread_id();
	
	th->stat = STAT_DEAD;

	thdbg("thread_exit:th = 0x%08x, stat = %d\n", (uint32_t)th, th->stat);
	
	schedule();
}

int idle_thread_create(void)
{
	thread_t *th;
	uint32_t cpuid = get_cpuid();
	
	th = &g_idle_thread[cpuid];

	thdbg("idle_thread_create:th = 0x%08x\n", (uint32_t)th);
	
	th->stat = STAT_RUNNING;
	th->flag = FLAG_IDLE;
	
	set_thread_id((uint32_t)th);

	return 0;
}

int thread_create(thread_arg_t *thread_arg)
{
	thread_t *th;

	th = rmalloc(sizeof(thread_t));
	if(th == NULL)
	{
		return -1;
	}
	
	thdbg("thread_create:th = 0x%08x\n", (uint32_t)th);

	th->prio = thread_arg->prio;
	th->cpu_ctx.r13 = (uint32_t)stack_alloc();
	if(th->cpu_ctx.r13 == 0)
	{
		return -1;
	}
	th->cpu_ctx.r13 += STACK_SIZE;
	th->cpu_ctx.r14 = (uint32_t)thread_entry;
	th->cpu_ctx.cpsr = PSR_MODE_SVC | PSR_I_BIT | PSR_F_BIT | PSR_A_BIT;
	th->cpu_ctx.tpidrprw = (uint32_t)th;
	th->cpu_ctx.r4 = (uint32_t)thread_arg->entry;
	th->cpu_ctx.r5 = (uint32_t)thread_arg->arg;
	th->stat = STAT_READYTORUN;
	th->flag = FLAG_NORMAL;
	heap_node_init(&th->rnode);

	spin_lock(&g_heap_lock);
	heap_insert(&g_thead_heap, &th->rnode, thread_prio_comp);
	spin_unlock(&g_heap_lock);

	return 0;
}

int schedule(void)
{
	thread_t *old_th;
	thread_t *new_th;
	heap_node_t *n;
	uint32_t cpuid = get_cpuid();

	//thdbg("schedule:cpu_id = %d\n", cpuid);
	old_th = (thread_t *)get_thread_id();
	assert(old_th->stat != STAT_READYTORUN);

next:
	spin_lock(&g_heap_lock);
	n = heap_get(&g_thead_heap);
	if(n == NULL)
	{
		spin_unlock(&g_heap_lock);
		if(old_th->flag == FLAG_IDLE)
		{
			return -1;
		}
		new_th = &g_idle_thread[cpuid];
		assert(new_th->stat == STAT_READYTORUN);
	}
	else
	{
		heap_delete(&g_thead_heap, thread_prio_comp);
		spin_unlock(&g_heap_lock);
		new_th = heap_data(n, thread_t, rnode);
		assert(new_th->stat != STAT_RUNNING);
		thdbg("schedule:new_th = 0x%08x, stat = %d\n", (uint32_t)new_th, new_th->stat);
		if(new_th->stat == STAT_DEAD)
		{
			thdbg("schedule:remove dead thread.\n");
			rfree(new_th);
			goto next;
		}
	}

	if((old_th->flag == FLAG_NORMAL) && (old_th->stat != STAT_SLEEP))
	{
		heap_node_init(&old_th->rnode);
		spin_lock(&g_heap_lock);
		heap_insert(&g_thead_heap, &old_th->rnode, thread_prio_comp);
		spin_unlock(&g_heap_lock);
	}

	thdbg("schedule:new_th = 0x%08x, old_th = 0x%08x\n", (uint32_t)new_th, (uint32_t)old_th);
	thdbg("schedule:new_tpid = 0x%08x, old_tpid = 0x%08x\n", new_th->cpu_ctx.tpidrprw, old_th->cpu_ctx.tpidrprw);
	new_th->stat = STAT_RUNNING;
	if(old_th->stat == STAT_RUNNING)
	{
		old_th->stat = STAT_READYTORUN;
	}
	
	context_switch((uint32_t)&old_th->cpu_ctx, (uint32_t)&new_th->cpu_ctx);

	return 0;
}

void waitq_init(waitq_t *wq)
{
	wq->lock = 0;
	list_init(&wq->list);
}

void wait(waitq_t *wq)
{
	thread_t *cur_th;

	cur_th = (thread_t *)get_thread_id();
	thdbg("wait:get cur_th = 0x%08x.\n", (uint32_t)cur_th); 
	
	assert(cur_th->stat == STAT_RUNNING);

	thdbg("wait:th = 0x%08x, flag = %d.\n", (uint32_t)cur_th, cur_th->flag); 

	/*The idle thread can not wait.*/
	if(cur_th->flag == FLAG_IDLE)
	{
		return;
	}

	cur_th->stat = STAT_SLEEP;
	
	list_init(&cur_th->wnode);
	spin_lock(&wq->lock);
	list_add_tail(&wq->list, &cur_th->wnode);
	spin_unlock(&wq->lock);

	schedule();

	thdbg("wait:wake:th = 0x%08x, flag = %d.\n", (uint32_t)cur_th, cur_th->flag); 
}

void wake_up(waitq_t *wq)
{
	list_node_t *wait_node;
	thread_t *th;
	
	spin_lock(&wq->lock);
	
	list_foreach(&wq->list, wait_node)
	{
		th = list_data(wait_node, thread_t, wnode);
		thdbg("wake_up:th = 0x%08x, flag = %d.\n", (uint32_t)th , th ->flag); 
		assert(th->stat == STAT_SLEEP);
		th->stat = STAT_READYTORUN;
		heap_node_init(&th->rnode);
		spin_lock(&g_heap_lock);
		heap_insert(&g_thead_heap, &th->rnode, thread_prio_comp);
		spin_unlock(&g_heap_lock);
	}
	list_init(&wq->list);
	
	spin_unlock(&wq->lock);
}

void mutex_init(mutex_t *mux)
{
	mux->lock = 0;
	mux->counter = MUTEX_UNLOCK;
	mux->owner = 0;
	waitq_init(&mux->wq);
}

int mutex_lock(mutex_t *mux)
{
	thdbg("mutex_lock:mux = 0x%08x.\n", (uint32_t)mux);

	while(1)
	{
		spin_lock(&mux->lock);
		if(mux->counter == MUTEX_UNLOCK)
		{
			mux->counter = MUTEX_LOCKED;
			mux->owner = get_thread_id();
			spin_unlock(&mux->lock);
			return 0;
		}
		else
		{
			if(mux->owner == get_thread_id())
			{
				return 1;
			}
		}
		spin_unlock(&mux->lock);
	
		wait(&mux->wq);
	}
}

void mutex_unlock(mutex_t *mux)
{
	thdbg("mutex_unlock:mux = 0x%08x.\n", (uint32_t)mux);
	
	spin_lock(&mux->lock);
	assert(mux->owner == get_thread_id());
	assert(mux->counter == MUTEX_LOCKED);
	mux->counter = MUTEX_UNLOCK;
	mux->owner = 0;
	spin_unlock(&mux->lock);
	
	wake_up(&mux->wq);
}

