/*
*Name: heap.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef HEAP_H
#define HEAP_H

#define offset(type, member) ((unsigned long)&((type *)0)->member)
#define heap_data(n, type, member) \
(type *)((void *)n - offset(type, member))

typedef struct heap_node
{
	struct heap_node *parent;
	struct heap_node *lchild;
	struct heap_node *rchild;
}heap_node_t;

typedef int (*heap_compare_fn)(heap_node_t *, heap_node_t *);

typedef struct 
{
	heap_node_t *root; 
	int nodes;
}heap_t;

#define heap_init(h) \
do                   \
{                    \
	(h)->root = NULL;   \
	(h)->nodes = 0;     \
}while(0);

#define heap_node_init(n) \
do                   \
{                    \
	(n)->parent = NULL;   \
	(n)->lchild = NULL;     \
	(n)->rchild = NULL;     \
}while(0);


#define DELCARE_MIN_HEAP(name) \
{                              \
	heap_t name = {            \
		.root = NULL,          \
   		.nodes = 0            \
	}                          \
}

extern void heap_insert(heap_t *, heap_node_t *, heap_compare_fn);
extern heap_node_t *heap_delete(heap_t *, heap_compare_fn);
extern heap_node_t *heap_get(heap_t *);

#endif

