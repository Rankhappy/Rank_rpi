/*
*Name: mm_internal.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef MM_INTERNAL_H
#define MM_INTERNAL_H

#include "type.h"
#include "mmu.h"

#define allign_mask(a) ((1<<(a))-1)
#define allign_up(x, a) (((x)+((1<<(a))-1))&(~((1<<(a))-1)))
#define allign_down(x, a) ((x)&(~((1<<(a))-1)))

#define LINEAR_MEM_SIZE  0x2000000
#define PHY_MEM_SIZE 	0x3e000000

#define FRAME_SHIFT 12
#define FRAME_MASK (~((1<<FRAME_SHIFT)-1))
#define FRAME_SIZE (1<<FRAME_SHIFT)

#define mmdbg(fmt, args...)  //printf("[RANK][MM][DEBUG]"fmt, ##args)
#define mmerr(fmt, args...) printf("[RANK][MM][ERROR]"fmt, ##args)


extern size_t g_v2p_off;
extern void printf(const char *fmt, ...);

static inline int size2order(size_t size, int max_order, int shift)
{
	int order = 0;

	size >>= shift;
	
	if(size >= (1<<max_order))
	{
		return max_order;
	}
	
	while(size > 0)
	{
		order++;
		size >>= 1;
	}
	
	return order;
}

static inline int order2size(int order, int shift)
{
	return (1<<(order+shift));
}

#endif


