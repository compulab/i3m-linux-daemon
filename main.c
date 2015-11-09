/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

#include "panel.h"
#include "sensors.h"

struct cmdline_opt {
	int i2c_bus;
};

static bool daemon_terminate = false;
static struct cmdline_opt options;
static int panel_desc;

static void signal_handler(int signo)
{
	switch (signo)
	{
	case SIGTERM:
		daemon_terminate = true;
		break;
	}
}

static void install_sighandler(void)
{
	struct sigaction sig = {{0}};
	int err;

	sig.sa_handler = signal_handler;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = SA_RESTART;

	err = sigaction(SIGTERM, &sig, NULL);
	if (err) {
		fprintf(stderr, "SIGTERM: could not install signal handler: %d \n", err);
		exit(EXIT_FAILURE);
	}
}

static void initialize(void)
{
	if (fork() != 0)
		exit(0);

	setsid();
	install_sighandler();
	panel_desc = panel_open_i2c_device(options.i2c_bus, I2C_PANEL_INTERFACE_ADDR);
	if (panel_desc < 0)
		exit(1);
}

static void cleanup(void)
{
	close(panel_desc);
}

static int main_loop(void)
{
	int count = 0;

	while ( !daemon_terminate ) {
		/* test */
		panel_set_temperature(panel_desc, 0/*CPU ID*/, (5 + count)/*temp.*/);
		count = (count + 1) % 21;

		printf(".");
		fflush(stdout);
		sleep(1);
	}

	return 0;
}

/*
 * Parse command line options:
 * phase I:   set default options values
 * phase II:  set options according to the command line
 * phase III: test no options are missing, and they are correct
 */
static int fpsvc_parse_cmdline_opts(int argc, char *argv[], struct cmdline_opt *opts)
{
	const struct option long_options[] = {
		{"i2c-bus",	required_argument,	0,	'b'},
		{0,			0,					0,	0}
	};

	int c;
	int option_index = 0;

	/* initialize default values */
	opts->i2c_bus = -1;

	/* parse command line options */
	while (true) {
		c = getopt_long_only(argc, argv, "", long_options, &option_index);

		switch (c) {
		case -1:
			/* no more valid options */
			/* test nothing has left in argv[] */
			if (argv[optind] == NULL)
				goto getopt_out_ok;
			/* fall through */

		case '?':
			/* invalid option */
			goto getopt_out_err;

		/* valid options */
		case 'b':
			opts->i2c_bus = strtol(optarg, NULL, 0);
			break;
		}
	}

getopt_out_ok:
	/* test options validity */
	if (opts->i2c_bus == -1)
		goto getopt_out_err;

	/* success */
	return 0;

getopt_out_err:
	/* print usage */
	fprintf(stderr, "Usage:\n" \
					"%s --i2c-bus=N\n" \
					"Where:\n" \
					"--i2c-bus  is i2c bus number of the front panel\n",
					basename(argv[0]));
	return -1;
}

int main(int argc, char *argv[])
{
	int ret;

	ret = fpsvc_parse_cmdline_opts(argc, argv, &options);
	if (ret < 0)
		return ret;

	initialize();
	ret = main_loop();
	cleanup();

	return ret;
}

