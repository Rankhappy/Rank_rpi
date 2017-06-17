/*
*Name: slab
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "list.h"
#include "type.h"
#include "mm.h"
#include "mm_internal.h"

#define RMALLOC_MAX_ORDER 5
#define RMALLOC_SIZE_SHIFT 4

typedef struct
{
	uint32_t obj_size;
	uint32_t objects;
	list_t partial_list;
	list_t full_list;
}slabs_t;

typedef struct
{
	slabs_t *slabs;
	void *base;
	uint32_t obj_head;
	uint32_t availables;
	frame_t *frame;
	list_node_t node;
}slab_t;

static slabs_t rmalloc_slabs[RMALLOC_MAX_ORDER];

static int slab_init(slabs_t *slabs, int obj_size)
{
	slabs->obj_size = obj_size;
	
	slabs->objects = (FRAME_SIZE-sizeof(slab_t))/obj_size;

	list_init(&slabs->partial_list);
	list_init(&slabs->full_list);

	return 0;
}

static void *slab_alloc(slabs_t *slabs)
{
	void *obj;
	slab_t *slab;
	
	if(!list_empty(&slabs->partial_list))
	{
		list_node_t *slab_node = list_head(&slabs->partial_list);
		slab = list_data(slab_node, slab_t, node);
	}
	else
	{
		int i;
		frame_t *frame;
		frame = alloc_linear_zone(1);
		if(frame == NULL)
		{
			return NULL;
		}
		slab = (slab_t *)(phy2vir((frame->pfn+1)<<FRAME_SHIFT, g_v2p_off)-sizeof(slab_t));
		slab->frame = frame;
		slab->slabs = slabs;
		slab->base = (void *)((addr_t)slab & FRAME_MASK);
		slab->availables = slabs->objects;
		slab->obj_head = 0;
		for (i = 0; i < slabs->objects; i++)
		{
			*((uint32_t *)(slab->base+i*slabs->obj_size)) = i+1;
		}
	}
	
	obj = (void *)((addr_t)(slab->base) + slab->obj_head*slabs->obj_size);
	slab->obj_head = *(uint32_t *)obj;
	slab->availables--;
	if(slab->availables == 0)
	{
		list_delete(&slab->node);
		list_add_head(&slabs->full_list, &slab->node);
	}

	return obj;
}

static void slab_free(void *obj)
{
	slabs_t *slabs;;
	slab_t *slab;
	uint32_t idx;
	addr_t base;

	base = (addr_t)obj & FRAME_MASK;
	slab = (slab_t *)(base + FRAME_SIZE - sizeof(slab_t));
	slabs = slab->slabs;
	idx = ((addr_t)obj-base)/slabs->obj_size;

	*(uint32_t *)obj = slab->obj_head;
	slab->obj_head = idx;
	slab->availables++;

	if(slab->availables == 1)
	{
		list_delete(&slab->node);
		list_add_head(&slabs->partial_list, &slab->node);
	}
	else if(slab->availables == slabs->objects)
	{
		list_delete(&slab->node);
		free_linear_zone(slab->frame);
	}
}

int rmalloc_slab_init(void)
{
	int rc;
	int order;

	for(order = 0; order <= RMALLOC_MAX_ORDER; order++)
	{
		rc = slab_init(&rmalloc_slabs[order], 1<<order);
		if(rc)
		{
			return rc;
		}
	}

	return rc;
}

void *rmalloc(size_t size)
{
	int order;

	size = allign_up(size, RMALLOC_SIZE_SHIFT);

	if(size > 1<<(RMALLOC_MAX_ORDER+RMALLOC_SIZE_SHIFT))
	{
		return NULL;
	}

	order = size2order(size);

	return slab_alloc(&rmalloc_slabs[order]);
}

void rfree(void *addr)
{
	slab_free(addr);
}

