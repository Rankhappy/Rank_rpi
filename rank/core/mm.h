/*
*Name: mm.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef MM_H
#define MM_H
#include "type.h"
#include "mm_internal.h"

typedef struct
{
	uint32_t frames;
	uint32_t pfn;
}frame_t;

extern int low_area_init(addr_t, size_t);
extern void *low_malloc(size_t);
extern void low_free(void *);

extern int linear_zones_init(addr_t, size_t);
extern frame_t *alloc_linear_zone(uint32_t);
extern void free_linear_zone(frame_t *);

extern int rmalloc_slab_init(void);
extern void *rmalloc(size_t);
extern void rfree(void *);

#endif
