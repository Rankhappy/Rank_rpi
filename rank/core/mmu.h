#ifndef MMU_H
#define MMU_H

typedef struct
{
	uint32_t cacheble:1;
	uint32_t shareble:1;
	uint32_t ap:2;
	uint32_t ns:1;
	uint32_t ng:1;
}mmu_flag_t;

typedef addr_t (*alloc_pt2_t)(int);

extern int do_mmu_map(addr_t, addr_t, size_t, mmu_flag_t, alloc_pt2_t);

#endif
