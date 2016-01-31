/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdbool.h>

typedef struct {
	size_t elemsize;
	int maxlen;
	int count;
	int head;
	int tail;
	/* 'buffer' must be the last */
	unsigned char buffer[0];
} Queue;


Queue *queue_create(size_t elemsize, int length);
void queue_destroy(Queue *q);
bool queue_is_empty(Queue *q);
bool queue_is_full(Queue *q);
void queue_pop_front(Queue *q, void *elem);
void queue_push_back(Queue *q, void *elem);

int queue_test(void);

#endif	/* _QUEUE_H */

