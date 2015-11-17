/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "thread-pool.h"
#include "panel.h"
#include "sensors.h"

/*
 * Getting and setting core temperature.
 */

typedef struct {
	int num_sensors;
	int temp[ATFP_MAX_CPU_CORES];
} CpuTemp;

static void set_temperature(void *priv_context, void *shared_context)
{
	CpuTemp *context = (CpuTemp *)priv_context;
	int core_id;
	int temp;
	int err;

	for (core_id = 0; core_id < context->num_sensors; ++core_id) {
		temp = context->temp[core_id];
		printf("CPUTR: Core %d: %d [deg] \n", core_id, temp);
		err = panel_set_temperature(panel_desc, core_id, temp);
		if ( err )
			break;
	}

	free(context);
}

static void get_temperature(void *priv_context, void *shared_context)
{
	int temp;
	int core_id;
	int core_id_save;
	int err;
	CpuTemp *context = (CpuTemp *)malloc(sizeof(CpuTemp));

	context->num_sensors = 0;
	for (core_id = 0; core_id >= 0;) {
		core_id_save = core_id;
		err = sensors_coretemp_read(&core_id, &temp);
		if ( err )
			break;

		context->temp[core_id_save] = temp;
		context->num_sensors++;
	}

	thread_pool_add_request(frontend_thread, set_temperature, context);
}

void panel_update_temperature(void)
{
	thread_pool_add_request(backend_thread, get_temperature, NULL);
}

