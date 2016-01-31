/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include "dlist.h"


typedef struct {
	DListNode dlist_hook;
	time_t deadline;
} WDeadline;

typedef struct {
	pthread_mutex_t lock;
	pthread_cond_t list_not_empty;
	pthread_t thread;
	DList *activelist;
	DList *freelist;
} Watchdog;


Watchdog *watchdog_create(void);
void watchdog_destroy(Watchdog *p);
WDeadline *watchdog_submit_deadline(Watchdog *p, unsigned int delta);
void watchdog_clear_deadline(Watchdog *p, WDeadline *dl);

#endif	/* _WATCHDOG_H */

