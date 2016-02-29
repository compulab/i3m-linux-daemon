/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "common.h"
#include "nvml-tools.h"



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
	load_symbol(dllh, nvmlSystemGetDriverVersion, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlSystemGetNVMLVersion, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetCount, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetHandleByIndex, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetName, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetTemperature, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetPowerManagementMode, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetPowerManagementLimit, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetPowerManagementLimitConstraints, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceGetPowerManagementDefaultLimit, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceSetPersistenceMode, dlerrstr, dll_out_err1);
	load_symbol(dllh, nvmlDeviceSetPowerManagementLimit, dlerrstr, dll_out_err1);

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


void nvml_cleanup(int status, void *_dllh)
{
	NvmlHandle *dllh = (NvmlHandle *)_dllh;

	dllh->nvmlShutdown();
	nvml_dll_cleanup(dllh);
}

NvmlHandle *nvml_init(void)
{
	nvmlReturn_t err;
	unsigned int count;
	NvmlHandle *dllh;

	dllh = nvml_dll_load();
	if (dllh == NULL)
		goto nvml_out_err0;

	err = dllh->nvmlInit();
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not initialize NVML: %d", err);
		goto nvml_out_err1;
	}

	err = dllh->nvmlDeviceGetCount(&count);
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not get device count: %d", err);
		goto nvml_out_err2;
	}

	if (count == 0) {
		sloge("nvml: no GPU card present");
		err = NVML_ERROR_NOT_FOUND;
		goto nvml_out_err2;
	}
	else if (count > 1) {
		slogw("nvml: %d GPU cards present, however, only one will be monitored", count);
	}

	err = dllh->nvmlDeviceGetHandleByIndex(NVML_DEVICE_DEFAULT_INDEX, &dllh->device);
	if (err != NVML_SUCCESS) {
		sloge("nvml: could not get device handle: %d", err);
		goto nvml_out_err2;
	}

	return dllh;

nvml_out_err2:
	dllh->nvmlShutdown();

nvml_out_err1:
	nvml_dll_cleanup(dllh);

nvml_out_err0:
	return NULL;
}

