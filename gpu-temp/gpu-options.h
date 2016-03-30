/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _GPU_OPTIONS_H
#define _GPU_OPTIONS_H

#include <stdbool.h>


typedef struct {
	bool help;
	unsigned int temp_limit;
	unsigned int power_limit;
	int loglevel;
	char configfile[128];

	/* _private_ */
	bool temp_limit_set;
	bool power_limit_set;
	bool loglevel_set;
} GPUOptions;


void gpu_options_process_or_abort(GPUOptions *opts, int argc, char *argv[]);

#endif	/* _GPU_OPTIONS_H */

