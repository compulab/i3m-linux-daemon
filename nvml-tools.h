/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _NVML_TOOLS_H
#define _NVML_TOOLS_H

#include <nvml.h>


struct NvmlHandle {
	/* reference to the DLL */
	void *dll;

	/* GPU device handle [0] */
	nvmlDevice_t device;

	/* reference to DLL symbols */
	nvmlReturn_t (*nvmlInit)(void);
	nvmlReturn_t (*nvmlShutdown)(void);
	nvmlReturn_t (*nvmlSystemGetDriverVersion)(char *, unsigned int);
	nvmlReturn_t (*nvmlSystemGetNVMLVersion)(char *, unsigned int);
	nvmlReturn_t (*nvmlDeviceGetCount)(unsigned int *);
	nvmlReturn_t (*nvmlDeviceGetHandleByIndex)(unsigned int, nvmlDevice_t *);
	nvmlReturn_t (*nvmlDeviceGetName)(nvmlDevice_t, char *, unsigned int);
	nvmlReturn_t (*nvmlDeviceGetTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int *);
	nvmlReturn_t (*nvmlDeviceGetPowerManagementMode)(nvmlDevice_t, nvmlEnableState_t *);
	nvmlReturn_t (*nvmlDeviceGetPowerManagementLimit)(nvmlDevice_t, unsigned int *);
	nvmlReturn_t (*nvmlDeviceGetPowerManagementLimitConstraints)(nvmlDevice_t, unsigned int *, unsigned int *);
	nvmlReturn_t (*nvmlDeviceGetPowerManagementDefaultLimit)(nvmlDevice_t, unsigned int *);
	nvmlReturn_t (*nvmlDeviceSetPersistenceMode)(nvmlDevice_t, nvmlEnableState_t);
	nvmlReturn_t (*nvmlDeviceSetPowerManagementLimit)(nvmlDevice_t, unsigned int);
};

typedef struct NvmlHandle NvmlHandle;

NvmlHandle *nvml_init(void);
void nvml_cleanup(int status, void *_dllh);

#endif	/* _NVML_TOOLS_H */

