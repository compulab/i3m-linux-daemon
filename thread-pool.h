/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <pthread.h>

#include "queue.h"

typedef void (*ThreadPoolWork)(void *priv_context, void *shared_context);

typedef struct {
	pthread_mutex_t lock;
	pthread_cond_t queue_not_empty;
	pthread_cond_t queue_not_full;
	Queue *work_queue;
	void *shared_context;
	int thread_count;
	/* 'thread_pool' must be the last */
	pthread_t thread_pool[0];
} ThreadPool;

ThreadPool *thread_pool_create(int thread_count, int queue_size, void *shared_context);
void thread_pool_join(ThreadPool *p);
void thread_pool_destroy(ThreadPool *p);
void thread_pool_add_request(ThreadPool *p, ThreadPoolWork func, void *context);

int thread_pool_test(void);

#endif	/* _THREAD_POOL_H */

