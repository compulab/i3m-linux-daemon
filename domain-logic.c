/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "registers.h"
#include "thread-pool.h"
#include "panel.h"
#include "sensors.h"
#include "cpu-freq.h"
#include "vga-tools.h"
#include "hdd-info.h"

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
	in_processing_remove_request(ATFP_MASK_PENDR0_CPUTR, shared_context);
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
	in_processing_remove_request(ATFP_MASK_PENDR0_CPUFR, shared_context);
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

	if (temp != NULL) {
		slogd("GPUTR: %d [degC]", *temp);
		panel_set_gpu_temp(*temp);
		free(temp);
	}
	else {
		slogw("GPU Temp: abort request");
	}

	in_processing_remove_request(ATFP_MASK_PENDR0_GPUTR, shared_context);
}

static void get_gpu_temperature(void *priv_context, void *shared_context)
{
	int temp;
	int *context;
	int err;

	err = GPU_get_temperature(&temp);
	if (err == 0) {
		context = (int *)malloc(sizeof(int));
		*context = temp;
	}
	else {
		context = NULL;
	}
	thread_pool_add_request(frontend_thread, set_gpu_temperature, context);
}

void panel_update_gpu_temp(void)
{
	thread_pool_add_request(backend_thread, get_gpu_temperature, NULL);
}


/*
 * Getting and setting HDD temperature.
 */

static void set_hdd_temperature(void *priv_context, void *shared_context)
{
	DList *hdd_list = (DList *)priv_context;
	SMARTinfo *si;
	int index;

	index = 0;
	while ((si = dlist_pop_front(hdd_list)) != NULL) {
		if (si->temp_valid) {
			slogd("HDDTR: %s: %u [degC]", si->devname, si->temp);
			panel_set_hdd_temp(index, si->temp);
		}
		else {
			slogd("HDDTR: %s: --", si->devname);
		}

		delete_SMARTinfo(si);
		++index;
	}

	in_processing_remove_request(ATFP_MASK_PENDR0_HDDTR, shared_context);
}

static void get_hdd_temperature(void *priv_context, void *shared_context)
{
	DList *hdd_list;

	hdd_get_temperature(&hdd_list);
	thread_pool_add_request(frontend_thread, set_hdd_temperature, hdd_list);
}

void panel_update_hdd_temp(void)
{
	thread_pool_add_request(backend_thread, get_hdd_temperature, NULL);
}


/*
 * Store daemon postcode in Front Panel register.
 *
 * The purpose of the postcode is to let the FP know,
 * the daemon is up and running.
 * In case daemon postcode was not stored,
 * the FP should display a reminder to install the daemon.
 */

typedef struct {
	unsigned int delay_sec;
	ThreadPoolWork func;
} DelayInfo;

/* backend */
static void backend_delay(void *priv_context, void *shared_context)
{
	DelayInfo *di = (DelayInfo *)priv_context;

	sleep(di->delay_sec);
	thread_pool_add_request(frontend_thread, di->func, (void *)di);
}

/* frontend */
void really_store_daemon_postcode(void *priv_context, void *shared_context)
{
	int err;

	err = panel_store_daemon_postcode();
	if (err == 0) {
		/* success */
		free(priv_context);
		return;
	}

	/* retry */
	thread_pool_add_request(backend_thread, backend_delay, priv_context);
}

DelayInfo *postcode_info;

void FP_store_daemon_postcode(void)
{
	postcode_info = (DelayInfo *)calloc(1, sizeof(DelayInfo));

	postcode_info->delay_sec = ATFP_MAIN_STARTUP_DELAY;
	if (postcode_info->delay_sec >= ATFP_WATCHDOG_DEFAULT_DELAY)
		postcode_info->delay_sec = ATFP_WATCHDOG_DEFAULT_DELAY >> 1;

	postcode_info->func = really_store_daemon_postcode;

	thread_pool_add_request(backend_thread, backend_delay, (void *)postcode_info);
}

