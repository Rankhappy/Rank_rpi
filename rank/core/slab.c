/*
*Name: slab
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "list.h"
#include "type.h"
#include "mm.h"
#include "mm_internal.h"
#include "thread.h"

#define RMALLOC_MAX_ORDER 5
#define RMALLOC_SIZE_SHIFT 4
#define STACK_SIZE 1024

typedef struct
{
	uint32_t frames;
	uint32_t obj_size;
	uint32_t objects;
	list_t partial_list;
	list_t full_list;
	mutex_t lock;
	//spin_lock_t lock;
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

static slabs_t rmalloc_slabs[RMALLOC_MAX_ORDER+1];
static slabs_t stack_slab;

static int slab_init(slabs_t *slabs, int obj_size, uint32_t frames)
{
	slabs->frames = frames;
	slabs->obj_size = obj_size;
	slabs->objects = (frames<<FRAME_SHIFT)/obj_size;

	list_init(&slabs->partial_list);
	list_init(&slabs->full_list);

	mutex_init(&slabs->lock);
	//slabs->lock = 0;

	return 0;
}

static void *slab_alloc(slabs_t *slabs)
{
	void *obj;
	slab_t *slab;
	int locked;

	mmdbg("slab_alloc:obj_size = 0x%x, objects = %d.\n", slabs->obj_size, slabs->objects);

	locked = mutex_lock(&slabs->lock);
	//spin_lock(&slabs->lock);
	
	if(!list_empty(&slabs->partial_list))
	{
		list_node_t *slab_node = list_head(&slabs->partial_list);
		slab = list_data(slab_node, slab_t, node);
	}
	else
	{
		/*Allocte new slab*/
		int i;
		frame_t *frame;
		/*Should add type as the function's input parameter rather than fixed here?*/
		frame = alloc_frames(TYPE_LINEAR_ZONE, slabs->frames);
		if(frame == NULL)
		{
			if(locked == 0)	mutex_unlock(&slabs->lock);
			//spin_unlock(&slabs->lock);
			return NULL;
		}
		slab = low_malloc(sizeof(slab_t));
		if(slab == NULL)
		{
			if(locked == 0)	mutex_unlock(&slabs->lock);
			//spin_unlock(&slabs->lock);
			return NULL;
		}
		slab->frame = frame;
		slab->slabs = slabs;
		/*Get the virtual address of the frame*/
		mmdbg("slab_alloc:pfn = %d, off = 0x%08x\n", frame->pfn, g_v2p_off);
		slab->base = (void *)phy2vir(frame->pfn << FRAME_SHIFT, g_v2p_off);
		slab->availables = slabs->objects;
		slab->obj_head = 0;
		/*Link all of the objs*/
		mmdbg("slab_alloc:base = 0x%08x.\n", (uint32_t)slab->base);
		for(i = 0; i < slabs->objects; i++)
		{
			*((uint32_t *)(slab->base+i*slabs->obj_size)) = i+1;
		}
		/*Set slab as frame's private data.*/
		for(i = 0; i < slabs->frames; i++)
		{
			set_frame_priv(TYPE_LINEAR_ZONE, frame->pfn+i, (void *)slab);
			mmdbg("slab_alloc:slab = 0x%08x.\n", (uint32_t)slab);
		}
		list_init(&slab->node);
		list_add_head(&slabs->partial_list, &slab->node);
	}

	mmdbg("slab_alloc:obj_head@1 = 0x%08x.\n", slab->obj_head);
	obj = (void *)((addr_t)(slab->base) + slab->obj_head*slabs->obj_size);
	slab->obj_head = *(uint32_t *)obj;
	mmdbg("slab_alloc:obj_head@2 = 0x%08x.\n", slab->obj_head);
	slab->availables--;
	if(slab->availables == 0)
	{	
		/*Move slab from partial list to full list.*/
		list_delete(&slab->node);
		list_add_head(&slabs->full_list, &slab->node);
	}

	if(locked == 0)	mutex_unlock(&slabs->lock);
	//spin_unlock(&slabs->lock);
	
	return obj;
}

static slab_t *get_slab_byobj(type_zone_t type, void *obj)
{
	uint32_t pfn;

	pfn = vir2phy((addr_t)obj, g_v2p_off)>>FRAME_SHIFT;

	return (slab_t *)get_frame_priv(type, pfn);
}

static void slab_free(void *obj)
{
	slabs_t *slabs;;
	slab_t *slab;
	uint32_t idx;
	int locked;

	/*Should add type as the function's input parameter rather than fixed here?*/
	slab = get_slab_byobj(TYPE_LINEAR_ZONE, obj);
	mmdbg("slab_free:slab = 0x%08x.\n", (uint32_t)slab);
	slabs = slab->slabs;

	locked = mutex_lock(&slabs->lock);
	//spin_lock(&slabs->lock);
	
	idx = ((addr_t)obj-(addr_t)(slab->base))/slabs->obj_size;

	mmdbg("slab_free:obj_head@1 = 0x%08x.\n", slab->obj_head);
	*(uint32_t *)obj = slab->obj_head;
	slab->obj_head = idx;
	mmdbg("slab_free:obj_head@2 = 0x%08x.\n", slab->obj_head);
	slab->availables++;

	/*Move slab from full list to partial list.*/
	if(slab->availables == 1)
	{
		list_delete(&slab->node);
		list_add_head(&slabs->partial_list, &slab->node);
	}

	/*Move slab from partial list to frame pool.*/
	else if(slab->availables == slabs->objects)
	{
		list_delete(&slab->node);
		free_frames(slab->frame);
		low_free(slab);
	}

	if(locked == 0)	mutex_unlock(&slabs->lock);
	//spin_unlock(&slabs->lock);
}

int rmalloc_slab_init(void)
{
	int rc;
	int order;

	for(order = 0; order <= RMALLOC_MAX_ORDER; order++)
	{
		rc = slab_init(&rmalloc_slabs[order], 1<<(order+RMALLOC_SIZE_SHIFT), 1);
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

	/*Even size is zero, we still allocate space for the caller.*/
	size = allign_up(size, RMALLOC_SIZE_SHIFT);

	if(size > 1<<(RMALLOC_MAX_ORDER+RMALLOC_SIZE_SHIFT))
	{
		return NULL;
	}

	order = size2order(size, RMALLOC_MAX_ORDER, RMALLOC_SIZE_SHIFT);

	mmdbg("rmalloc:size = 0x%x, order = %d.\n", size, order);

	return slab_alloc(&rmalloc_slabs[order]);
}

void rfree(void *addr)
{
	slab_free(addr);
}

int stack_slab_init(void)
{
	int rc;

	rc = slab_init(&stack_slab, STACK_SIZE, 2);

	return rc;
}

void *stack_alloc(void)
{
	return slab_alloc(&stack_slab);
}

void stack_free(void *addr)
{
	slab_free(addr);
}


