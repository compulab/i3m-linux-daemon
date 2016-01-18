/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sensors/sensors.h>

#include "common.h"
#include "thread-pool.h"
#include "registers.h"
#include "panel.h"
#include "sensors.h"
#include "domain-logic.h"
#include "stats.h"
#include "pci-tools.h"

struct cmdline_opt {
	bool display_help;
	bool list_temp_sensors;
	bool list_video_driver;
	int i2c_bus;
	int loglevel;
};

ThreadPool *frontend_thread;
ThreadPool *backend_thread;

static struct cmdline_opt options;

static void main_thread(void *priv_context, void *shared_context);


static void signal_handler(int signo)
{
	switch (signo)
	{
	case SIGALRM:
		thread_pool_add_request(frontend_thread, main_thread, NULL);
		break;

	case SIGUSR1:
		stat_show();
		break;

	case SIGUSR2:
		/* nothing - interrupt sleep */
		break;
	}
}

static void install_sighandler(int signo)
{
	struct sigaction sig = {{0}};
	int err;

	sig.sa_handler = signal_handler;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = SA_RESTART;

	err = sigaction(signo, &sig, NULL);
	if (err) {
		fprintf(stderr, "Signal %d: could not install signal handler: %d \n", signo, err);
		exit(EXIT_FAILURE);
	}
}

enum {DTERM_INIT, DTERM_WAIT};

static void daemon_termination(int phase)
{
	sigset_t signals;
	int signo = 0;

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);

	switch (phase) {
	case DTERM_INIT:
		pthread_sigmask(SIG_BLOCK, &signals, NULL);
		break;
	case DTERM_WAIT:
		while (signo != SIGTERM)
			sigwait(&signals, &signo);
		break;
	}
}

static void daemonize(void)
{
	int fd;
	int err;
	const int loglevels[] = {LOG_DEBUG, LOG_INFO, LOG_NOTICE};

	/*
	 * assert a single instance of the daemon
	 */
	fd = open(ATFP_DAEMON_LOCKFILE, O_CREAT, 0);
	if (fd < 0) {
		fprintf(stderr, "Could not open lock file: %d. \n", errno);
		exit(1);
	}
	err = flock(fd, (LOCK_EX | LOCK_NB));
	if (err < 0) {
		if (errno == EWOULDBLOCK)
			fprintf(stderr, "An instance of the daemon already runs. \n");
		else
			fprintf(stderr, "Could not acquire lock file: %d. \n", errno);
		exit(1);
	}

	/*
	 * fork background daemon
	 */
	daemon(0, 0);
	umask(0022);

	/*
	 * set up logging
	 */
	openlog(ATFP_SYSLOG_IDENT, LOG_PID, LOG_USER);
	atexit(closelog);
	setlogmask(LOG_UPTO( loglevels[options.loglevel] ));
	syslog(LOG_NOTICE, "AirTop Front-Panel Service -- start");
}

static void initialize(void)
{
	int err;
	int panel;

	install_sighandler(SIGALRM);
	install_sighandler(SIGUSR1);
	install_sighandler(SIGUSR2);
	daemon_termination(DTERM_INIT);
	panel = panel_open_i2c_device(options.i2c_bus, I2C_PANEL_INTERFACE_ADDR);
	if (panel < 0)
		exit(1);
	err = panel_reset();
	if ( err )
		exit(1);

	err = sensors_coretemp_init();
	if ( err )
		exit(1);

	sensors_nouveau_init();

	frontend_thread = thread_pool_create(1, ATFP_FRONTEND_QUEUE_LEN, NULL);
	backend_thread = thread_pool_create(ATFP_BACKEND_THREAD_NUM, ATFP_BACKEND_QUEUE_LEN, NULL);
}

static void cleanup(void)
{
	thread_pool_join(frontend_thread);
	thread_pool_destroy(backend_thread);
	thread_pool_destroy(frontend_thread);

	panel_close();
	sensors_cleanup();
}

#define UNSUPPORTED_REQ_MESSAGE(r)	do { fprintf(stderr, "%s: "#r" request is not supported \n", __FUNCTION__); } while (0)

static void main_thread(void *priv_context, void *shared_context)
{
	int i;
	long request_bitmap;
	long request;

	request_bitmap = panel_get_pending_requests();

	/* dispatch each request */
	for (i = 0; i < (sizeof(request_bitmap) * 8); ++i) {
		request = request_bitmap & (1L << i);
		switch (request) {
		case 0:
			/* no pending requests */
			break;

		case ATFP_MASK_PENDR0_HDDTR:
			UNSUPPORTED_REQ_MESSAGE(HDDTR);
			break;

		case ATFP_MASK_PENDR0_CPUFR:
			panel_update_frequency();
			break;

		case ATFP_MASK_PENDR0_CPUTR:
			panel_update_temperature();
			break;

		case ATFP_MASK_PENDR0_GPUTR:
			panel_update_gpu_temp();
			break;

		default:
			break;
		}
	}

	/* program our next appearance */
	alarm(ATFP_MAIN_POLL_CYCLE);
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
		{"help",		no_argument,		0,	'h'},
		{"list-temp-sensors",	no_argument,		0,	't'},
		{"list-video-driver",	no_argument,		0,	'v'},
		{"i2c-bus",		required_argument,	0,	'b'},
		{"loglevel",      	required_argument,	0,	'l'},
		{0,			0,			0,	0}
	};

	int c;
	int option_index = 0;
	const char *loglevels[] = {"debug", "info", "notice", NULL};
	int i;

	/* initialize default values */
	opts->display_help = false;
	opts->list_temp_sensors = false;
	opts->list_video_driver = false;
	opts->i2c_bus = -1;
	opts->loglevel = 2;	/* 'notice' */

	/* parse command line options */
	while (true) {
		c = getopt_long_only(argc, argv, "", long_options, &option_index);

		switch (c) {
		case -1:
			/* no more valid options */
			/* test nothing has left in argv[] */
			if (argv[optind] == NULL)
				goto getopt_out_test_validity;
			/* fall through */

		case '?':
			/* invalid option */
			goto getopt_out_err;

		/* valid options */
		case 'h':
			opts->display_help = true;
			break;
		case 't':
			opts->list_temp_sensors = true;
			break;
		case 'v':
			opts->list_video_driver = true;
			break;
		case 'b':
			opts->i2c_bus = strtol(optarg, NULL, 0);
			break;
		case 'l':
			i = 0;
			while (loglevels[i] != NULL) {
				if ( !strcmp(loglevels[i], optarg) ) {
					opts->loglevel = i;
					break;
				}
				++i;
			}
			break;
		}
	}

getopt_out_test_validity:
	/* a special case */
	if (opts->display_help)
		goto getopt_out_err;

	/* test options validity */
	if (opts->list_temp_sensors ||
	    opts->list_video_driver)
		goto getopt_out_ok;

	if (opts->i2c_bus == -1)
		opts->i2c_bus = panel_lookup_i2c_bus();

	if (opts->i2c_bus == -1)
		goto getopt_out_err;

	/* success */
getopt_out_ok:
	return 0;

getopt_out_err:
	/* print usage */
	fprintf(stderr, "Usage:\n" \
			"%s --help\n" \
			"%s --list-temp-sensors\n" \
			"%s --list-video-driver\n" \
			"%s [--i2c-bus=N] [--loglevel=<notice|info|debug>]\n" \
			"Where:\n" \
			"--i2c-bus  is i2c bus number of the front panel\n",
			basename(argv[0]),
			basename(argv[0]),
			basename(argv[0]),
			basename(argv[0]));
	return -1;
}

int main(int argc, char *argv[])
{
	int err;

	err = fpsvc_parse_cmdline_opts(argc, argv, &options);
	if (err < 0)
		return err;

	if (options.list_temp_sensors)
		sensors_show(SENSORS_FEATURE_TEMP);

	if (options.list_video_driver)
		printf("Video driver: %s \n", get_vga_driver_name());

	if (options.list_temp_sensors ||
	    options.list_video_driver) {
		return 0;
	}

	daemonize();
	initialize();
	/*
	 * Main FP communication routine is armed
	 * upon SIGALRM reception.
	 */
	alarm(ATFP_MAIN_STARTUP_DELAY);
	daemon_termination(DTERM_WAIT);
	printf("Daemon exit. \n");

	cleanup();
	return 0;
}

