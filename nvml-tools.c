/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <nvml.h>

#include "common.h"



/*
 * Dynamically load nvml library.
 * As nvml lib is distributed along with proprietary nvidia graphics driver,
 * we must be able to tolerate its absence on systems employing other drivers.
 * This is achieved by taking control over nvml library loading process.
 */

/* NVML DLL file name */
#define NVML_DLL_FILE			"libnvidia-ml.so"

/* load a function symbol from the DLL */
#define load_symbol(dllh, symbol, errstr, gotoerr)	do {	\
		dllh->symbol = dlsym(dllh->dll, #symbol);	\
		errstr = dlerror();				\
		if (errstr != NULL)				\
			goto gotoerr;				\
	} while (0)

typedef struct {
	/* reference to the DLL */
	void *dll;

	/* GPU device handle [0] */
	nvmlDevice_t device;

	/* reference to DLL symbols */
	nvmlReturn_t (*nvmlInit)(void);
	nvmlReturn_t (*nvmlShutdown)(void);
	nvmlReturn_t (*nvmlDeviceGetCount)(unsigned int *);
	nvmlReturn_t (*nvmlDeviceGetHandleByIndex)(unsigned int, nvmlDevice_t *);
	nvmlReturn_t (*nvmlDeviceGetTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int *);
} NvmlHandle;

static NvmlHandle *nvml_dll_load(void)
{
	void *dll;
	NvmlHandle *dllh;
	char *dlerrstr;

	dll = dlopen(NVML_DLL_FILE, RTLD_NOW);
	if ( !dll ) {
		dlerrstr = dlerror();
		goto dll_out_err0;
	}

	dllh = (NvmlHandle *)calloc(1, sizeof(NvmlHandle));
	dllh->dll = dll;

	load_symbol(dllh, nvmlInit, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlShutdown, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetCount, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetHandleByIndex, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetTemperature, dlerrstr, dll_out_err1);

	return dllh;

dll_out_err1:
	dlclose(dll);
	free(dllh);

dll_out_err0:
	sloge("%s", dlerrstr);
	return NULL;
}

static void nvml_dll_cleanup(NvmlHandle *dllh)
{
	dlclose(dllh->dll);
	free(dllh);
}



/*
 * NVML library tools.
 * Reference:
 * NVML API Reference Manual (https://developer.nvidia.com/nvidia-management-library-nvml)
 */

#define NVML_DEVICE_DEFAULT_INDEX	0


static NvmlHandle *nvml_hdl = NULL;


static void nvml_cleanup(void)
{
	nvml_hdl->nvmlShutdown();
	nvml_dll_cleanup(nvml_hdl);
	nvml_hdl = NULL;
}

int nvml_init(void)
{
	nvmlReturn_t err;
	unsigned int count;

	nvml_hdl = nvml_dll_load();
	if (nvml_hdl == NULL)
		return NVML_ERROR_LIBRARY_NOT_FOUND;

	err = nvml_hdl->nvmlInit();
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not initialize NVML: %d", err);
		goto nvml_out_err0;
	}

	err = nvml_hdl->nvmlDeviceGetCount(&count);
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not get device count: %d", err);
		goto nvml_out_err1;
	}

	if (count == 0) {
		sloge("nvml: no GPU card present");
		err = NVML_ERROR_NOT_FOUND;
		goto nvml_out_err1;
	}
	else if (count > 1) {
		slogw("nvml: %d GPU cards present, however, only one will be monitored", count);
	}

	err = nvml_hdl->nvmlDeviceGetHandleByIndex(NVML_DEVICE_DEFAULT_INDEX, &nvml_hdl->device);
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not get device handle: %d", err);
		goto nvml_out_err1;
	}

	atexit(nvml_cleanup);
	return 0;

nvml_out_err1:
	nvml_hdl->nvmlShutdown();

nvml_out_err0:
	return err;
}

int nvml_gpu_temp_read(unsigned int *temp)
{
	nvmlReturn_t err;

	err = nvml_hdl->nvmlDeviceGetTemperature(nvml_hdl->device, NVML_TEMPERATURE_GPU/*no other options*/, temp);
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not get device temperature: %d", err);
	}

	return err;
}

