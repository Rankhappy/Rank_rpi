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

#define FRAME_SHIFT 12
#define FRAME_MASK (~((1<<FRAME_SHIFT)-1))
#define FRAME_SIZE (1<<FRAME_SHIFT)

extern size_t g_v2p_off;

static inline int size2order(size_t size)
{
	int order = 0;

	size >>= 4;
	
	if(size >= (1<<5))
	{
		return 5;
	}
	
	while(size > 1)
	{
		order++;
		size >>= 1;
	}
	
	return order;
}

static inline int order2size(int order)
{
	return (1<<(order+4));
}

#endif


