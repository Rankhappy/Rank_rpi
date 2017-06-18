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

#define IO_BASE_PADDR    0x3f200000
#define IO_BASE_VADDR    0xff200000

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

boot_param_t g_boot_param;

uint8_t g_pt2_map[2];
uint32_t sp_idle[1024];

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

addr_t alloc_pt2_low(int blocks)
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

int do_io_map(void)
{
	int ret;

	mmu_flag_t flag;

	MEMORY_DEVICE(flag);

	addr_t vaddr = IO_BASE_VADDR;
	addr_t paddr = IO_BASE_PADDR;

	ret = do_mmu_map(vaddr, paddr, SECTION_SIZE, flag, alloc_pt2_low);

	return ret;
}

int do_linear_map(void)
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

int mm_init(void)
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
	
	if(mm_init() == -1)
	{
		rdbg("mm init failed!\n");
		return -1;
	}

	print_init(UART0_BASE_VADDR);
	
	rdbg("mm init success!\n");

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
	if(zones_init(mm_start, LINEAR_MEM_SIZE) == -1)
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

#if 1//test rmalloc, if needed, can enable.
	uint32_t *p = (uint32_t *)rmalloc(sizeof(uint32_t)*20);
	*p = 0;
	*(p+19) = 0;
#endif
	
	return 0;
}


