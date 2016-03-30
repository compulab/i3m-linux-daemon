/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _GPU_THERMALD_H
#define _GPU_THERMALD_H

#include <syslog.h>


/* logging: syslog */
#define sloge(...)			syslog(LOG_ERR,     __VA_ARGS__)
#define slogw(...)			syslog(LOG_WARNING, __VA_ARGS__)
#define slogn(...)			syslog(LOG_NOTICE,  __VA_ARGS__)
#define slogi(...)			syslog(LOG_INFO,    __VA_ARGS__)
#define slogd(...)			syslog(LOG_DEBUG,   __VA_ARGS__)

#define GPUD_SYSLOG_ID			"gpu-thermd"


#define GPUD_CONFIGFILE			"/etc/gpu-thermald.conf"
#define GPUD_DEFAULT_TEMP		100


#endif	/* _GPU_THERMALD_H */

