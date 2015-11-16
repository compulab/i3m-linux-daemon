/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread-pool.h"


typedef struct {
	ThreadPoolWork func;
	void *context;
} ThreadPoolRequest;


static void thread_pool_runner_cleanup(void *arg)
{
	pthread_mutex_t *lock = (pthread_mutex_t *)arg;

	pthread_mutex_unlock(lock);
}

static void *thread_pool_runner(void *arg)
{
	ThreadPool *p = (ThreadPool *)arg;
	ThreadPoolRequest req;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while ( 1 ) {
		pthread_mutex_lock(&p->lock);

		/* cleanup routine to be run upon thread cancellation */
		pthread_cleanup_push(thread_pool_runner_cleanup, &p->lock);

		while (queue_is_empty(p->work_queue)) {
			pthread_cond_wait(&p->queue_not_empty, &p->lock);
		}

		queue_pop_front(p->work_queue, &req);
		pthread_cond_signal(&p->queue_not_full);

		/* execute cleanup routine */
		pthread_cleanup_pop(true);

		req.func(req.context, p->shared_context);
	}

	return NULL;
}

ThreadPool *thread_pool_create(int thread_count, int queue_size, void *shared_context)
{
	ThreadPool *p;
	int i;
	int err;

	p = (ThreadPool *)malloc(sizeof(ThreadPool) + (sizeof(pthread_t) * thread_count));
	if ( !p ) {
		fprintf(stderr, "Could not allocate thread pool \n");
		return NULL;
	}

	pthread_mutex_init(&p->lock, NULL);
	pthread_cond_init(&p->queue_not_empty, NULL);
	pthread_cond_init(&p->queue_not_full, NULL);

	p->work_queue = queue_create(sizeof(ThreadPoolRequest), queue_size);

	p->shared_context = shared_context;

	/*
	 * As the threads are born live,
	 * they should be started when all thread pool data fields are well initialized.
	 */
	p->thread_count = 0;
	for (i = 0; i < thread_count; ++i) {
		err = pthread_create(&p->thread_pool[i], NULL, thread_pool_runner, p);
		if ( err ) {
			fprintf(stderr, "Could not spawn a thread: %d \n", err);
			break;
		}

		p->thread_count++;
	}

	return p;
}

void thread_pool_destroy(ThreadPool *p)
{
	int i;

	/* join all the threads */
	for (i = 0; i < p->thread_count; ++i) {
		pthread_cancel(p->thread_pool[i]);
	}
	for (i = 0; i < p->thread_count; ++i) {
		pthread_join(p->thread_pool[i], NULL);
	}

	pthread_mutex_destroy(&p->lock);
	pthread_cond_destroy(&p->queue_not_empty);
	pthread_cond_destroy(&p->queue_not_full);
	queue_destroy(p->work_queue);
	/* zero everything against evil eye */
	memset(p, 0, (sizeof(ThreadPool) + (sizeof(pthread_t) * p->thread_count)));
	free(p);
}

void thread_pool_add_request(ThreadPool *p, ThreadPoolWork func, void *context)
{
	ThreadPoolRequest req = {
		.func = func,
		.context = context,
	};

	pthread_mutex_lock(&p->lock);

	while (queue_is_full(p->work_queue)) {
		pthread_cond_wait(&p->queue_not_full, &p->lock);
	}

	queue_push_back(p->work_queue, &req);

	pthread_cond_signal(&p->queue_not_empty);
	pthread_mutex_unlock(&p->lock);
}


int thread_pool_test(void)
{
	typedef struct {
		double x;
		double y;
		double ans;
		double ref;
		void (*func)(void *, void *);
	} Test;

	const int test_length = 50000;
	Test t[test_length];

	void func_plus(void *a, void *b)
	{
		Test *t = (Test *)a;

		usleep(1);
		t->ans = t->x + t->y;
	}

	void func_minus(void *a, void *b)
	{
		Test *t = (Test *)a;

		usleep(2);
		t->ans = t->x - t->y;
	}

	ThreadPool *tp;
	int i;
	int err = 0;

	tp = thread_pool_create(4, 10, NULL);

	srand(0);
	for (i = 0; i < test_length; ++i) {
		t[i].x = (double)rand() * 100.0 / (double)RAND_MAX;
		t[i].y = (double)rand() * 100.0 / (double)RAND_MAX;
		t[i].ans = 0.0;
		if (i % 2 == 0) {
			t[i].ref = t[i].x + t[i].y;
			t[i].func = func_plus;
		}
		else {
			t[i].ref = t[i].x - t[i].y;
			t[i].func = func_minus;
		}
	}
	for (i = 0; i < test_length; ++i) {
		thread_pool_add_request(tp, t[i].func, &t[i]);
	}

	/* no way to wait for completion */

	for (i = 0; i < test_length; ++i) {
		if (t[i].ans != t[i].ref) {
			err = -i;
			goto test_out;
		}
	}

test_out:
	thread_pool_destroy(tp);
	return err;
}

