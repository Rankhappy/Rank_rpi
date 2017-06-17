/*
*Name: list.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef LIST_H
#define LIST_H
typedef struct list_node
{
	struct list_node *prev;
	struct list_node *next;
}list_node_t;

typedef list_node_t list_t;

#define offset(type, member) ((unsigned long)&((type *)0)->member)
#define list_data(n, type, member) \
(type *)((void *)n - offset(type, member))

#define list_init(node)             \
do{                                 \
	(node)->next = (node)->prev = node; \
}while(0)

#define list_empty(node) \
((node)->next == node)

#define list_foreach(head, node) \
for(node = (head)->next; (node) != head; node = (node)->next)

#define list_head(head) ((head)->next)

static void list_add_head(list_node_t *head, list_node_t *node)
{
	head->next->prev = node;
	node->next = head->next;
	head->next = node;
	node->prev = head;
}

static void list_add_tail(list_node_t *head, list_node_t *node)
{
	head->prev->next = node;
	node->prev = head->prev;
	head->prev = node;
	node->next = head;
}

static void list_delete(list_node_t *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	list_init(node);
}

static void list_split(list_node_t *head, list_node_t *node, list_node_t *n)
{
	head->prev->next = n;
	n->prev = head->prev;
	head->prev = node->prev;
	node->prev->next = head;
	n->next = node;
	node->prev = n;
}

static void list_move(list_node_t *head, list_node_t *n)
{
	if(list_empty(head))
	{
		list_init(n);
	}
	else
	{
		list_split(head, head->next, n);
	}
}
#endif

