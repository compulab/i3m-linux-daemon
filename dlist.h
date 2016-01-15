/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */
/*
 * Doubly Linked List
 */

#ifndef _DLIST_H
#define _DLIST_H

typedef int (*DListCompare)(void *inlist, void *outlist);

typedef struct DListNode {
	struct DListNode *next;
	struct DListNode *prev;
} DListNode;

typedef struct DList {
	unsigned int count;
	DListNode *head;
	DListNode *tail;
	DListCompare compare;
} DList;


DList *dlist_create(DListCompare compare);
void dlist_destroy(DList *list);
void dlist_push_back(DList *list, void *_node);
void dlist_push_ordered(DList *list, void *_node);
void *dlist_pop_front(DList *list);
void dlist_remove_node(DList *list, void *_node);
void *dlist_remove_ordered(DList *list, void *_node);
void *dlist_peek_front(DList *list);
bool dlist_is_empty(DList *list);

int dlist_test(void);

#endif	/* _DLIST_H */

