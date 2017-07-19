/*
*Name: vmalloc.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "list.h"
#include "type.h"
#include "core.h"
#include "mmu.h"
#include "mm.h"
#include "mm_internal.h"
#include "thread.h"
#include "assert.h"

#define V_MAX_ORDER 10
#define V_SIZE_SHIFT 4
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define FLAG_ALLOC_BIT (1<<31)
#define CHUNK_RESERVE_SIZE (8)

typedef struct
{
	uint32_t pindex;
	list_node_t node;
}area_entity_t;

typedef struct
{
	addr_t start;
	size_t size;
	list_t list;
	spin_lock_t lock;
}v_area_t;

typedef struct
{
	size_t size;
	uint32_t flag;
	list_node_t node;
}heap_chunk_t;

typedef struct
{
	addr_t start;
	size_t size;
	list_t free_list[V_MAX_ORDER+1];
	mutex_t lock;
}v_heap_t;

static v_heap_t g_v_heap;
static v_area_t g_v_area;
static heap_chunk_t *g_head_chunk;
static heap_chunk_t *g_tail_chunk;

static addr_t v_alloc_pt2(int blocks)
{
	addr_t rc = (addr_t)rmalloc(blocks * 1024);

	return rc;
}

static int heap_area_grow(size_t size)
{
	list_node_t *ae_node;
	area_entity_t *ae;
	heap_chunk_t *chunk;
	addr_t vaddr, paddr;
	frame_t *frame;
	mmu_flag_t flag;
	int sindex;
	int index = size >> PAGE_SHIFT;
	int i;
	int locked;

	spin_lock(&g_v_area.lock);
	for(i = 0; i < index; i++)
	{
		if(list_empty(&g_v_area.list))
		{
			return -1;
		}
		ae_node = list_head(&g_v_area.list);
		list_delete(ae_node);
		ae = list_data(ae_node, area_entity_t, node);
		vaddr = g_v_area.start + ((ae->pindex)<<PAGE_SHIFT);
		frame = alloc_frames(TYPE_NORMAL_ZONE, 1);
		if(frame == NULL)
		{
			spin_unlock(&g_v_area.lock);
			return -1;
		}
		paddr = frame->pfn << FRAME_SHIFT;
		MEMORY_NOMAL(flag);
		if(do_mmu_map(vaddr, paddr, PAGE_SIZE, flag, v_alloc_pt2))
		{
			spin_unlock(&g_v_area.lock);
			return -1;
		}
	}
	spin_unlock(&g_v_area.lock);
	
	sindex = ae->pindex - index + 1;
	chunk = (heap_chunk_t *)(g_v_area.start + (sindex<<PAGE_SHIFT));
	assert((addr_t)chunk == (addr_t)g_tail_chunk + g_tail_chunk->size);

	locked = mutex_lock(&g_v_heap.lock);
	if(g_tail_chunk->flag&FLAG_ALLOC_BIT)
	{
		chunk->size = size;
		int order = size2order(chunk->size, V_MAX_ORDER, V_SIZE_SHIFT);
		list_init(&chunk->node);
		list_add_head(&g_v_heap.free_list[order], &chunk->node);
		g_tail_chunk = chunk;
	}
	else
	{
		chunk = g_tail_chunk;
		chunk->size += size;
		list_delete(&chunk->node);
		int order = size2order(chunk->size, V_MAX_ORDER, V_SIZE_SHIFT);
		list_init(&chunk->node);
		list_add_head(&g_v_heap.free_list[order], &chunk->node);
	}
	if(locked == 0)	mutex_unlock(&g_v_heap.lock);

	rfree(ae);
	
	return 0;
}

int vmalloc_init(addr_t start, size_t size)
{
	int i;

	start = allign_up(start, PAGE_SHIFT);
	size = allign_down(size, PAGE_SHIFT);

	g_v_area.start = start;
	g_v_area.size = size;
	list_init(&g_v_area.list);
	for(i = 0; i < (size>>PAGE_SHIFT); i++)
	{
		area_entity_t *ae = rmalloc(sizeof(area_entity_t));
		if(ae == NULL)
		{
			return -1;
		}
		ae->pindex = i;
		list_init(&ae->node);
		list_add_tail(&g_v_area.list, &ae->node);
	}

	g_v_heap.start = start;
	g_v_heap.size = 0;
	for(i = 0; i <= V_MAX_ORDER; i++)
	{
		list_init(&g_v_heap.free_list[i]);
	}

	g_head_chunk = (heap_chunk_t *)start;
	
	g_v_area.lock = 0;
	mutex_init(&g_v_heap.lock);

	return 0;
}
void *vmalloc(size_t size)
{
	int order;
	int i;
	heap_chunk_t *chunk;
	int chunk_ok = 0;
	int locked;

	size += CHUNK_RESERVE_SIZE;
	size = allign_up(size, V_SIZE_SHIFT);

	if(size > 1<<(V_MAX_ORDER+V_SIZE_SHIFT))
	{
		return NULL;
	}

	order = size2order(size, V_MAX_ORDER, V_SIZE_SHIFT);
	
	locked = mutex_lock(&g_v_heap.lock);
retry:
	for(i = order; i <= V_MAX_ORDER; i++)
	{
		if(list_empty(&g_v_heap.free_list[i]))
		{
			continue;
		}
		
		list_node_t *chunk_node;
		list_foreach(&g_v_heap.free_list[i], chunk_node)
		{
			chunk = list_data(chunk_node, heap_chunk_t, node);
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

	/*Grow size of the heap and retry.*/
	if(i > V_MAX_ORDER)
	{
		if(heap_area_grow(allign_up(size, PAGE_SHIFT)))
		{
			if(locked == 0)	mutex_unlock(&g_v_heap.lock);
			return NULL;
		}	
		goto retry;
	}

	/*Add the remain area to the free list.*/
	if(chunk->size > size)
	{
		size_t remain = chunk->size - size;
		if(remain >= (1<<V_SIZE_SHIFT))
		{
			chunk->size -= remain;
			heap_chunk_t *temp = (heap_chunk_t *)((addr_t)chunk+chunk->size);
			temp->size = remain;
			temp->flag = chunk->size;
			list_init(&temp->node);
			order = size2order(remain, V_MAX_ORDER, V_SIZE_SHIFT);
			list_add_head(&g_v_heap.free_list[order], &temp->node);
			if(chunk == g_tail_chunk)
			{
				g_tail_chunk = temp;
			}
		}
	}

	if(locked == 0)	mutex_unlock(&g_v_heap.lock);

	return (void *)((addr_t)chunk+CHUNK_RESERVE_SIZE);
}

void vfree(void *addr)
{
	int order;
	heap_chunk_t *chunk, *next_chunk, *pre_chunk;
	int locked;

	locked = mutex_lock(&g_v_heap.lock);

	chunk = (heap_chunk_t *)((addr_t)addr-CHUNK_RESERVE_SIZE);
	next_chunk = (heap_chunk_t *)((addr_t)chunk+chunk->size);
	pre_chunk = (heap_chunk_t *)((addr_t)chunk-(chunk->flag&(~FLAG_ALLOC_BIT)));

	chunk->flag &= ~FLAG_ALLOC_BIT;

	/*Combine with the next chunck.*/
	if((chunk!=g_tail_chunk) && (next_chunk->flag&FLAG_ALLOC_BIT) == 0)
	{
		chunk->size += next_chunk->size;
		list_delete(&next_chunk->node);
		next_chunk = (heap_chunk_t *)((addr_t)chunk+chunk->size);
		next_chunk->flag &= FLAG_ALLOC_BIT;
		next_chunk->flag |= chunk->size;
	}

	/*Combine with the previous chunck.*/
	if((chunk!=g_head_chunk) && (pre_chunk->flag&FLAG_ALLOC_BIT) == 0)
	{
		pre_chunk->size += chunk->size;
		list_delete(&pre_chunk->node);
		chunk = pre_chunk;
		next_chunk->flag &= FLAG_ALLOC_BIT;
		next_chunk->flag |= chunk->size;
	}

	/*Return to the area list.*/
	if(chunk->size > PAGE_SIZE)
	{
		int i;
		int sindex = (allign_up((addr_t)chunk, PAGE_SHIFT) - g_v_area.start)>>PAGE_SHIFT;

		spin_lock(&g_v_area.lock);
		for(i = 0; i < ((chunk->size)>>PAGE_SHIFT); i++)
		{
			area_entity_t *ae = rmalloc(sizeof(area_entity_t));
			if(ae == NULL)
			{
				goto out;
			}
			ae->pindex = sindex+i;
			list_init(&ae->node);
			list_add_tail(&g_v_area.list, &ae->node);
			chunk->size -= PAGE_SIZE;
			do_mmu_unmap(g_v_area.start + (ae->pindex<<PAGE_SHIFT));
		}
		spin_unlock(&g_v_area.lock);
	}

out:
	if(chunk->size)
	{
		list_init(&chunk->node);
		order = size2order(chunk->size, V_MAX_ORDER, V_SIZE_SHIFT);
		list_add_head(&g_v_heap.free_list[order], &chunk->node);
	}

	if(locked == 0)	mutex_unlock(&g_v_heap.lock);
}

