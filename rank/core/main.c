/*
*Name: main.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "uart.h"
#include "type.h"
#include "arm.h"
#include "mmu.h"
#include "mm.h"
#include "thread.h"
#include "core.h"

#define CPU_NUM 4

#define IO_BASE1_PADDR    0x3f200000
#define IO_BASE1_VADDR    0xef200000

#define IO_BASE2_PADDR    0x40000000
#define IO_BASE2_VADDR    0xf0000000

#define MEMORY_NOMAL(flag)\
{ \
	flag.cacheble = 1; \
	flag.shareble = 1; \
	flag.ap = 0; \
	flag.ns = 0; \
	flag.ng = 0; \
}

#define MEMORY_DEVICE(flag)\
{ \
	flag.cacheble = 0; \
	flag.shareble = 0; \
	flag.ap = 0; \
	flag.ns = 0; \
	flag.ng = 0; \
}

#define DECALRE_PT2_ARRAY(num) \
	addr_t g_pt2_addr##num[1024] __attribute__((aligned(1024)));

#define rdbg(fmt, args...) printf("[RANK][DEBUG]"fmt, ##args)
#define rerr(fmt, args...) printf("[RANK][ERROR]"fmt, ##args)

typedef struct
{
	addr_t ebss;
	addr_t pt1_addr;
	size_t loffset;
}boot_param_t;

static spin_lock_t g_cpu_lock;

boot_param_t g_boot_param;

uint8_t g_pt2_map[2];

#define ARRAY_NUM 2 //two blocks are enough
DECALRE_PT2_ARRAY(0)
DECALRE_PT2_ARRAY(1)

extern addr_t g_pt1_addr;
extern size_t g_v2p_off;

extern void printf(const char *fmt, ...);
extern void print_init(uint32_t base);

int raise(int signo)
{
	return 0;
}

static addr_t alloc_pt2_low(int blocks)
{
	int i;

	addr_t pt2_addr = 0;

	//only support one block now.
	if(blocks != 1)
	{
		return 0;
	}

	for(i = 0; i < ARRAY_NUM; i++)
	{
		if(g_pt2_map[i] == 0)
		{
			break;
		}
	}
	if(i < ARRAY_NUM)
	{
		switch(i)
		{
			case 0:
			pt2_addr = (addr_t)g_pt2_addr0;
			break;
			case 1:
			pt2_addr = (addr_t)g_pt2_addr1;
			break;	
			default:
			break;
		}
	}

	return pt2_addr;
}

static int do_io_map(void)
{
	int ret;

	mmu_flag_t flag;

	MEMORY_DEVICE(flag);

	addr_t vaddr = IO_BASE1_VADDR;
	addr_t paddr = IO_BASE1_PADDR;
	ret = do_mmu_map(vaddr, paddr, SECTION_SIZE, flag, alloc_pt2_low);
	if(ret < 0)
	{
		return ret;
	}

	vaddr = IO_BASE2_VADDR;
	paddr = IO_BASE2_PADDR;
	ret = do_mmu_map(vaddr, paddr, SECTION_SIZE, flag, alloc_pt2_low);

	return ret;
}

static int do_linear_map(void)
{
	int ret;

	mmu_flag_t flag;

	MEMORY_NOMAL(flag);

	addr_t vaddr = allign_up(g_boot_param.ebss, PAGE_SHIFT);
	addr_t paddr = vaddr - g_boot_param.loffset;

	ret = do_mmu_map(vaddr, paddr, LINEAR_MEM_SIZE, flag, alloc_pt2_low);

#if 1 //test if liner map success, if needed, can enable.
	*(uint32_t *)vaddr = 0;
	*(uint32_t *)(vaddr+SECTION_SIZE) = 0;
	*(uint32_t *)(vaddr+LINEAR_MEM_SIZE-4) = 0;
#endif
	
	return ret;
}

static int mmu_init(void)
{
	int ret;

	g_pt1_addr = g_boot_param.pt1_addr;
	g_v2p_off = g_boot_param.loffset;

	ret = do_io_map();
	if(ret < 0)
	{
		rdbg("do_io_map failed!\n");
		return ret;
	}
	rdbg("do_io_map success!\n");
	
	ret = do_linear_map();
	if(ret < 0)
	{
		rdbg("do_linear_map failed!\n");
		return ret;
	}
	rdbg("do_linear_map success!\n");
	
	return ret;
}

void data_abort_dump(uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
	printf("Data abort@PC:0x%08x!\n", regs[15]);

	printf("Core dump Registers: R0-R15, CPSR:\n");

	printf("0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", \
		regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);

	printf("0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", \
		regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);

	printf("0x%08x\n", regs[16]);

	printf("DFAR:0x%08x DFSR:0x%08x\n", dfar, dfsr);
}

void cpu_lock(void)
{
	spin_lock(&g_cpu_lock);
}

void cpu_unlock(void)
{
	spin_unlock(&g_cpu_lock);
}

void idle_entry(void)
{
	//cpu_lock();
	//idle_thread_create();
	//cpu_unlock();

	while(1)
	{
		//cpu_lock();
		/*if return 0, we unlock at the other point.*/
		//if(schedule() == -1)
		//{
			//cpu_unlock();
		//}
		schedule();
	}
}
 
void secondary_cpu(void)
{
	//cpu_lock();

	uint32_t cpuid = get_cpuid();

	rdbg("secondary_cpu:i am cpu#%d.\n", cpuid);

	idle_thread_create();
	//cpu_unlock();
	idle_entry();
}

#if 1 //thread test
void thread_test(void *arg)
{
	//cpu_lock();
	
	uint32_t *p;
	uint32_t cpuid = get_cpuid();
	
	rdbg("thread_test:i am cpu#%d.\n", cpuid);

	p = (uint32_t *)rmalloc(sizeof(uint32_t)*20);
	*p = 0;
	*(p+19) = 0;
	rdbg("[cpu#%d]rmalloc test ok!\n", cpuid);
	rfree(p);
	rdbg("[cpu#%d]rfree test ok!\n", cpuid);
	
	//cpu_unlock();
}
#endif
extern void start_secondary_cpu(uint32_t);

/*
* Main function.
*/
int rank_main(void)
{
	addr_t mm_start;
	size_t low_area_size;

	uart_init(UART0_BASE_PADDR, UART_CLK, 115200);
	
	print_init(UART0_BASE_PADDR);
	
	rdbg("start!!!\n");

	rdbg("mm init...\n");

	rdbg("boot params: ebss= 0x%x, pt1_addr = 0x%x, loffset = 0x%x\n", \
		g_boot_param.ebss, g_boot_param.pt1_addr, g_boot_param.loffset);
	
	if(mmu_init() == -1)
	{
		rdbg("mmu init failed!\n");
		return -1;
	}

	print_init(UART0_BASE_VADDR);
	
	rdbg("mmu init success!\n");

	mm_start = allign_up(g_boot_param.ebss, 4);
	low_area_size = (mm_start&((1<<PAGE_SHIFT)-1))+PAGE_SIZE;
	if(low_area_init(mm_start, low_area_size) == -1)
	{
		rdbg("low area init failed!\n");
		return -1;
	}
	rdbg("low area init success!\n");

	mm_start += low_area_size;
	mm_start = allign_up(mm_start, PAGE_SHIFT);
	rdbg("g_v2p_off= 0x%08x\n", g_v2p_off);
	if(zones_init(vir2phy(mm_start, g_v2p_off), LINEAR_MEM_SIZE) == -1)
	{
		rdbg("zones init failed!\n");
		return -1;
	}
	rdbg("zones init success!\n");

	if(rmalloc_slab_init() == -1)
	{
		rdbg("rmalloc slab init failed!\n");
		return -1;
	}
	rdbg("rmalloc slab init success!\n");

	if(stack_slab_init() == -1)
	{
		rdbg("stack slab init failed!\n");
		return -1;
	}
	rdbg("stack slab init success!\n");

#if 1//test rmalloc, if needed, can enable.
	uint32_t *p = (uint32_t *)rmalloc(sizeof(uint32_t)*20);
	*p = 0;
	*(p+19) = 0;
	rdbg("rmalloc test ok!\n");
	rfree(p);
	rdbg("rfree test ok!\n");
#endif
	idle_thread_create();

	start_secondary_cpu(1);
	start_secondary_cpu(2);
	start_secondary_cpu(3);

#if 1 //test thread
	int i;
	for(i = 0; i < CPU_NUM; i++)
	{
		thread_arg_t arg;
		arg.arg = NULL;
		arg.entry = thread_test;
		arg.prio = 0;
		//cpu_lock();
		thread_create(&arg);
		//cpu_unlock();
	}
#endif

	idle_entry();
	
	return 0;
}


