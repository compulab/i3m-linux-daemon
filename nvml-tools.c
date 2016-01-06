/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <nvml.h>



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
		fprintf(stderr, "%s \n", dlerror());
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
	fprintf(stderr, "%s \n", dlerrstr);
	free(dllh);
	dlclose(dll);

dll_out_err0:
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


static bool nvml_is_initialized = false;
static NvmlHandle *nvml_hdl = NULL;


static void nvml_cleanup(void)
{
	nvml_hdl->nvmlShutdown();
	nvml_dll_cleanup(nvml_hdl);
	nvml_hdl = NULL;
	nvml_is_initialized = false;
}

static int nvml_init(void)
{
	nvmlReturn_t err;
	unsigned int count;

	nvml_hdl = nvml_dll_load();
	if (nvml_hdl == NULL)
		return NVML_ERROR_LIBRARY_NOT_FOUND;

	err = nvml_hdl->nvmlInit();
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not initialize NVML: %d \n", err);
		goto nvml_out_err0;
	}

	err = nvml_hdl->nvmlDeviceGetCount(&count);
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
	nvml_hdl->nvmlShutdown();

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
	err = nvml_hdl->nvmlDeviceGetHandleByIndex(NVML_DEVICE_DEFAULT_INDEX, &device);
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not get device handle: %d \n", err);
		return err;
	}

	err = nvml_hdl->nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU/*no other options*/, temp);
	if (err != NVML_SUCCESS) {
		fprintf(stderr, "nvml: could not get device temperature: %d \n", err);
	}

	return err;
}

