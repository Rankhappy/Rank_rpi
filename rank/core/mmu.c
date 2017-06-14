/*
*Name: mmu.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#include "type.h"
#include "arm.h"
#include "uart.h"
#include "mmu.h"

#define PT1_FIXED_FLAG_S ((1 << 1)|(1 << 10))
#define PT1_FIXED_FLAG_T (1 << 0)
#define PT2_FIXED_FLAG ((1 << 1)|(1 << 4))

#define mdbg(fmt, args...) printf("[RANK][MMU][DEBUG]"fmt, ##args)
#define merr(fmt, args...) printf("[RANK][MMU][ERROR]"fmt, ##args)

addr_t g_pt1_addr;
size_t g_v2p_off;
extern void printf(const char *fmt, ...);

static int do_section_map(addr_t vaddr, addr_t paddr, size_t size, mmu_flag_t mflag)
{
	uint32_t l1_offset = vaddr >> SECTION_SHIFT;
	uint32_t *pt1_entry = (uint32_t *)g_pt1_addr;
	uint32_t flag = PT1_FIXED_FLAG_S;
	
	if(mflag.cacheble == 1)
	{
		flag |= (3 << 2);
	}
	flag |= mflag.shareble << 16;
	flag |= mflag.ng << 17;
	flag |= mflag.ns << 19;
	flag |= (mflag.ap&0x01) << 11;
	flag |= (mflag.ap&0x02) << 14;
	
	pt1_entry += l1_offset;

	while(size)
	{
		if((*pt1_entry & 0x03) != 0)
		{
			if(*pt1_entry == (paddr|flag))
			{
				return 0;
			}
			merr("do_section_map:0x%x != 0x%x\n", *pt1_entry, paddr|flag);
			return -1;
		}
		*pt1_entry = paddr | flag;
		mdbg("do_section_map:pt1_entry = 0x%x, *pt1_entry = 0x%x\n", pt1_entry, *pt1_entry);
		pt1_entry++;
		vaddr += SECTION_SIZE;
		paddr += SECTION_SIZE;
		size  -= SECTION_SIZE;
	}

	return 0;
}

static int do_coarse_map(addr_t vaddr, addr_t paddr, size_t size, mmu_flag_t mflag, alloc_pt2_t alloc_pt2)
{
	int i;
	unsigned char ov;

	addr_t pt2_addr;
	int nsections = (allign_up(vaddr+size, SECTION_SHIFT) - \
						allign_down(vaddr, SECTION_SHIFT)) >> SECTION_SHIFT;
	uint32_t l1_offset = vaddr >> SECTION_SHIFT;
	uint32_t *pt1_entry = (uint32_t *)g_pt1_addr;
	uint32_t flag1 = PT1_FIXED_FLAG_T;
	uint32_t l2_offset;
	uint32_t *pt2_entry;
	uint32_t flag2 = PT2_FIXED_FLAG;

	if(mflag.cacheble == 1)
	{
		flag2 |= (3 << 2);
	}
	flag2 |= mflag.shareble << 10;
	flag2 |= mflag.ng << 11;
	flag2 |= (mflag.ap&0x01) << 5;
	flag2 |= (mflag.ap&0x02) << 8;
	flag1 |= mflag.ns << 3;

	pt1_entry += l1_offset;

	for(i = 0; i < nsections; i++)
	{
		if((*pt1_entry & 0x03) == 0)
		{
			pt2_addr = alloc_pt2(1);
			if(pt2_addr == 0)
			{
				mdbg("do_coarse_map:pt2_addr == 0\n");
				return -1;
			}
			*pt1_entry = vir2phy(pt2_addr, g_v2p_off) | flag1;
			mdbg("do_coarse_map:pt1_entry = 0x%x, *pt1_entry = 0x%x\n", pt1_entry, *pt1_entry);
		}
		else
		{	if(flag1 != (*pt1_entry&PAGE_MASK))
			{
				merr("do_coarse_map:@1 0x%x != 0x%x\n", flag1, *pt1_entry&PAGE_MASK);
				return -1;
			}
			pt2_addr = (*pt1_entry)&(~PAGE_MASK); //maybe leve2 pagetale already exist, we can reuse it, if flag is right.
		}
		pt1_entry++;
		
		pt2_entry = (uint32_t *)pt2_addr;
		l2_offset = (vaddr >> PAGE_SHIFT)&0xff;
		pt2_entry += l2_offset;
		ov = l2_offset;

		while(size && (++ov)) //ov means over the section.
		{
			if((*pt2_entry & 0x03) != 0)
			{
				if(*pt2_entry == (paddr|flag2))
				{
					return 0;
				}
				merr("do_coarse_map:@2 0x%x != 0x%x\n", *pt2_entry, paddr|flag2);
				return -1;
			}
			*pt2_entry = paddr | flag2;
			mdbg("do_coarse_map:pt2_entry = 0x%x, *pt2_entry = 0x%x\n", pt2_entry, *pt2_entry);
			pt2_entry++;
			vaddr += PAGE_SIZE;
			paddr += PAGE_SIZE;
			size  -= PAGE_SIZE;
		}
	}

	return 0;
}

/*
*memory map, section map at first, then coarse table map.
*only support section and short page,
*dose not support super section and large page.
*/
int do_mmu_map(addr_t vaddr, addr_t paddr, size_t size, mmu_flag_t mflag, alloc_pt2_t alloc_pt2)
{
	vaddr = allign_down(vaddr, PAGE_SHIFT);
	paddr = allign_down(paddr, PAGE_SHIFT);
	size = allign_up(size, PAGE_SHIFT);

	int ret = 0;
	
	if(alloc_pt2 == NULL)
	{
		return -1;
	}

loop:
	if(size == 0)
	{
		return 0;
	}

	mdbg("do_mmu_map:vaddr = 0x%x, paddr = 0x%x, size = 0x%x\n", vaddr, paddr, size);
	
	if(((vaddr&SECTION_MASK) != (paddr&SECTION_MASK)) || (size < SECTION_SIZE))
	{
		/*can not do section map for this case.*/
		ret = do_coarse_map(vaddr, paddr, size, mflag , alloc_pt2);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if((vaddr&SECTION_MASK) == 0)
	{
		size_t s_size = size & (~SECTION_MASK);
		ret = do_section_map(vaddr, paddr, s_size, mflag);
		if(ret < 0)
		{
			return -1;
		}
		vaddr += s_size;
		paddr += s_size;
		size &= SECTION_MASK;
		goto loop; //iterator
	}
	else
	{
		size_t p_size = SECTION_SIZE - (vaddr&SECTION_MASK);
		ret = do_coarse_map(vaddr, paddr, p_size, mflag, alloc_pt2);
		if(ret < 0)
		{
			return -1;
		}
		vaddr += p_size;
		paddr += p_size;
		size -= p_size;
		goto loop; //iterator
	}

	return ret;
}

