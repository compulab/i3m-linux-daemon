/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "thread-pool.h"

extern ThreadPool *frontend_thread;
extern ThreadPool *backend_thread;

/* maximum number of CPU cores */
#define ATFP_MAX_CPU_CORES		8

#define ATFP_FRONTEND_QUEUE_LEN		16
#define ATFP_BACKEND_THREAD_NUM		8
#define ATFP_BACKEND_QUEUE_LEN		16

#define ATFP_MAIN_STARTUP_DELAY		2
#define ATFP_MAIN_POLL_CYCLE		2

#define ATFP_WATCHDOG_DEFAULT_DELAY	5


#endif	/* _COMMON_H */

