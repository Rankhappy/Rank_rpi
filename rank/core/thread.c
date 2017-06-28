/*
*Name: thread.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "type.h"
#include "heap.h"
#include "core.h"
#include "thread.h"
#include "mm.h"

#define STACK_SIZE 1024

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

typedef struct
{
	int prio;
	uint32_t cpu_mask;
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

void thread_exit(void)
{
	thread_t *th;
	
	th = (thread_t *)get_thread_id();

	rfree(th);
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
	th->cpu_ctx.tpidrprw = (uint32_t)th;
	th->cpu_ctx.r4 = (uint32_t)thread_arg->entry;
	th->cpu_ctx.r5 = (uint32_t)thread_arg->arg;
	
	heap_node_init(&th->node);
	heap_insert(&g_thead_heap, &th->node, thread_prio_comp);

	return 0;
}

void schedule(void)
{
	thread_t *old_th;
	thread_t *new_th;
	heap_node_t *n;
	uint32_t cpuid = get_cpuid();
	
	n = heap_get(&g_thead_heap);
	if(n == NULL)
	{
		return;
	}
	new_th = heap_data(n, thread_t, node);
	if(new_th->cpu_mask & (1<<cpuid))
	{
		return;
	}
	heap_delete(&g_thead_heap, thread_prio_comp);
	
	old_th = (thread_t *)get_thread_id();
	heap_node_init(&old_th->node);
	heap_insert(&g_thead_heap, &old_th->node, thread_prio_comp);

	context_switch((uint32_t)&old_th->cpu_ctx, (uint32_t)&new_th->cpu_ctx);
}

