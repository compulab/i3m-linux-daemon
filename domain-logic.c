/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "thread-pool.h"
#include "panel.h"
#include "sensors.h"
#include "cpu-freq.h"
#include "vga-tools.h"

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
		slogd("CPUTR: Core %d: %d [degC]", core_id, temp);
		err = panel_set_temperature(core_id, temp);
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


/*
 * Getting and setting core frequency.
 */

typedef struct {
	int num_cores;
	int freq[ATFP_MAX_CPU_CORES];
} CpuFreq;

static void set_frequency(void *priv_context, void *shared_context)
{
	CpuFreq *context = (CpuFreq *)priv_context;
	int core_id;
	int freq;
	int err;

	for (core_id = 0; core_id < context->num_cores; ++core_id) {
		freq = context->freq[core_id];
		slogd("CPUFR: %d [MHz]", freq);
		err = panel_set_frequency(core_id, freq);
		if ( err )
			break;
	}

	free(context);
}

static void get_frequency(void *priv_context, void *shared_context)
{
	CpuFreq *context = (CpuFreq *)malloc(sizeof(CpuFreq));

	context->num_cores = ATFP_MAX_CPU_CORES;

	cpu_freq_get_list(&context->num_cores, context->freq);
	thread_pool_add_request(frontend_thread, set_frequency, context);
}

void panel_update_frequency(void)
{
	thread_pool_add_request(backend_thread, get_frequency, NULL);
}


/* 
 * Getting and setting GPU temperature.
 */

static void set_gpu_temperature(void *priv_context, void *shared_context)
{
	int *temp = (int *)priv_context;

	slogd("GPUTR: %d [degC]", *temp);
	panel_set_gpu_temp(*temp);
	free(temp);
}

static void get_gpu_temperature(void *priv_context, void *shared_context)
{
	int temp;
	int *context;
	int err;

	err = GPU_get_temperature(&temp);
	if (err) {
		slogw("GPU Temp: abort request");
		return;
	}

	context = (int *)malloc(sizeof(int));
	*context = temp;
	thread_pool_add_request(frontend_thread, set_gpu_temperature, context);
}

void panel_update_gpu_temp(void)
{
	thread_pool_add_request(backend_thread, get_gpu_temperature, NULL);
}

