/*
*Name: irq.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/
#include "core.h"
#include "list.h"
#include "mm.h"
#include "board.h"
#include "irq.h"

#define irqdbg(fmt, args...) printf("[RANK][IRQ][DEBUG]"fmt, ##args)
#define irqerr(fmt, args...) printf("[RANK][IRQ][ERROR]"fmt, ##args)

typedef struct
{
	int irq_num;
	irq_handle_t handle;
	list_node_t node;
}irq_t;

static list_t g_irq_list;
static spin_lock_t g_irq_lock;

static inline int get_num(uint32_t pending)
{
	int num = 0;
	
	while((pending&1) == 0)
	{
		num++;
		pending >>= 1;
	}
	
	return num;
}

void irq_init(void)
{
	list_init(&g_irq_list);
	g_irq_lock = 0;

#if 0
	writel(0, INTERRUPT_BASE + FIQ_CONTROL); //disalbe fiq
	/*disable all of the irqs.*/
	writel(0, INTERRUPT_BASE + IRQ_ENABLE1);
	writel(0, INTERRUPT_BASE + IRQ_ENABLE2);
	writel(0, INTERRUPT_BASE + IRQ_BASIC_ENABLE);
#endif

}

void enable_irq(int irq_no)
{
	uint32_t v; 

	irqdbg("enable_irq, irq_no = %d.\n", irq_no);

	if((irq_no >= 0) && (irq_no <= 31))
	{
		v = readl(INTERRUPT_BASE + IRQ_ENABLE1);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_ENABLE1);
	}
	else if((irq_no >= 32) && (irq_no <= 63))
	{
		irq_no -= 32;
		v = readl(INTERRUPT_BASE + IRQ_ENABLE2);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_ENABLE2);
	}
	else if((irq_no >= 64) && (irq_no <= 71))
	{
		irq_no -= 64;
		v = readl(INTERRUPT_BASE + IRQ_BASIC_ENABLE);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_BASIC_ENABLE);
	}
	else
	{
		irqerr("Wrong irq no, irq_no = %d.\n", irq_no);
	}
}

void disable_irq(int irq_no)
{
	uint32_t v; 

	irqdbg("disable_irq, irq_no = %d.\n", irq_no);

	if((irq_no >= 0) && (irq_no <= 31))
	{
		v = readl(INTERRUPT_BASE + IRQ_DISABLE1);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_DISABLE1);
	}
	else if((irq_no >= 32) && (irq_no <= 63))
	{
		irq_no -= 32;
		v = readl(INTERRUPT_BASE + IRQ_DISABLE2);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_DISABLE2);
	}
	else if((irq_no >= 64) && (irq_no <= 71))
	{
		irq_no -= 64;
		v = readl(INTERRUPT_BASE + IRQ_BASIC_DISABLE);
		writel(v | (1<<irq_no), INTERRUPT_BASE + IRQ_BASIC_DISABLE);
	}
}

int irq_register(int irq_num, irq_handle_t handle)
{
	uint32_t flag;

	irq_t *irq = rmalloc(sizeof(irq_t));
	if(irq == NULL)
	{
		return -1;
	}

	irq->handle = handle;
	irq->irq_num = irq_num;
	list_init(&irq->node);
	spin_lock_irqsave(&g_irq_lock, &flag);
	list_add_tail(&g_irq_list, &irq->node);
	spin_unlock_irqrestore(&g_irq_lock, flag);
	
	return 0;
}

/*interrupt handler*/
void irq_dispatch(void)
{
	uint32_t pending;
	int irq_num;
	irq_t *irq;
	list_node_t *irq_node;

	irqdbg("irq_dispatch.\n");

	pending = readl(INTERRUPT_BASE + IRQ_PENDING1);
	while(pending)
	{
		irq_num = get_num(pending);
		pending &= ~(1<<irq_num);
		spin_lock(&g_irq_lock);
		list_foreach(&g_irq_list, irq_node)
		{
			irq = list_data(irq_node, irq_t, node);
			if(irq->irq_num == irq_num)
			{
				spin_unlock(&g_irq_lock);
				irq->handle();
				spin_lock(&g_irq_lock);
			}
		}
		spin_unlock(&g_irq_lock);
	}

	pending = readl(INTERRUPT_BASE + IRQ_PENDING2);
	while(pending)
	{
		irq_num = get_num(pending);
		pending &= ~(1<<irq_num);
		irq_num += 32;
		spin_lock(&g_irq_lock);
		list_foreach(&g_irq_list, irq_node)
		{
			irq = list_data(irq_node, irq_t, node);
			if(irq->irq_num == irq_num)
			{
				spin_unlock(&g_irq_lock);
				irq->handle();
				spin_lock(&g_irq_lock);
			}
		}
		spin_unlock(&g_irq_lock);
	}
	
	pending = readl(INTERRUPT_BASE + IRQ_BASIC_PENDING);
	while(pending)
	{
		irq_num = get_num(pending);
		pending &= ~(1<<irq_num);
		if(irq_num == 10)
		{
			irq_num = 7;
		}
		else if((irq_num >= 11) && (irq_num <= 12))
		{
			irq_num -= 2;
		}
		else if((irq_num >= 13) && (irq_num <= 14))
		{
			irq_num += 5;
		}
		else if((irq_num >= 15) && (irq_num <= 19))
		{
			irq_num += 38;
		}
		else
		{
			irq_num += 64;
		}
		spin_lock(&g_irq_lock);
		list_foreach(&g_irq_list, irq_node)
		{
			irq = list_data(irq_node, irq_t, node);
			if(irq->irq_num == irq_num)
			{
				spin_unlock(&g_irq_lock);
				irq->handle();
				spin_lock(&g_irq_lock);
			}
		}
		spin_unlock(&g_irq_lock);
	}
}


