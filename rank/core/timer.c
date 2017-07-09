/*
*Name: timer.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/
#include "core.h"
#include "list.h"
#include "board.h"
#include "irq.h"
#include "timer.h"

extern void printf(const char *fmt, ...);

#define tmdbg(fmt, args...) printf("[RANK][TIMER][DEBUG]"fmt, ##args)
#define tmerr(fmt, args...) printf("[RANK][TIMER][ERROR]"fmt, ##args)
	
typedef struct
{
	timeout_handle_t handle;
	void *arg;
}timer_t;

static timer_t g_timer[GPU_TIMER_CHANNELS];
static spin_lock_t g_timer_lock;

static inline uint32_t get_channel(uint32_t cs)
{
	uint32_t channle = 0;
	
	while((cs&1) == 0)
	{
		channle++;
		cs >>= 1;
	}

	return channle;
}

int set_timer(uint32_t interval, timeout_handle_t handle, void *arg, int channle)
{
	uint32_t counter;
	uint32_t flag;

	tmdbg("set timer interval = %d, channle = %d\n", interval, channle);

	if(channle < 0 || channle > 3)
	{
		tmerr("channle is error, channle = %d.\n", channle);
		return -1;
	}

	if(handle == NULL)
	{
		tmerr("handle is null.\n");
		return -1;
	}

	spin_lock_irqsave(&g_timer_lock, &flag);
	g_timer[channle].handle = handle;
	g_timer[channle].arg = arg;
	spin_unlock_irqrestore(&g_timer_lock, flag);
	
	counter = readl(GPU_TIMER_BASE + GPU_TIMER_CLO);
	counter += interval;
	writel(counter, GPU_TIMER_BASE + GPU_TIMER_C0 + (channle << 2));

	return 0;
}

uint64_t get_timer_counter(void)
{
	uint64_t counter;

	counter = readl(GPU_TIMER_BASE + GPU_TIMER_CHI);
	counter <<= 32;
	counter += readl(GPU_TIMER_BASE + GPU_TIMER_CLO);

	return counter;
}

void set_timer_counter(uint64_t counter)
{
	writel(counter >> 32, GPU_TIMER_BASE + GPU_TIMER_CHI);
	writel(counter&0xffffffff, GPU_TIMER_BASE + GPU_TIMER_CLO);
}

uint32_t get_timer_lcounter(void)
{
	uint32_t counter;
	
	counter = readl(GPU_TIMER_BASE + GPU_TIMER_CLO);

	return counter;
}

void enable_timer_irq(int channle)
{
	tmdbg("enable_timer_irq.\n");

	enable_irq(GPU_TIMER_IRQ + channle);
}

void disable_timer_irq(int channle)
{
	tmdbg("disable_timer_irq.\n");

	writel(1 << channle, GPU_TIMER_BASE + GPU_TIMER_CS);
	disable_irq(GPU_TIMER_IRQ + channle);
}

/*interrupt handle*/
static void timer_irq_handle(void)
{
	uint32_t channle;
	uint32_t cs;
	timer_t *timer;
	timeout_handle_t handle;

	tmdbg("timer_irq_handle.\n");
	
	cs = readl(GPU_TIMER_BASE + GPU_TIMER_CS);
	if(cs == 0)
	{
		tmdbg("no timer irq pending.\n");
		return;
	}

	tmdbg("cs = 0x%08x.\n", cs);

	writel(cs, GPU_TIMER_BASE + GPU_TIMER_CS);
	
	channle = get_channel(cs);

	tmdbg("channle = %d.\n", channle);
	
	spin_lock(&g_timer_lock);
	timer = &g_timer[channle];
	handle = timer->handle;
	spin_unlock(&g_timer_lock);

	tmdbg("handle = 0x%08x.\n", handle);
	
	if(handle)
	{
		handle(timer->arg);
	}
}

void timer_init(int channle)
{
	disable_timer_irq(channle);
	writel(0, GPU_TIMER_BASE + GPU_TIMER_C0 + (channle << 2));
	writel(1<<channle, GPU_TIMER_BASE + GPU_TIMER_CS);

	irq_register(GPU_TIMER_IRQ + channle, timer_irq_handle);

	g_timer_lock = 0;
}


