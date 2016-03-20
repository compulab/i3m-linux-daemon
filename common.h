/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
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


typedef struct {
	long bitmap;
} InProcessingBitmap;

void in_processing_add_request(long request, InProcessingBitmap *processing);
void in_processing_remove_request(long request, InProcessingBitmap *processing);
long in_processing_get_bitmap(InProcessingBitmap *processing);


#define ATFP_DAEMON_LOCKFILE		"/tmp/airtop-fpsvc.lock"
#define ATFP_SYSLOG_IDENT		"at-fpsvc"
#define ATFP_DAEMON_CONFIGFILE		"/etc/airtop-fpsvc.conf"

/* maximum number of CPU cores */
#define ATFP_MAX_CPU_CORES		8

#define ATFP_FRONTEND_QUEUE_LEN		16
#define ATFP_BACKEND_THREAD_NUM		8
#define ATFP_BACKEND_QUEUE_LEN		16

#define ATFP_MAIN_STARTUP_DELAY		2
#define ATFP_MAIN_POLL_CYCLE		2

#define ATFP_WATCHDOG_DEFAULT_DELAY	5

#define ATFP_DAEMON_POSTCODE_MSB	0xDA
#define ATFP_DAEMON_POSTCODE_LSB	0xE7


#endif	/* _COMMON_H */

