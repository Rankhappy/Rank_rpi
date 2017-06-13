#include "list.h"
#include "type.h"

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1<<PAGE_SHIFT)
#define PAGE_MASK (PAGE_SIZE - 1)
#define PAGE_ALIGN (~PAGE_MASK)

#define PA2VA(x) ((x)+0x40000000)

#define SIZE2NPAGES(size) (((size - 1) >> PAGE_SHIFT) + 1)
#define MAX_MALLOC_SHIFT 22
#define MIN_MALLOC_SHIFT 4

#define MB_INFO_SIZE (sizeof(mm_block_t))

#define MB_FLAG_INSIDE 0x01

typedef struct 
{
	size_t block_size;
	size_t nblocks;
	uint32_t flags;
	list_t partial_block;
}mm_pool_t;

typedef struct 
{
	addr_t start;
	int next_avilable;
	int avilables;
	list_node_t node;
	mm_pool_t *mp;
}mm_block_t;

static mm_pool_t malloc_pool[MAX_MALLOC_SHIFT - MIN_MALLOC_SHIFT + 1];
static mm_pool_t mb_pool;

extern void set_page_info(page_t pages, void *data);
extern void *get_page_info(page_t pages);

void init_mm_pool(void)
{
	int i;

	printf("mb_pool = 0x%08x\n", (uint32_t)&mb_pool);
	
	mb_pool.block_size = MB_INFO_SIZE;
	mb_pool.nblocks = (PAGE_SIZE-MB_INFO_SIZE)/mb_pool.block_size;
	mb_pool.flags = MB_FLAG_INSIDE;
	list_init(&mb_pool.partial_block);
	
	for(i = 0; i <= MAX_MALLOC_SHIFT - MIN_MALLOC_SHIFT; i++)
	{
		malloc_pool[i].block_size = 1 << (i + MIN_MALLOC_SHIFT);
		malloc_pool[i].nblocks = (SIZE2NPAGES(malloc_pool[i].block_size)<<PAGE_SHIFT)/malloc_pool[i].block_size;
		malloc_pool[i].flags = 0;
		list_init(&malloc_pool[i].partial_block);
	}
}

void *mm_alloc(mm_pool_t *mp);
static mm_block_t* init_mm_block(mm_pool_t *mp, addr_t vaddr)
{
	int i;
	mm_block_t *mb;

	if(mp->flags & MB_FLAG_INSIDE)
	{
		mb = (mm_block_t *)(vaddr+PAGE_SIZE-MB_INFO_SIZE);
	}
	else
	{
		mb = (mm_block_t *)mm_alloc(&mb_pool);
		if(mb == NULL)
		{
			return NULL;
		}
	}
	mb->start = vaddr;
	mb->next_avilable = 0;
	mb->avilables = mp->nblocks;
	mb->mp = mp;
	list_init(&mb->node);
	
	for(i = 0; i < mp->nblocks; i++)
	{
		*(uint32_t *)(mb->start + i*mp->block_size) = i+1;
	}
	
	list_add_tail(&mp->partial_block, &mb->node);
	
	return mb;
}

void *mm_alloc(mm_pool_t *mp)
{	
	page_t pages;
	mm_block_t *mb;
	list_node_t *mb_node;
	void *addr;

	if((list_empty(&mp->partial_block)))
	{
		//printf("npages = %ld\n", SIZE2NPAGES(mp->block_size));
		mm_block_t *mbx;
		pages = alloc_pages(SIZE2NPAGES(mp->block_size));
		if(pages < 0)
		{
			return NULL;
		}
		mbx = init_mm_block(mp, PA2VA(pages << PAGE_SHIFT));
		if(mbx == NULL)
		{
			return NULL;
		}
		set_page_info(pages, (void *)mbx);
	}

	mb_node = list_head(&mp->partial_block);
	mb = list_data(mb_node, mm_block_t, node);
	mb->avilables--;

	addr = (void *)(mb->start + mb->next_avilable * mp->block_size);
	
	if(mb->avilables == 0)
	{
		list_delete(&mb->node);
	}
	else
	{
		mb->next_avilable = *(uint32_t *)addr;
	}
	
	return (void *)addr;
}

void mm_free(addr_t addr)
{
	page_t pages = (page_t)(addr >> PAGE_SHIFT);
	mm_block_t *mb = (mm_block_t *)get_page_info(pages);
	mm_pool_t *mp = mb->mp;
	
	mb->avilables++;
	if(mb->avilables == 1)
	{
		list_delete(&mb->node);
		list_add_tail(&mp->partial_block, &mb->node);
	}
	else if(mb->avilables == mp->nblocks)
	{
		list_delete(&mb->node);
		if(mp->flags & MB_FLAG_INSIDE)
		{
			mm_free((addr_t)(mb));
		}
		free_pages((page_t)(addr >> PAGE_SHIFT), SIZE2NPAGES(mp->block_size));
		return;
	}
	
	*(uint32_t *)(addr) = mb->next_avilable;
	//printf("mb->next_avilable = %d\n", mb->next_avilable);
	mb->next_avilable = (addr - mb->start)/mp->block_size;
}

static int Size2Index(size_t size)
{	
	int index;
	
	if((size == 0) || (size > (1<<MAX_MALLOC_SHIFT)))
	{
		return -1;
	}
	
	if(size < (1<<MIN_MALLOC_SHIFT))
	{
		size = 1<<MIN_MALLOC_SHIFT;
	}
	
	for(index = MAX_MALLOC_SHIFT; index >= MIN_MALLOC_SHIFT; index--)
	{
		if(size & (1 << index))
		{
			if(size & ((1 << index) - 1))
			{
				index++;
			}
			return index - MIN_MALLOC_SHIFT;
		}
	}
	
	return -1;
}

void *kmalloc(size_t size)
{
	return mm_alloc(&malloc_pool[Size2Index(size)]);
}

void kfree(void *addr)
{
	mm_free((addr_t)addr);
}

