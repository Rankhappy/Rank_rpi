/*
*Name: mm.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef MM_H
#define MM_H
#include "type.h"
#include "mm_internal.h"

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

typedef enum
{
	TYPE_LINEAR_ZONE = 0,
	TYPE_NORMAL_ZONE,
	TYPE_ZONE_SIZE
}type_zone_t;

typedef struct
{
	uint32_t frames;
	uint32_t pfn;
	type_zone_t type;
}frame_t;

//low
extern int low_area_init(addr_t, size_t);
extern void *low_malloc(size_t);
extern void low_free(void *);

//frame
extern int zones_init(addr_t);
extern frame_t *alloc_frames(type_zone_t, uint32_t);
extern void free_frames(frame_t *);
extern void set_frame_priv(type_zone_t, uint32_t, void *);
extern void *get_frame_priv(type_zone_t, uint32_t);

//slab
extern int rmalloc_slab_init(void);
extern void *rmalloc(size_t);
extern void rfree(void *);
extern int stack_slab_init(void);
extern void *stack_alloc(void);
extern void stack_free(void *);

//vmalloc
extern int vmalloc_init(addr_t, size_t);
extern void *vmalloc(size_t);
extern void vfree(void *);

#endif
