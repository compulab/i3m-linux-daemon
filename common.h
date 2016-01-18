/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <syslog.h>

#include "thread-pool.h"

extern ThreadPool *frontend_thread;
extern ThreadPool *backend_thread;


#define sloge(...)			syslog(LOG_ERR,     __VA_ARGS__)
#define slogw(...)			syslog(LOG_WARNING, __VA_ARGS__)
#define slogn(...)			syslog(LOG_NOTICE,  __VA_ARGS__)
#define slogi(...)			syslog(LOG_INFO,    __VA_ARGS__)
#define slogd(...)			syslog(LOG_DEBUG,   __VA_ARGS__)


#define ATFP_DAEMON_LOCKFILE		"/tmp/airtop-fpsvc.lock"
#define ATFP_SYSLOG_IDENT		"at-fpsvc"

/* maximum number of CPU cores */
#define ATFP_MAX_CPU_CORES		8

#define ATFP_FRONTEND_QUEUE_LEN		16
#define ATFP_BACKEND_THREAD_NUM		8
#define ATFP_BACKEND_QUEUE_LEN		16

#define ATFP_MAIN_STARTUP_DELAY		2
#define ATFP_MAIN_POLL_CYCLE		2

#define ATFP_WATCHDOG_DEFAULT_DELAY	5


#endif	/* _COMMON_H */

