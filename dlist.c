/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */
/*
 * Doubly Linked List
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "dlist.h"


/*
 * DList: create
 * Args:
 * compare - comparison function used for ordered insertion / removal
 */
DList *dlist_create(DListCompare compare)
{
	DList *list;

	list = calloc(1, sizeof(DList));
	list->compare = compare;

	return list;
}

void dlist_destroy(DList *list)
{
	/* assert(list->count == 0); */
	memset(list, 0, sizeof(DList));
	free(list);
}

/*
 * DList: attach a node to list tail
 */
void dlist_push_back(DList *list, void *_node)
{
	DListNode *node = (DListNode *)_node;

	if (list->count == 0)
		list->head = node;
	else
		list->tail->prev = node;

	node->next = list->tail;	/* NULL is an option */
	list->tail = node;
	node->prev = NULL;
	++(list->count);
}

void dlist_push_ordered(DList *list, void *_node)
{
	DListNode *node = (DListNode *)_node;
	DListNode *current;
	int cmp;

	for (current = list->tail; current != NULL; current = current->next) {
		cmp = list->compare(current, node);
		if (cmp >= 0) {
			/* insert 'node' past current */
			if (current->prev != NULL)
				current->prev->next = node;
			else
				list->tail = node;
			node->prev = current->prev;
			node->next = current;
			current->prev = node;
			goto push_ordered_success;
		}
	}

	/* insert 'node' at 'head' position */
	if (list->count == 0)
		list->tail = node;
	else
		list->head->next = node;
	node->prev = list->head;	/* NULL is an option */
	node->next = NULL;
	list->head = node;

push_ordered_success:
	++(list->count);
}

void *dlist_pop_front(DList *list)
{
	DListNode *node;

	if (list->count == 0)
		return NULL;

	node = list->head;
	list->head = node->prev;	/* NULL is an option */

	if (list->count == 1)
		list->tail = NULL;
	else
		list->head->next = NULL;

	--(list->count);

	node->next = node->prev = NULL;
	return node;
}

/*
 * DList: remove a node
 * Args:
 * _node - a pointer to the node being removed
 */
void dlist_remove_node(DList *list, void *_node)
{
	DListNode *node = (DListNode *)_node;

	if (node->prev != NULL)
		node->prev->next = node->next;
	else
		list->tail = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;
	else
		list->head = node->prev;

	node->next = node->prev = NULL;
	--(list->count);
}

/*
 * DList: remove a node _like_ the one provided
 * Args:
 * _node - a pointer to a 'reference node', equal to the node being removed
 *         according to compare().
 */
void *dlist_remove_by_sample(DList *list, void *_node)
{
	DListNode *node = (DListNode *)_node;
	DListNode *current;
	int cmp;

	for (current = list->head; current != NULL; current = current->prev) {
		cmp = list->compare(current, node);
		if (cmp == 0) {
			/* identified */
			dlist_remove_node(list, current);
			break;
		}
	}

	return current;
}

void *dlist_peek_front(DList *list)
{
	return list->head;
}

bool dlist_is_empty(DList *list)
{
	return (list->count <= 0);
}



/* unit test */
typedef struct {
	DListNode dlist_hook;
	char value;
} MyChar;

static int MyChar_compare(void *inlist, void *outlist)
{
	MyChar *a = (MyChar *)inlist;
	MyChar *b = (MyChar *)outlist;

	return (int)(a->value - b->value);
}

int dlist_test(void)
{
	DList *dlist;
	MyChar a, b, c, d, e, f, g;
	MyChar *mc;
	int i;
	int err = 0;

	a.value = 'a';
	b.value = 'b';
	c.value = 'c';
	d.value = 'd';
	e.value = 'e';
	f.value = 'f';
	g.value = 'g';

	dlist = dlist_create(MyChar_compare);

	dlist_push_ordered(dlist, &d);
	dlist_push_ordered(dlist, &c);
	dlist_push_ordered(dlist, &e);
	dlist_push_ordered(dlist, &a);
	dlist_push_ordered(dlist, &g);
	dlist_push_ordered(dlist, &b);
	dlist_push_ordered(dlist, &f);

	for (i = 0; i < 7; ++i) {
		const char order[] = "gfedcba";
		mc = dlist_pop_front(dlist);
		if (mc->value != order[i]) {
			err = -i;
			goto test_out;
		}
	}

	dlist_push_back(dlist, &a);
	dlist_push_back(dlist, &b);
	dlist_push_back(dlist, &c);
	dlist_push_back(dlist, &d);
	dlist_push_back(dlist, &e);
	dlist_push_back(dlist, &f);
	dlist_push_back(dlist, &g);

	for (i = 0; i < 10; ++i) {
		const char order[] = "xgaecfbdxx";
		MyChar sample;

		sample.value = order[i];
		mc = dlist_remove_by_sample(dlist, &sample);
		if (((mc != NULL) ? mc->value : 'x') != order[i]) {
			err = -i;
			goto test_out;
		}
	}

test_out:
	dlist_destroy(dlist);
	return err;
}

