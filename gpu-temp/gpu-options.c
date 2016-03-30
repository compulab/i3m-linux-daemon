/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 *
 * Command line and configuration file options processing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <syslog.h>

#include "gpu-thermald.h"
#include "gpu-options.h"


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

static int options_parse_cmdline(GPUOptions *opts, int argc, char *argv[])
{
	int i;
	int flag;
	unsigned int uint_value;
	char str_value[128];

	for (i = 1; i < argc; ++i) {
		if ( !strcmp("--help", argv[i]) ) {
			opts->help = true;
			continue;
		}

		flag = sscanf(argv[i], "--temp-limit=%d", &uint_value);
		if (flag > 0) {
			opts->temp_limit = uint_value;
			opts->temp_limit_set = true;
			continue;
		}

		flag = sscanf(argv[i], "--power-limit=%d", &uint_value);
		if (flag > 0) {
			opts->power_limit = uint_value;
			opts->power_limit_set = true;
			continue;
		}

		flag = sscanf(argv[i], "--loglevel=%7s", str_value);
		if (flag > 0) {
			opts->loglevel_set = loglevel_conv_string_to_int(str_value, &opts->loglevel);
			if (opts->loglevel_set)
				continue;
		}

		flag = sscanf(argv[i], "--configfile=%127s", str_value);
		if (flag > 0) {
			strcpy(opts->configfile, str_value);
			continue;
		}

		/* something that does not match any pattern */
		return -EINVAL;
	}

	return 0;
}

static bool is_nodata_line(const char *line)
{
	int i;

	if (line == NULL)
		return true;

	for (i = 0; line[i] != '\0'; ++i) {
		if ( !isspace(line[i]) ) {
			if (line[i] == '#')
				break;

			return false;
		}
	}

	return true;
}

static int options_parse_configfile(GPUOptions *opts, const char *filename)
{
	FILE *config;
	char *line;
	char buff[128];
	int flag;
	unsigned int uint_value;
	char str_value[128];

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
		if (is_nodata_line(line)) {
			continue;
		}

		flag = sscanf(line, "temp-limit=%d", &uint_value);
		if ((flag > 0) && !opts->temp_limit_set) {
			opts->temp_limit = uint_value;
			opts->temp_limit_set = true;
			continue;
		}

		flag = sscanf(line, "power-limit=%d", &uint_value);
		if ((flag > 0) && !opts->power_limit_set) {
			opts->power_limit = uint_value;
			opts->power_limit_set = true;
			continue;
		}

		flag = sscanf(line, "loglevel=%7s", str_value);
		if ((flag > 0) && !opts->loglevel_set) {
			opts->loglevel_set = loglevel_conv_string_to_int(str_value, &opts->loglevel);
			if (opts->loglevel_set)
				continue;
		}

		goto configfile_out_err;
	}

	return 0;

configfile_out_err:
	fprintf(stderr, "Configfile: %s: invalid option: %s \n", opts->configfile, line);
	return -EINVAL;
}

static void print_help_message_and_exit(const char *name)
{
	fprintf(stderr, "Usage: %s [OPTION] [OPTION] ... \n", name);

	fprintf(stderr, "\nCompuLab GPU thermal daemon. \n");
	fprintf(stderr, "Keep GPU temperature below preset value. \n");

	fprintf(stderr, "\nCommand line options: \n");
	fprintf(stderr, "  --temp-limit=T     GPU temperature limit in degC \n");
	fprintf(stderr, "  --power-limit=P    GPU power limit in milliW \n");
	fprintf(stderr, "  --loglevel=LEVEL   print to system log messages up to LEVEL. LEVEL may be either [notice], info, debug \n");
	fprintf(stderr, "  --configfile=PATH  path to (optional) configuration file. By default '%s' will be used. \n", GPUD_CONFIGFILE);
	fprintf(stderr, "  --help             display this help and exit \n");

	fprintf(stderr, "\nConfiguration file options: \n");
	fprintf(stderr, "  temp-limit=T \n");
	fprintf(stderr, "  power-limit=P \n");
	fprintf(stderr, "  loglevel=LEVEL \n");

	fprintf(stderr, "\nNotice: \n");
	fprintf(stderr, "  Command line options take precedence over configuration file options. \n");

	exit(1);
}

static void options_load_defaults(GPUOptions *opts)
{
	memset(opts, 0, sizeof(GPUOptions));

	/* non-zero default values */
	opts->temp_limit = GPUD_DEFAULT_TEMP;
	opts->loglevel = LOG_NOTICE;
	strcpy(opts->configfile, GPUD_CONFIGFILE);
}

void gpu_options_process_or_abort(GPUOptions *opts, int argc, char *argv[])
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

