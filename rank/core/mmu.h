/*
*Name: mmu.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef MMU_H
#define MMU_H

#define phy2vir(phy, v2poff) ((phy) + (v2poff))
#define vir2phy(vir, v2poff) ((vir) - (v2poff))

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
extern void do_mmu_unmap(addr_t);

#endif
