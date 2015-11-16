/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"


Queue *queue_create(size_t elemsize, int length)
{
	Queue *q;

	q = (Queue *)malloc(sizeof(Queue) + (elemsize * length));
	if ( !q ) {
		fprintf(stderr, "Could not allocate queue \n");
		return NULL;
	}

	q->elemsize = elemsize;
	q->maxlen = length;
	q->count = 0;
	q->head = 0;
	q->tail = 0;

	return q;
}

void queue_destroy(Queue *q)
{
	/* zero everything against evil eye */
	memset(q, 0, (sizeof(Queue) + (q->elemsize * q->maxlen)));
	free(q);
}

bool queue_is_empty(Queue *q)
{
	return (q->count <= 0);
}

bool queue_is_full(Queue *q)
{
	return (q->count >= q->maxlen);
}

void queue_pop_front(Queue *q, void *elem)
{
	if (queue_is_empty(q))
		return;

	memcpy(elem, (q->buffer + q->head * q->elemsize), q->elemsize);
	q->head = (q->head + 1) % q->maxlen;
	q->count--;
}

void queue_push_back(Queue *q, void *elem)
{
	memcpy((q->buffer + q->tail * q->elemsize), elem, q->elemsize);
	q->tail = (q->tail + 1) % q->maxlen;
	if (q->count < q->maxlen)
		q->count++;
}


/* unit test */
int queue_test(void)
{
	Queue *q;
	int i;
	int x;
	int expected[] = {5, 6, 7, 3, 4, 4, 4, 4};
	int err = 0;

	q = queue_create(sizeof(int), 5);
	for (i = 0; i < 8; ++i) {
		queue_push_back(q, &i);
	}

	for (i = 0; i < 8; ++i) {
		queue_pop_front(q, &x);
		if (x != expected[i]) {
			err = -i;
			goto test_out;
		}
	}

test_out:
	queue_destroy(q);
	return err;
}

