/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"


#define PATH_PROC_CPUINFO		"/proc/cpuinfo"

#define starts_with(substr, str)	((str != NULL) && !strncmp((substr), (str), strlen(substr)))

enum {
	CPUINFO_INIT,
	CPUINFO_PROCESSOR_BLOCK,
};


/**
 * Fill list of CPU core frequencies (in MHz). 
 * On a multithreaded core, the 1-st discovered thread sets the core frequency. 
 * Arguments:   								      .
 * cpuinfo - FILE struct pointing to an open /proc/cpuinfo file 									      .
 * num_cores - initially, the maximum available freq_table size,        															      .
 *             upon return, the number of freq_table entries that have been filled      																					.
 * freq_table - the table (list) of CPU core frequencies 																														.
 */
static void parse_cpuinfo(FILE *cpuinfo, int *num_cores, int *freq_table)
{
	char buff[128];
	char *line;
	int state;
	int core_id;
	float cpu_freq;
	int real_num_cores;

	memset(freq_table, 0, *num_cores);
	real_num_cores = 0;
	state = CPUINFO_INIT;
	while ( !feof(cpuinfo) ) {
		line = fgets(buff, sizeof(buff), cpuinfo);
		switch (state) {
		case CPUINFO_INIT:
			if (starts_with("processor", line))
				state = CPUINFO_PROCESSOR_BLOCK;
			break;

		case CPUINFO_PROCESSOR_BLOCK:
			if (starts_with("core id", line)) {
				sscanf(line, "core id : %d", &core_id);
				break;
			}

			if (starts_with("cpu MHz", line)) {
				sscanf(line, "cpu MHz : %f", &cpu_freq);
				break;
			}

			/*
			 * Processor block end is identified by either a
			 * new block beginning or end of file.
			 */
			if (starts_with("processor", line) || feof(cpuinfo)) {
				if ((core_id < *num_cores) && (freq_table[core_id] == 0)) {
					freq_table[core_id] = (int)roundf(cpu_freq);
					++real_num_cores;
				}
				break;
			}

			break;
		}
	}

	*num_cores = real_num_cores;
}

void cpu_freq_get_list(int *num_cores, int *freq_list)
{
	FILE *cpuinfo;

	cpuinfo = fopen(PATH_PROC_CPUINFO, "r");
	if ( !cpuinfo ) {
		sloge("CPU Freq: could not open %s", PATH_PROC_CPUINFO);
		return;
	}

	parse_cpuinfo(cpuinfo, num_cores, freq_list);
	fclose(cpuinfo);
}

