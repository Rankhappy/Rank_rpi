/*
*Name: frame.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "list.h"
#include "type.h"
#include "mm.h"
#include "mm_internal.h"

#define FRAME_MAX_ORDER 11

typedef struct
{
	uint32_t idx;
	uint32_t frames;
	list_node_t node;
}zone_t;

typedef struct
{
	uint32_t start_pfn;
	uint32_t frames;
	list_t free_list[FRAME_MAX_ORDER+1];
}zones_t;

typedef struct
{
	zone_t zone;
	void *priv;
}zone_map_t;

static zones_t g_linear_zones;
static zone_map_t g_linear_map[LINEAR_MEM_SIZE>>FRAME_SHIFT];

static zones_t *g_zones[TYPE_ZONE_SIZE];
static zone_map_t *g_map[TYPE_ZONE_SIZE];

static inline int frames2order(uint32_t frames)
{
	/*Return zero when frames is zero*/
	int order = 0;
	
	if(frames >= (1<<FRAME_MAX_ORDER))
	{
		return FRAME_MAX_ORDER;
	}
	
	while(frames > 1)
	{
		order++;
		frames >>= 1;
	}
	
	return order;
}

static int _zones_init(zones_t *zones, zone_map_t *zone_map)
{
	int order;
	uint32_t frames;
	uint32_t map_idx;

	for(order = 0; order <= FRAME_MAX_ORDER; order++)
	{	
		list_init(&zones->free_list[order]);
	}

	frames = zones->frames;
	map_idx = 0;

	/*Add the whole zone to the free list.*/
	while(frames)
	{
		order = frames2order(frames);
		zone_t *zone = &zone_map[map_idx].zone;
		zone->frames = (1 << order);
		zone->idx = map_idx;
		list_init(&zone->node);
		list_add_tail(&zones->free_list[order], &zone->node);
		map_idx += zone->frames;
		frames -= zone->frames;
	}

	return 0;
}

static frame_t *_alloc_zone(zones_t *zones, zone_map_t *zone_map, uint32_t frames)
{
	int order, order_i;
	zone_t *zone = NULL;
	frame_t *frame;

	/*Even frames is zero, we also allocate one frames.*/
	order = frames2order(frames);
	
	for(order_i = order; order_i <= FRAME_MAX_ORDER; order_i++)
	{
		if(list_empty(&zones->free_list[order_i]))
		{
			continue;
		}
	
		list_node_t *zone_node;
		zone_node = list_head(&zones->free_list[order_i]);
		zone = list_data(zone_node, zone_t, node);
		list_delete(&zone->node);
		break;
	}

	/*If there is no free space, then return.*/
	if(zone == NULL)
	{
		return NULL;
	}
	
	frame = (frame_t *)low_malloc(sizeof(frame_t));
	if(frame == NULL)
	{
		return NULL;
	}
	frame->frames = (1<<order);
	frame->pfn = zones->start_pfn + zone->idx;
	frame->type = TYPE_LINEAR_ZONE;

	/*Add the remain zones to the free list.*/
	while(order_i > order)
	{
		uint32_t next_idx = zone->idx + (1<<order);
		zone_t *next_zone = &zone_map[next_idx].zone;
		next_zone->frames = (1<<order);
		list_init(&next_zone->node);
		list_add_head(&zones->free_list[order], &next_zone->node);
		order++;
	}

	return frame;	
}

static zone_t *find_buddy_zone(zone_map_t *zone_map, uint32_t idx, int order)
{
	zone_t *zone = NULL;

	/*idx must be order alligned.*/
	if(idx&((1<<order)-1))
	{	
		return NULL;
	}

	idx ^= (1<<order);
	zone = &zone_map[idx].zone;;

	return zone;
	
}
void _free_zone(zones_t *zones, zone_map_t *zone_map, frame_t *frame)
{
	int order;
	uint32_t map_idx;
	zone_t *zone, *zone_buddy;

	order = frames2order(frame->frames);
	map_idx = frame->pfn - zones->start_pfn;

	/*Recursive to find the buddy idx.*/
	while(order < FRAME_MAX_ORDER)
	{
		zone_buddy = find_buddy_zone(zone_map, map_idx, order);
		if(zone_buddy == NULL)
		{
			break;
		}
		map_idx &= (0<<order);
		order++;
		list_delete(&zone_buddy->node);
	}

	zone = &zone_map[map_idx].zone;
	list_init(&zone->node);
	zone->frames = (1<<order);
	zone->idx = map_idx;
	list_init(&zone->node);
	list_add_head(&zones->free_list[order], &zone->node);

	low_free(frame);
}

/*TODO: add other types of physical memory.*/
int zones_init(addr_t start, size_t size)
{
	int rc;
	zones_t *zones;
	zone_map_t *zone_map;

	start = allign_up(start, FRAME_SHIFT);
	size = allign_down(size, FRAME_SHIFT);

	if(size == 0)
	{
		return -1;
	}

	g_zones[TYPE_LINEAR_ZONE] = &g_linear_zones;
	g_map[TYPE_LINEAR_ZONE] = g_linear_map;

	zones = g_zones[TYPE_LINEAR_ZONE];
	zone_map = g_map[TYPE_LINEAR_ZONE];
	zones->start_pfn = start>>FRAME_SHIFT;
	zones->frames = size>>FRAME_SHIFT;
	rc = _zones_init(zones, zone_map);
	if(rc < 0)
	{
		return rc;
	}

	return 0;
}

frame_t *alloc_frames(type_zone_t type, uint32_t frames)
{
	frame_t *frame = NULL;
	zones_t *zones = g_zones[type];
	zone_map_t *zone_map = g_map[type];
	
	frame = _alloc_zone(zones, zone_map, frames);

	return frame;
}

void free_frames(frame_t *frame)
{
	zones_t *zones = g_zones[frame->type];
	zone_map_t *zone_map = g_map[frame->type];
	
	_free_zone(zones, zone_map, frame);

	low_free(frame);
}

void set_frame_priv(type_zone_t type, uint32_t pfn, void *data)
{
	uint32_t map_idx;
	zones_t *zones;
	zone_map_t *zone_map;

	if(type >= TYPE_ZONE_SIZE)
	{
		return;
	}

	zones = g_zones[type];
	zone_map = g_map[type];
	map_idx = pfn - zones->start_pfn;

	zone_map[map_idx].priv = data;
}

void *get_frame_priv(type_zone_t type, uint32_t pfn)
{
	uint32_t map_idx;
	zones_t *zones;
	zone_map_t *zone_map;
	
	if(type >= TYPE_ZONE_SIZE)
	{
		return NULL;
	}

	zones = g_zones[type];
	zone_map = g_map[type];
	map_idx = pfn - zones->start_pfn;

	return zone_map[map_idx].priv;
}

