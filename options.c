/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */
/*
 * Command line and configuration file options processing.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <errno.h>
#include <syslog.h>

#include "options.h"
#include "common.h"
#include "registers.h"


#define starts_with(substr, str, idx)	((str != NULL) && (((idx) = strlen(substr)), !strncmp((substr), (str), (idx))))
#define mask_if_match(r, p)		((long)(!strncmp(#r, p, strlen(#r))) * ATFP_MASK_PENDR0_##r)


/*
 * Syslog loglevel: convert string representation to int.
 * Return:
 * true or false in case match could not be done
 */
static bool loglevel_conv_string_to_int(const char *s_loglevel, int *i_loglevel)
{
	int idx;
	const char *s_loglevels[] = {"notice", "info", "debug", NULL};
	const int i_loglevels[] = {LOG_NOTICE, LOG_INFO, LOG_DEBUG};

	idx = 0;
	while (s_loglevels[idx] != NULL) {
		if ( !strncmp(s_loglevels[idx], s_loglevel, strlen(s_loglevels[idx])) ) {
			*i_loglevel = i_loglevels[idx];
			return true;
		}

		++idx;
	}

	return false;
}

static int options_parse_cmdline(Options *opts, int argc, char *argv[])
{
	const struct option long_options[] = {
		{"help",		no_argument,		0,	'h'},
		{"info",		no_argument,		0,	'i'},
		{"i2c-bus",		required_argument,	0,	'b'},
		{"poll-cycle",		required_argument,	0,	'p'},
		{"loglevel",      	required_argument,	0,	'l'},
		{"configfile",          required_argument,	0,	'f'},
		{0,			0,			0,	0}
	};

	int option_index = 0;
	int c;

	while (true) {
		c = getopt_long_only(argc, argv, "", long_options, &option_index);

		switch (c) {
		case -1:
			/* no more valid options */
			/* test nothing has left in argv[] */
			if (argv[optind] == NULL)
				return 0;
			/* fall through */

		case '?':
			/* invalid option */
			return -EINVAL;

		/* valid options */
		case 'h':
			opts->help = true;
			break;
		case 'i':
			opts->info = true;
			break;
		case 'b':
			opts->i2c_bus = strtol(optarg, NULL, 0);
			opts->i2c_bus_set = true;
			break;
		case 'p':
			opts->poll_cycle = strtol(optarg, NULL, 0);
			opts->poll_cycle_set = true;
			break;
		case 'l':
			opts->loglevel_set = loglevel_conv_string_to_int(optarg, &opts->loglevel);
			break;
		case 'f':
			strncpy(opts->configfile, optarg, sizeof(opts->configfile));
			break;
		}
	}
}

static int options_parse_configfile(Options *opts, const char *filename)
{
	FILE *config;
	char *line;
	char buff[128];
	int k;

	/* config. file is optional - tolerate its absence */
	if (filename == NULL)
		return 0;

	config = fopen(filename, "r");
	if (config == NULL) {
		if (errno == ENOENT)
			return 0;

		/* %m below prints strerror(errno) - no additional argument is required */
		fprintf(stderr, "Configfile: %s: %m \n", opts->configfile);
		return -errno;
	}

	while ( !feof(config) ) {
		line = fgets(buff, sizeof(buff), config);
		if ((line == NULL) || (strlen(line) == 0)) {
			continue;
		}
		else if (starts_with("#", line, k)) {
			continue;
		}
		else if (starts_with("i2c-bus=", line, k)) {
			if (!opts->i2c_bus_set) {
				opts->i2c_bus = strtol(&line[k], NULL, 0);
				opts->i2c_bus_set = true;
			}
		}
		else if (starts_with("poll-cycle=", line, k)) {
			if (!opts->poll_cycle_set) {
				opts->poll_cycle = strtol(&line[k], NULL, 0);
				opts->poll_cycle_set = true;
			}
		}
		else if (starts_with("loglevel=", line, k)) {
			if (!opts->loglevel_set) {
				opts->loglevel_set = loglevel_conv_string_to_int(&line[k], &opts->loglevel);
			}
		}
		else if (starts_with("disable=", line, k)) {
			char *ptr = strtok(&line[k], ",");
			while (ptr != NULL) {
				opts->disable |= mask_if_match(HDDTR, ptr);
				opts->disable |= mask_if_match(CPUFR, ptr);
				opts->disable |= mask_if_match(CPUTR, ptr);
				opts->disable |= mask_if_match(GPUTR, ptr);

				ptr = strtok(NULL, ",");
			}
		}
		else {
			goto configfile_out_err;
		}
	}
	return 0;

configfile_out_err:
	fprintf(stderr, "Configfile: %s: invalid option: %s \n", opts->configfile, line);
	return -EINVAL;
}

static void print_help_message_and_exit(const char *name)
{
	fprintf(stderr, "Usage: %s [OPTION] [OPTION] ... \n", name);

	fprintf(stderr, "\nCompuLab AirTop Front Panel service daemon. \n");
	fprintf(stderr, "Provide hardware-related metrics to the front panel display controller. \n");

	fprintf(stderr, "\nCommand line options: \n");
	fprintf(stderr, "  --i2c-bus=N        front panel controller I2C bus. By default, FP I2C bus will be discovered automatically. \n");
	fprintf(stderr, "  --poll-cycle=T     number of seconds to poll for front panel request. \n");
	fprintf(stderr, "  --loglevel=LEVEL   print to system log messages up to LEVEL. LEVEL may be either [notice], info, debug \n");
	fprintf(stderr, "  --configfile=PATH  path to (optional) configuration file \n");
	fprintf(stderr, "  --info             display brief system information relevant for this daemon and exit \n");
	fprintf(stderr, "  --help             display this help and exit \n");

	fprintf(stderr, "\nConfiguration file options: \n");
	fprintf(stderr, "  i2c-bus=N \n");
	fprintf(stderr, "  poll-cycle=T \n");
	fprintf(stderr, "  loglevel=LEVEL \n");
	fprintf(stderr, "  disable=FUNC1[,FUNC2[,...]]  disable particular functionality, that may be requested by the FP controller. FUNC may be: \n");
	fprintf(stderr, "                               HDDTR  HDD temperature \n");
	fprintf(stderr, "                               CPUFR  CPU frequency \n");
	fprintf(stderr, "                               CPUTR  CPU temperature \n");
	fprintf(stderr, "                               GPUTR  GPU temperature \n");
	fprintf(stderr, "Command line options take precedence over configuration file options. \n");

	exit(1);
}

static void options_load_defaults(Options *opts)
{
	memset(opts, 0, sizeof(Options));

	/* non-zero default values */
	opts->poll_cycle = ATFP_MAIN_POLL_CYCLE;
	opts->loglevel = LOG_NOTICE;
	strcpy(opts->configfile, ATFP_DAEMON_CONFIGFILE);
}

void options_process_or_abort(Options *opts, int argc, char *argv[])
{
	int err;

	options_load_defaults(opts);

	err = options_parse_cmdline(opts, argc, argv);
	if ((err < 0) || (opts->help))
		print_help_message_and_exit(basename(argv[0]));

	err = options_parse_configfile(opts, opts->configfile);
	if (err < 0)
		exit(1);
}

/* for testing purposes */
void show_options(Options *opts)
{
	printf("help        : %c \n", opts->help ? '+' : '-');
	printf("info        : %c \n", opts->info ? '+' : '-');
	printf("i2c-bus     : %d [%c] \n", opts->i2c_bus, opts->i2c_bus_set ? '+' : '-');
	printf("poll-cycle  : %d [%c] \n", opts->poll_cycle, opts->poll_cycle_set ? '+' : '-');
	printf("loglevel    : %d [%c] \n", opts->loglevel, opts->loglevel_set ? '+' : '-');
	printf("configfile  : %s \n", opts->configfile);
	printf("disable     : 0x%016lx \n", opts->disable);
}

