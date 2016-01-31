/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <stdbool.h>


typedef struct {
	bool help;
	bool info;
	int i2c_bus;
	int poll_cycle;
	int loglevel;
	char configfile[128];
	long disable;

	/* _private_ */
	bool i2c_bus_set;
	bool poll_cycle_set;
	bool loglevel_set;
} Options;


void options_process_or_abort(Options *opts, int argc, char *argv[]);
void show_options(Options *opts);

#endif	/* _OPTIONS_H */

