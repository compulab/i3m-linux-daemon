/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <nvml.h>


#define NVML_DEVICE_DEFAULT_INDEX	0


static bool nvml_is_initialized = false;


static void nvml_cleanup(void)
{
	nvmlShutdown();
}

static int nvml_init(void)
{
	nvmlReturn_t err;
	unsigned int count;

	err = nvmlInit();
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not initialize NVML: %d \n", err);
		goto nvml_out_err0;
	}

	err = nvmlDeviceGetCount(&count);
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not get device count: %d \n", err);
		goto nvml_out_err1;
	}

	if (count == 0) {
		fprintf(stderr, "nvml: no GPU card present \n");
		err = NVML_ERROR_NOT_FOUND;
		goto nvml_out_err1;
	}
	else if (count > 1) {
		fprintf(stderr, "nvml: %d GPU cards present. However, only one will be monitored. \n", count);
	}

	atexit(nvml_cleanup);
	return 0;

nvml_out_err1:
	nvmlShutdown();

nvml_out_err0:
	return err;
}

int nvml_gpu_temp_read(unsigned int *temp)
{
	nvmlReturn_t err;
	nvmlDevice_t device;

	if ( !nvml_is_initialized ) {
		err = nvml_init();
		if (err != NVML_SUCCESS)
			return err;
	}

	nvml_is_initialized = true;
	err = nvmlDeviceGetHandleByIndex(NVML_DEVICE_DEFAULT_INDEX, &device);
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not get device handle: %d \n", err);
		return err;
	}

	err = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU/*no other options*/, temp);
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not get device temperature: %d \n", err);
	}

	return err;
}

