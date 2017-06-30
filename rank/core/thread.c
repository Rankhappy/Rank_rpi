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

#define STACK_SIZE 1024

#define thdbg(fmt, args...) printf("[RANK][THREAD][DEBUG]"fmt, ##args)
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
	STAT_DEAD
}thread_stat_t;

typedef struct
{
	int prio;
	uint32_t cpu_mask;
	thread_stat_t stat;
	heap_node_t node;
	cpu_ctx_t cpu_ctx;
}thread_t;

static heap_t g_thead_heap;

static int thread_prio_comp(heap_node_t *n1, heap_node_t *n2)
{
	thread_t *th1, *th2;

	th1 = heap_data(n1, thread_t, node);
	th2 = heap_data(n2, thread_t, node);

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
extern void cpu_lock(void);
extern void cpu_unlock(void);
void thread_exit(void)
{
	thread_t *th;
	
	th = (thread_t *)get_thread_id();
	th->stat = STAT_DEAD;

	cpu_lock();
	thdbg("thread_exit:th = 0x%08x, stat = %d\n", (uint32_t)th, th->stat);

	schedule();

	cpu_unlock();
	//rfree(th);
}

int idle_thread_create(thread_arg_t *thread_arg)
{
	thread_t *th;

	th = rmalloc(sizeof(thread_t));
	if(th == NULL)
	{
		return -1;
	}
	
	th->cpu_mask = thread_arg->cpu_mask;
	th->prio = thread_arg->prio;
	th->stat = STAT_RUNNING;

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

	th->cpu_mask = thread_arg->cpu_mask;
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
	
	heap_node_init(&th->node);
	heap_insert(&g_thead_heap, &th->node, thread_prio_comp);

	return 0;
}

int schedule(void)
{
	thread_t *old_th;
	thread_t *new_th;
	heap_node_t *n;
	uint32_t cpuid = get_cpuid();

	//thdbg("schedule:cpu_id = %d\n", cpuid);

next:
	n = heap_get(&g_thead_heap);
	if(n == NULL)
	{
		return -1;
	}
	new_th = heap_data(n, thread_t, node);
	thdbg("schedule:new_th = 0x%08x, stat = %d\n", (uint32_t)new_th, new_th->stat);
	if(new_th->stat == STAT_DEAD)
	{
		thdbg("schedule:remove dead thread.\n");
		heap_delete(&g_thead_heap, thread_prio_comp);
		rfree(new_th);
		goto next;
	}
	
	if((new_th->cpu_mask) & (1<<cpuid))
	{
		return -1;
	}
	heap_delete(&g_thead_heap, thread_prio_comp);
	
	old_th = (thread_t *)get_thread_id();
	heap_node_init(&old_th->node);
	heap_insert(&g_thead_heap, &old_th->node, thread_prio_comp);

	thdbg("schedule:new_th = 0x%08x, old_th = 0x%08x\n", (uint32_t)new_th, (uint32_t)old_th);
	new_th->stat = STAT_RUNNING;
	if(old_th->stat == STAT_RUNNING)
	{
		old_th->stat = STAT_READYTORUN;
	}

	context_switch((uint32_t)&old_th->cpu_ctx, (uint32_t)&new_th->cpu_ctx);

	return 0;
}

