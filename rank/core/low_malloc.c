/*
*Name: low_memory.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "list.h"
#include "type.h"
#include "core.h"
#include "mm.h"
#include "mm_internal.h"
#include "thread.h"

#define LOW_MAX_ORDER 5
#define LOW_SIZE_SHIFT 4
#define FLAG_ALLOC_BIT (1<<31)
#define CHUNK_RESERVE_SIZE (8)

typedef struct
{
	size_t size;
	uint32_t flag;
	list_node_t node;
}chunk_t;

static list_t g_free_list[LOW_MAX_ORDER+1];
static mutex_t g_low_lock;

int low_area_init(addr_t start, size_t size)
{
	int order;
	chunk_t *chunk;
	
	start = allign_up(start, LOW_SIZE_SHIFT);
	size = allign_down(size, LOW_SIZE_SHIFT);

	for(order = 0; order <= LOW_MAX_ORDER; order++)
	{	
		list_init(&g_free_list[order]);
	}

	/*Upper boundary.*/
	chunk = (chunk_t *)start;
	chunk->size = 1<<LOW_SIZE_SHIFT;
	chunk->flag = FLAG_ALLOC_BIT;

	/*Main area.*/
	chunk = (chunk_t *)((addr_t)chunk + chunk->size);
	chunk->size = size - (2<<LOW_SIZE_SHIFT);
	chunk->flag = 1<<LOW_SIZE_SHIFT;
	order = size2order(chunk->size, LOW_MAX_ORDER, LOW_SIZE_SHIFT);
	list_init(&chunk->node);
	list_add_head(&g_free_list[order], &chunk->node);

	/*Down boundary.*/
	chunk = (chunk_t *)((addr_t)chunk + chunk->size);
	chunk->size = 1<<LOW_SIZE_SHIFT;
	chunk->flag = FLAG_ALLOC_BIT | (size - (2<<LOW_SIZE_SHIFT));

	mutex_init(&g_low_lock);

	return 0;
}

void *low_malloc(size_t size)
{
	int order, i;
	chunk_t *chunk;
	int chunk_ok = 0;
	int locked;

	size += CHUNK_RESERVE_SIZE;
	size = allign_up(size, LOW_SIZE_SHIFT);

	if(size > 1<<(LOW_MAX_ORDER+LOW_SIZE_SHIFT))
	{
		return NULL;
	}

	order = size2order(size, LOW_MAX_ORDER, LOW_SIZE_SHIFT);

	locked = mutex_lock(&g_low_lock);

	for(i = order; i <= LOW_MAX_ORDER; i++)
	{
		if(list_empty(&g_free_list[i]))
		{
			continue;
		}
		
		list_node_t *chunk_node;
		list_foreach(&g_free_list[i], chunk_node)
		{
			chunk = list_data(chunk_node, chunk_t, node);
			mmdbg("low_malloc:chunk = 0x%08x.\n", (uint32_t)chunk);
			if(chunk->size >= size)
			{
				chunk_ok = 1;
				chunk->flag |= FLAG_ALLOC_BIT;
				list_delete(&chunk->node);
				break;
			}
		}
		if(chunk_ok)
		{
			break;
		}
	}

	if(i > LOW_MAX_ORDER)
	{
		if(locked == 0)	mutex_unlock(&g_low_lock);
		return NULL;
	}

	/*Add the remain area to the free list.*/
	if(chunk->size > size)
	{
		size_t remain = chunk->size - size;
		if(remain >= (1<<LOW_SIZE_SHIFT))
		{
			chunk->size -= remain;
			chunk_t *temp = (chunk_t *)((addr_t)chunk+chunk->size);
			mmdbg("low_malloc:remain chunk = 0x%08x.\n", (uint32_t)temp);
			temp->size = remain;
			temp->flag = chunk->size;
			list_init(&temp->node);
			order = size2order(remain, LOW_MAX_ORDER, LOW_SIZE_SHIFT);
			list_add_head(&g_free_list[order], &temp->node);
		}
	}

	if(locked == 0)	mutex_unlock(&g_low_lock);
	
	return (void *)((addr_t)chunk+CHUNK_RESERVE_SIZE);
}

void low_free(void *addr)
{
	int order;
	chunk_t *chunk, *next_chunk, *pre_chunk;
	int locked;

	locked = mutex_lock(&g_low_lock);

	chunk = (chunk_t *)((addr_t)addr-CHUNK_RESERVE_SIZE);
	next_chunk = (chunk_t *)((addr_t)chunk+chunk->size);
	pre_chunk = (chunk_t *)((addr_t)chunk-(chunk->flag&(~FLAG_ALLOC_BIT)));

	mmdbg("low_free:chunk = 0x%08x ,next_chunk = 0x%08x, pre_chunk = 0x%08x\n", \
		(uint32_t)chunk, (uint32_t)next_chunk, (uint32_t)pre_chunk);

	chunk->flag &= ~FLAG_ALLOC_BIT;

	/*Combine with the next chunck.*/
	if((next_chunk->flag&FLAG_ALLOC_BIT) == 0)
	{
		chunk->size += next_chunk->size;
		list_delete(&next_chunk->node);
		next_chunk = (chunk_t *)((addr_t)chunk+chunk->size);
		next_chunk->flag &= FLAG_ALLOC_BIT;
		next_chunk->flag |= chunk->size;
	}

	/*Combine with the previous chunck.*/
	if((pre_chunk->flag&FLAG_ALLOC_BIT) == 0)
	{
		pre_chunk->size += chunk->size;
		list_delete(&pre_chunk->node);
		chunk = pre_chunk;
		next_chunk->flag &= FLAG_ALLOC_BIT;
		next_chunk->flag |= chunk->size;
	}

	mmdbg("low_free:chunk = 0x%08x.\n", (uint32_t)chunk);
	list_init(&chunk->node);
	order = size2order(chunk->size, LOW_MAX_ORDER, LOW_SIZE_SHIFT);
	list_add_head(&g_free_list[order], &chunk->node);

	if(locked == 0)	mutex_unlock(&g_low_lock);
}


