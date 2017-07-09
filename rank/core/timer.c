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

#define TIMNE_MS (1000)

typedef struct
{
	timeout_handle_t handle;
	void *arg;
}timer_t;

static timer_t g_timer[GPU_TIMER_CHANNELS];
static spin_lock_t g_timer_lock;

static inline uint32_t get_channel(uint32_t cs)
{
	uint32_t channel;
	
	while((cs&1) == 0)
	{
		channel++;
		cs >>= 1;
	}

	return channel;
}

int set_timer(uint32_t m_sec, timeout_handle_t handle, void *arg, int channel)
{
	uint32_t interval;
	uint32_t counter;
	uint32_t flag;

	if(channel < 0 || channel > 3)
	{
		return -1;
	}

	if(handle == NULL)
	{
		return -1;
	}

	spin_lock_irqsave(&g_timer_lock, &flag);
	g_timer[channel].handle = handle;
	g_timer[channel].arg = arg;
	spin_unlock_irqrestore(&g_timer_lock, flag);
	
	interval = m_sec * TIMNE_MS;
	counter = readl(GPU_TIMER_BASE + GPU_TIMER_CLO);
	counter += interval;
	writel(counter, GPU_TIMER_BASE + GPU_TIMER_C0 + (channel << 2));

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

void enable_timer_irq(void)
{
	uint32_t v; 
	
	v = readl(INTERRUPT_BASE + IRQ_ENABLE1);
	writel(v | 2, INTERRUPT_BASE + IRQ_ENABLE1);
}

void disable_timer_irq(void)
{	
	uint32_t v; 
	
	v = readl(INTERRUPT_BASE + IRQ_ENABLE1);
	writel(v & ~2, INTERRUPT_BASE + IRQ_ENABLE1);
}

/*interrupt handle*/
static void timer_irq_handle(void)
{
	uint32_t channel;
	uint32_t cs;
	timer_t *timer;
	timeout_handle_t handle;

	cs = readl(GPU_TIMER_BASE + GPU_TIMER_CS);
	if(cs == 0)
	{
		return;
	}
	
	channel = get_channel(cs);
	
	spin_lock(&g_timer_lock);
	timer = &g_timer[channel];
	handle = timer->handle;
	spin_lock(&g_timer_lock);
	
	if(handle)
	{
		handle(timer->arg);
	}
}

void timer_init(void)
{
	int i;

	disable_timer_irq();

	set_timer_counter(0);
	for(i = 0; i < GPU_TIMER_CHANNELS; i++)
	{
		writel(0, GPU_TIMER_BASE + GPU_TIMER_C0 + (i << 2));
	}
	writel(0xf, GPU_TIMER_BASE + GPU_TIMER_CS);

	irq_register(GPU_TIMER_IRQ, timer_irq_handle);

	g_timer_lock = 0;
}


