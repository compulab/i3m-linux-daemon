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
#include "options.h"


ThreadPool *frontend_thread;
ThreadPool *backend_thread;

static Options options;

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
		sloge("Signal %d: could not install handler: %d", signo, err);
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
	setlogmask(LOG_UPTO(options.loglevel));
	slogn("AirTop Front-Panel Service -- start");
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

#define UNSUPPORTED_REQ_MESSAGE(r)	do { slogw("%s: "#r" request is not supported", __FUNCTION__); } while (0)

static void main_thread(void *priv_context, void *shared_context)
{
	int i;
	long request_bitmap;
	long request;

	request_bitmap = panel_get_pending_requests();

	/* (optionally) disable particular functions */
	request_bitmap &= ~options.disable;

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


int main(int argc, char *argv[])
{
	options_process_or_abort(&options, argc, argv);
	if (options.info) {
		printf("Video driver: %s \n", get_vga_driver_name());
		printf("\nTemperature sensors: \n");
		sensors_show(SENSORS_FEATURE_TEMP);
		exit(0);
	}
	if ( !options.i2c_bus_set ) {
		options.i2c_bus = panel_lookup_i2c_bus();
		if (options.i2c_bus < 0) {
			fprintf(stderr, "Could not detect front panel I2C bus \n");
			exit(1);
		}
	}

	daemonize();
	initialize();
	/*
	 * Main FP communication routine is armed
	 * upon SIGALRM reception.
	 */
	alarm(ATFP_MAIN_STARTUP_DELAY);
	daemon_termination(DTERM_WAIT);
	slogn("AirTop Front-Panel Service -- stop");

	cleanup();
	return 0;
}

