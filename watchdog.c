/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */
/*
 * Watchdog that sends the whole daemon down upon
 * timeout detection.
 * Resolution: seconds
 * Signals employed: SIGUSR2
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "watchdog.h"
#include "stats.h"
#include "common.h"


/* the earliest deadline at head */
static int time_compare(void *inlist, void *outlist)
{
	WDeadline *a = (WDeadline *)inlist;
	WDeadline *b = (WDeadline *)outlist;

	return (int)(b->deadline - a->deadline);
}

static void watchdog_runner_cleanup(void *arg)
{
	pthread_mutex_t *lock = (pthread_mutex_t *)arg;

	pthread_mutex_unlock(lock);
}

static void *watchdog_runner(void *arg)
{
	Watchdog *p = (Watchdog *)arg;
	WDeadline *dl;
	time_t deadline;
	time_t now;
	double delta;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while ( 1 ) {
		pthread_mutex_lock(&p->lock);
		/* cleanup routine to be run upon thread cancellation */
		pthread_cleanup_push(watchdog_runner_cleanup, &p->lock);

		while (dlist_is_empty(p->activelist)) {
			pthread_cond_wait(&p->list_not_empty, &p->lock);
		}
		dl = dlist_peek_front(p->activelist);
		deadline = dl->deadline;

		/* execute cleanup routine */
		pthread_cleanup_pop(true);

		now = time(NULL);
		delta = difftime(now, deadline);

		if (delta >= 0.0) {
			/* timeout */
			sloge("watchdog: timeout exceeded - shutdown daemon");
			kill(0, SIGTERM);
			break;
		}

		/* sleep is interruptible by (SIGUSR2) signal */
		sleep((unsigned int)(-delta));
	}

	return NULL;
}

Watchdog *watchdog_create(void)
{
	Watchdog *p;

	p = (Watchdog *)malloc(sizeof(Watchdog));

	pthread_mutex_init(&p->lock, NULL);
	pthread_cond_init(&p->list_not_empty, NULL);

	p->activelist = dlist_create(time_compare);
	p->freelist = dlist_create(NULL);

	pthread_create(&p->thread, NULL, watchdog_runner, p);

	return p;
}

void watchdog_destroy(Watchdog *p)
{
	WDeadline *dl;

	pthread_cancel(p->thread);
	pthread_join(p->thread, NULL);

	pthread_mutex_destroy(&p->lock);
	pthread_cond_destroy(&p->list_not_empty);

	while ((dl = dlist_pop_front(p->activelist)) != NULL)
		free(dl);

	while ((dl = dlist_pop_front(p->freelist)) != NULL)
		free(dl);

	dlist_destroy(p->activelist);
	dlist_destroy(p->freelist);

	/* zero everything against evil eye */
	memset(p, 0, sizeof(Watchdog));
	free(p);
}

WDeadline *watchdog_submit_deadline(Watchdog *p, unsigned int delta)
{
	WDeadline *dl;
	time_t deadline;

	deadline = time(NULL) + delta;

	pthread_mutex_lock(&p->lock);

	dl = dlist_pop_front(p->freelist);
	if (dl == NULL) {
		dl = (WDeadline *)calloc(1, sizeof(WDeadline));
		if (dl == NULL)
			goto submit_deadline_out;

		stat_inc_watchdog_list_length();
	}

	dl->deadline = deadline;
	dlist_push_ordered(p->activelist, dl);

	pthread_cond_signal(&p->list_not_empty);
	pthread_kill(p->thread, SIGUSR2);

submit_deadline_out:
	pthread_mutex_unlock(&p->lock);
	return dl;
}

void watchdog_clear_deadline(Watchdog *p, WDeadline *dl)
{
	pthread_mutex_lock(&p->lock);
	dlist_remove_node(p->activelist, dl);
	dlist_push_back(p->freelist, dl);
	pthread_mutex_unlock(&p->lock);
}

