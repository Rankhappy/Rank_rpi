/*
*Name: heap.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/
#include "type.h"
#include "assert.h"
#include "heap.h"

#define heap_dbg(fmt, args...) printf("[RANK][HEAP][DEBUG]"fmt, ##args)
#define heap_err(fmt, args...) printf("[RANK][HEAP][ERROR]"fmt, ##args)

static void __heap_exchange_node(heap_t *h, heap_node_t *p, heap_node_t *c)
{
	heap_node_t t;
	
	/*c is child, and his parent is p*/
	assert((c->parent == p) && (p->lchild == c || p->rchild == c));

	/*exchange the node data.*/
	t = *c;
	*c = *p;
	*p = t;
	
	if(c->parent)
	{
		assert((c->parent->lchild == p) || (c->parent->rchild == p));
		if(c->parent->lchild == p)
		{
			c->parent->lchild = c;
		}
		else if(c->parent->rchild == p)
		{
			c->parent->rchild = c;
		}
		else
		{
			/*it is impossible.*/
			assert(0);
		}
	}
	else
	{
		/*p is the root node before, and now it should be c*/
		h->root = c; 
	}
	
	if(c->lchild == c)
	{
		c->lchild = p;
		if(c->rchild)
		{
			c->rchild->parent = c;
		}
	}
	else if(c->rchild == c)
	{
		c->rchild = p;
		if(c->lchild)
		{
			c->lchild->parent = c;
		}		
	}
	else
	{
		/*It is impossible.*/
		assert(0);
	}
	
	p->parent = c;
	if(p->lchild)
	{
		p->lchild->parent = p;
	}
	if(p->rchild)
	{
		p->rchild->parent = p;
	}
}

static void __heap_insert_node(heap_t *h, heap_node_t *n, heap_compare_fn compare)
{
	heap_node_t *t;
	unsigned long lr = 0;
	int i, j;
	
	/*We do not need to check the input parameter here
	because the unique caller is "heap_insert_node" and 
	we have already checked there.*/
	
	/*Here the root should not be NULL*/
	t = h->root;
	
	/*At first have to find the position to insert, and
	it is at the tail of the queue.*/
	for(i = 0, j = h->nodes+1; j>=2; j /= 2, i++)
	{
		/*will "lr" be overflow, i think it is impossible.*/
		lr = (lr << 1) | (j & 0x01); 
	}
	while(i > 1)
	{
		/*1:right, 0:left*/
		if(lr & 0x01)
		{
			t = t->rchild;
		}
		else
		{
			t = t->lchild;
		}
		lr >>= 1;
		i--;
	}
	
	/*Here the node must be exist.*/
	assert(t);
	
	/*Insert the new node.*/
	if(lr & 0x01)
	{
		/*In this case, "lchild" must be exist.*/
		assert(t->lchild); 
		t->rchild = n;
	}
	else
	{
		/*IN this case "rchild" must be NULL;*/
		assert(t->rchild == NULL); 
		t->lchild = n;
	}
	n->parent = t;
	
	h->nodes++;
	
	/*Fix up the heap by rising the node*/
	while((n->parent != NULL) && (compare(n->parent, n) >= 0))
	{
		__heap_exchange_node(h, n->parent, n);
	}
}

/*export to user*/
void heap_insert(heap_t *h, heap_node_t *n, heap_compare_fn compare)
{
	/*must not be NULL*/
	if(h == NULL || n == NULL)
	{
		heap_err("The heap or heap node is NULL.\n");
		return;
	}
	/*compare function should be set*/
	if(compare == NULL)
	{
		heap_err("please set the compare function.\n");
		return;
	}
	
	if(h->root == NULL)
	{
		/*if root is NULL, then the nodes must be zero*/
		assert(h->nodes == 0);
		h->root = n;
		h->nodes++;
		return;
	}
	
	__heap_insert_node(h, n, compare);
}

static void __heap_fixup(heap_t *h, heap_node_t *n, heap_compare_fn compare)
{
	heap_node_t *min;
	
	/*Terminate the recursion when the node is NULL*/
	if(n == NULL)
	{
		return;
	}
	
	/*Compare the node with his chidren to find the 
	minimum node, and take the minumum node as the parent.*/
	min = n;
	if(n->lchild)
	{
		if(compare(min, n->lchild) > 0)
		{
			min = n->lchild;
		}
	}
	if(n->rchild)
	{
		if(compare(min, n->rchild) > 0)
		{
			min = n->rchild;
		}
	}
	
	if(min != n)
	{
		/*In this case, we should exchange the node.*/
		if(n->parent)
			assert(n->parent != min);
		__heap_exchange_node(h, n, min);
		/*Fix up the heap by declining the node.*/
		__heap_fixup(h, n, compare);
	}
}

static void _heap_replace_root(heap_t *h, heap_node_t *t, heap_compare_fn compare)
{	
	heap_node_t *r = h->root;
	
	if(t->parent->lchild == t)
	{
		t->parent->lchild = NULL;
	}
	
	if(t->parent->rchild == t)
	{
		t->parent->rchild = NULL;
	}
	
	*t = *r;
	if(t->lchild)
	{
		t->lchild->parent = t;
	}
	if(t->rchild)
	{
		t->rchild->parent = t;
	}
	h->root = t;
	
	__heap_fixup(h, h->root, compare);
}

static void __heap_replace_root(heap_t *h, heap_compare_fn compare)
{
	heap_node_t *t;
	unsigned long lr = 0;
	int i, j;
	
	t = h->root;
	
	/*Before we delete the root node, should find a node to 
	replace the root node, usually we use the tail node as best*/
	for(i = 0, j = h->nodes; j>=2; j /= 2, i++)
	{
		lr = (lr << 1) | (j & 0x01); 
	}
	while(i > 0)
	{
		if(lr & 0x01)
		{
			t = t->rchild;
		}
		else
		{
			t = t->lchild;
		}
		lr >>= 1;
		i--;
	}
	
	/*t is the tail node, and must be exist.*/
	assert(t); 
	
	_heap_replace_root(h, t, compare);
}

/*export to user*/
heap_node_t *heap_delete(heap_t *h, heap_compare_fn compare)
{
	heap_node_t *r;
	
	/*must not be NULL*/
	if(h == NULL)
	{
		heap_err("The heap is NULL.\n");
		return NULL;
	}
	/*compare function should be set*/
	if(compare == NULL)
	{
		heap_err("please set the compare function.\n");
		return NULL;
	}
	if(h->root == NULL)
	{
		assert(h->nodes == 0);
		return NULL;
	}

	r = h->root;
	
	/*If only has one node, skip*/
	if(h->nodes > 1)
	{
		__heap_replace_root(h, compare);
	}
	
	h->nodes--;
	
	if(h->nodes == 0)
	{
		h->root = NULL;
	}
	
	return r;
}

/*export to user*/
heap_node_t *heap_get(heap_t *h)
{
	/*must not be NULL*/
	if(h == NULL)
	{
		heap_err("The heap is NULL.\n");
		return NULL;
	}
	if(h->root == NULL)
	{
		assert(h->nodes == 0);
		return NULL;
	}
	
	return h->root;
}

