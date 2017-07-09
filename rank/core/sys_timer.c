/*
*Name: sys_timer.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/
#include "core.h"
#include "list.h"
#include "timer.h"
#include "sys_timer.h"
#include "assert.h"

typedef enum
{
	STAT_IDLE,
	STAT_BUSY
}timer_stat_t;

typedef struct
{
	spin_lock_t lock;
	heap_t h;
	timer_stat_t stat;
	uint32_t last_counter;
	uint32_t base_counter;
}sys_timer_t;

static void sys_timer_timeout(void *arg);

#define set_sys_timer(x) set_timer(x, sys_timer_timeout, NULL, 0)

static sys_timer_t g_sys_timer;

static int timer_timeout_comp(heap_node_t *n1, heap_node_t *n2)
{
	timer_entity_t *te1, *te2;

	te1 = heap_data(n1, timer_entity_t, node);
	te2 = heap_data(n2, timer_entity_t, node);

	if(te1->timeout< te2->timeout)
	{
		return -1;
	}
	
	if(te1->timeout > te2->timeout)
	{
		return 1;
	}

	return 0;
}

void sys_timer_init(void)
{
	g_sys_timer.last_counter = 0;
	g_sys_timer.base_counter = 0;
	g_sys_timer.lock = 0;
	g_sys_timer.stat = STAT_IDLE;
	heap_init(&g_sys_timer.h);
}

void init_timer_entity(timer_entity_t *te)
{
	te->handle = NULL;
	te->arg = NULL;
	te->timeout = 0;
	heap_node_init(&te->node);
}

void sys_timer_add(timer_entity_t *te)
{
	uint32_t counter;
	uint32_t flag;

	spin_lock_irqsave(&g_sys_timer.lock, &flag);

	if(g_sys_timer.stat == STAT_IDLE)
	{
		g_sys_timer.base_counter = 0;
		g_sys_timer.last_counter = get_timer_lcounter();
		heap_insert(&g_sys_timer.h, &te->node, timer_timeout_comp);
		set_sys_timer(te->timeout);
		g_sys_timer.stat = STAT_BUSY;
		spin_unlock_irqrestore(&g_sys_timer.lock, flag);
		enable_timer_irq();
		return;
	}

	assert(g_sys_timer.stat == STAT_BUSY);

	counter = get_timer_lcounter();
	g_sys_timer.base_counter += counter - g_sys_timer.last_counter;
	g_sys_timer.last_counter = counter;
	te->timeout += g_sys_timer.base_counter;
	heap_insert(&g_sys_timer.h, &te->node, timer_timeout_comp);
	if(heap_get(&g_sys_timer.h) == &te->node)
	{
		set_sys_timer(te->timeout-g_sys_timer.base_counter);
	}
	
	spin_unlock_irqrestore(&g_sys_timer.lock, flag);
}

/*interrupt handle*/
static void sys_timer_timeout(void *arg)
{
	timer_entity_t *te, *next_te;
	heap_node_t *n;
	uint32_t counter;

	spin_lock(&g_sys_timer.lock);

	n = heap_get(&g_sys_timer.h);
	assert(n);
	
	counter = get_timer_lcounter();
	g_sys_timer.base_counter += counter - g_sys_timer.last_counter;
	g_sys_timer.last_counter = counter;
	te = heap_data(n, timer_entity_t, node);
	assert(te->timeout <= g_sys_timer.base_counter);
	heap_delete(&g_sys_timer.h, timer_timeout_comp);

	n = heap_get(&g_sys_timer.h);
	if(n == NULL)
	{
		g_sys_timer.base_counter = 0;
		g_sys_timer.last_counter = 0;
		g_sys_timer.stat = STAT_IDLE;
		disable_timer_irq();
	}
	else
	{
		next_te = heap_data(n, timer_entity_t, node);
		assert(next_te->timeout >= g_sys_timer.base_counter);
		set_sys_timer(next_te->timeout-g_sys_timer.base_counter);
	}

	spin_unlock(&g_sys_timer.lock);

	te->handle(te->arg);
}

