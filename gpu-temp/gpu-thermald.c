/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 *
 * This daemon limits GPU temperature controlling its power.
 * Reference:
 * [1] https://developer.nvidia.com/nvidia-management-library-nvml
 * [2] https://en.wikipedia.org/wiki/PID_controller
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <unistd.h>

#include "gpu-thermald.h"
#include "gpu-options.h"
#include "../nvml-tools.h"


#define goto_if(cond, line, label)		\
	do {					\
		if ( (cond) ) {			\
			line = __LINE__;	\
			goto label;		\
		}				\
	} while (0)

#define clamp(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })


typedef struct {
	unsigned int powerLimitMin;
	unsigned int powerLimitMax;
	unsigned int powerLimitActual;
} GPUPower;

static GPUPower gpu_power;
static GPUOptions options;
NvmlHandle *nvmlh;


static void initialize(void)
{
	nvmlReturn_t err;
	nvmlEnableState_t state = NVML_FEATURE_DISABLED;
	int line;

	/* set up logging */
	openlog(GPUD_SYSLOG_ID, LOG_PID, LOG_USER);
	setlogmask(LOG_UPTO(options.loglevel));

	/*
	 * NVML library is loaded manually, in order to cope gracefully
	 * with its (possible) absence.
	 */
	nvmlh = nvml_init();
	goto_if((nvmlh == NULL), line, init_out0);

	err = nvmlh->nvmlDeviceGetPowerManagementMode(nvmlh->device, &state);
	goto_if((err != NVML_SUCCESS) || (state != NVML_FEATURE_ENABLED), line, init_out1);

	/*
	 * It is believed, GPU has to be in persistent mode,
	 * in order to STAY power-limited.
	 */
	err = nvmlh->nvmlDeviceSetPersistenceMode(nvmlh->device, NVML_FEATURE_ENABLED);
	goto_if((err != NVML_SUCCESS), line, init_out1);

	err = nvmlh->nvmlDeviceGetPowerManagementLimitConstraints(nvmlh->device,
								&gpu_power.powerLimitMin,
								&gpu_power.powerLimitMax);
	goto_if((err != NVML_SUCCESS), line, init_out1);

	err = nvmlh->nvmlDeviceGetPowerManagementLimit(nvmlh->device, &gpu_power.powerLimitActual);
	goto_if((err != NVML_SUCCESS), line, init_out1);

	slogn("GPU Thermal Daemon -- start");
	return;

init_out1:
	nvml_cleanup(0, nvmlh);

init_out0:
	sloge("init: could not initialize NVML: %d [line %d]", err, line);
	fprintf(stderr, "init: could not initialize NVML: %d [line %d] \n", err, line);
	closelog();
	exit(1);
}

static void cleanup(void)
{
	slogn("GPU Thermal Daemon -- stop");

	nvmlh->nvmlDeviceSetPowerManagementLimit(nvmlh->device, gpu_power.powerLimitActual);
	nvml_cleanup(0, nvmlh);
	closelog();
}

static void SIGTERM_handler(int signo)
{
	exit(0);
}

static void SIGALRM_handler(int signo)
{
	/* nothing */
}


/*
 * Control system
 *
 * The control system operates in 3 ranges:
 * 1. (almost) no control:  T < LAMBDA * T_max
 * 2. normal control:       LAMBDA * T_max < T < T_max
 * 3. emergency control:    T > T_max
 *
 * Notice on each of control ranges:
 * 1. When the physical system is cooling down, do not discard I-term
 *    at once, as the temperature tends to jump up steeply,
 *    in case the load is applied back.
 * 2. Small I-term operates against P-term, making sure we do not
 *    limit the power too much.
 * 3. Common control loop.
 */
#define TEMP_CTRL_DELTAT		5
#define LAMBDA				0.9

/* (almost) no control range */
#define ncP_FACTOR			0	/* -- */
#define ncI_FACTOR			(20.0 * TEMP_CTRL_DELTAT)
#define ncD_FACTOR			0	/* -- */

/* normal control range */
#define nP_FACTOR			1500.0
#define nI_FACTOR			(4.0 * TEMP_CTRL_DELTAT)
#define nD_FACTOR			0	/* -- */

/* emergency control range */
#define eP_FACTOR			1500.0
#define eI_FACTOR			(40.0 * TEMP_CTRL_DELTAT)
#define eD_FACTOR			0	/* -- */

static double err_integral = 0.0;

static unsigned int pid_control(unsigned int _temp, unsigned int _temp_limit)
{
	static unsigned int count = 0;

	double temp = (double)_temp;
	double temp_max = (double)_temp_limit;
	double power_max = (double)gpu_power.powerLimitActual;
	double power_min = (double)gpu_power.powerLimitMin;
	double pl;
	double p;
	double i;

	if (temp <= LAMBDA * temp_max) {
		/* (almost) no control required */
		p = 0.0;
		i = 0.0;

		if (err_integral < 0.0) {
			i = err_integral + ncI_FACTOR * (temp_max - temp);
			if (i > 0.0)
				i = 0.0;
		}
	}
	else if (temp <= temp_max) {
		/* normal control */
		p = nP_FACTOR * (LAMBDA * temp_max - temp);
		i = err_integral + nI_FACTOR * (temp_max - temp);
	}
	else {
		/* emergency control */
		p = eP_FACTOR * (LAMBDA * temp_max - temp);
		i = err_integral + eI_FACTOR * (temp_max - temp);
	}

	pl = power_max + p + i;
	if (pl > power_max) {
		pl = power_max;
	}
	else if (pl < power_min) {
		pl = power_min;
	}
	else {
		err_integral = i;
	}

	/*
	 * No descriptive comment, as this log is useful, mainly,
	 * for computer-aided analysis of the controller.
	 */
	slogd("%u: %u %u %u %g %g", ++count, _temp_limit, _temp, (unsigned int)pl, p, i);

	return (unsigned int)pl;
}


int main(int argc, char *argv[])
{
	unsigned int temp;
	unsigned int power_limit;
	unsigned int power_limit_prev;

	gpu_options_process_or_abort(&options, argc, argv);
	initialize();
	atexit(cleanup);

	signal(SIGTERM, SIGTERM_handler);
	signal(SIGINT, SIGTERM_handler);	/* Ctrl-C */
	signal(SIGALRM, SIGALRM_handler);

	power_limit_prev = gpu_power.powerLimitActual;

	/* override pre-set power limit */
	if (options.power_limit_set) {
		gpu_power.powerLimitActual = clamp(options.power_limit,
						   gpu_power.powerLimitMin, gpu_power.powerLimitMax);
	}

	while ( 1 ) {
		alarm(TEMP_CTRL_DELTAT);

		nvmlh->nvmlDeviceGetTemperature(nvmlh->device,
						NVML_TEMPERATURE_GPU/*no other options*/,
						&temp);

		power_limit = pid_control(temp, options.temp_limit);
		if (power_limit != power_limit_prev) {
			power_limit_prev = power_limit;
			nvmlh->nvmlDeviceSetPowerManagementLimit(nvmlh->device, power_limit);
		}

		pause();
	}

	return 1;
}

