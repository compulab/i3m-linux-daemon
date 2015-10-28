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

static bool daemon_terminate = false;

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
}

static void cleanup(void)
{
}

static int main_loop(void)
{
	while ( !daemon_terminate ) {
		printf(".");
		fflush(stdout);
		sleep(1);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;

	initialize();
	ret = main_loop();
	cleanup();

	return ret;
}

