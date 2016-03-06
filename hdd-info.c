/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 * 
 * Gather some HDD-related information using the S.M.A.R.T. technology. 
 * This code relies on libatasmart.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <atasmart.h>

#include "common.h"
#include "dlist.h"
#include "hdd-info.h"


#define SYS_BLOCK_PATH			"/sys/block"
#define DEV_BLOCK_PATH			"/dev"


DList *smart_devices = NULL;


/*
 * SMARTinfo freelist-based memory management
 */
DList *SMARTinfo_freelist = NULL;

SMARTinfo *new_SMARTinfo(void)
{
	SMARTinfo *si;

	if (SMARTinfo_freelist == NULL)
		SMARTinfo_freelist = dlist_create(NULL);

	si = dlist_pop_front(SMARTinfo_freelist);
	if (si == NULL)
		si = (SMARTinfo *)malloc(sizeof(SMARTinfo));
	memset(si, 0, sizeof(SMARTinfo));

	return si;
}

void delete_SMARTinfo(SMARTinfo *si)
{
	if (si == NULL)
		return;

	dlist_push_back(SMARTinfo_freelist, si);
}


static void hdd_info_cleanup(void)
{
	SMARTinfo *si;

	while ((si = dlist_pop_front(SMARTinfo_freelist)) != NULL)
		free(si);

	dlist_destroy(SMARTinfo_freelist);
	dlist_destroy(smart_devices);
}


static unsigned int to_celsius(uint64_t millikelvin)
{
	return (millikelvin - 273150) / 1000;
}

/*
 * Implement visitor pattern on each directory under 'path'.
 */
static void scan_dirs(const char *path, int (*visitor)(const char *ata_dev, void *arg), void *varg)
{
	DIR *root;
	struct dirent *d;
	char buffer[128];
	bool keep_searching;
	int n;

	root = opendir(path);
	if (root == NULL) {
		sloge("%s: could not open directory: %m", path);
		return;
	}

	keep_searching = true;
	while (((d = readdir(root)) != NULL) && keep_searching) {
		if (!strcmp(".", d->d_name) || !strcmp("..", d->d_name))
			continue;

		n = readlinkat(dirfd(root), d->d_name, buffer, sizeof(buffer));
		if (n < 0)
			continue;

		buffer[n] = '\0';
		if (!strstr(buffer, "/ata"))
			continue;

		keep_searching = visitor(d->d_name, varg);
	}

	closedir(root);
}

static int atasmart_get_info(const char *devname, void *arg)
{
	int err;
	char device[64];
	SkDisk *d;
	SMARTinfo *si;
	uint64_t mkelvin;
	uint64_t size_B;
	DList *hdd_list = (DList *)arg;

	snprintf(device, sizeof(device), "%s/%s", DEV_BLOCK_PATH, devname);

	err = sk_disk_open(device, &d);
	if (err < 0) {
		sloge("%s: could not open: %m", device);
		goto gettemp_out0;
	}

	err = sk_disk_smart_read_data(d);
	if (err < 0) {
		slogi("%s: could not read SMART data: %m", device);
		goto gettemp_out1;
	}

	si = new_SMARTinfo();
	strncpy(si->devname, devname, HDD_DEVNAME_SIZE);
	err = sk_disk_smart_get_temperature(d, &mkelvin);
	if (err < 0) {
		slogd("%s: SMART: temperature is not available", device);
	}
	else {
		si->temp = to_celsius(mkelvin);
		si->temp_valid = true;
	}

	err = sk_disk_get_size(d, &size_B);
	if (err < 0) {
		slogd("%s: SMART: size is not available", device);
	}
	else {
		si->size_GB = (unsigned int)(size_B >> 30);
		si->size_valid = true;
	}
	dlist_push_back(hdd_list, si);

gettemp_out1:
	sk_disk_free(d);

gettemp_out0:
	return 1;
}


void hdd_get_temperature(DList **sd)
{
	if (smart_devices == NULL) {
		smart_devices = dlist_create(NULL);
		atexit(hdd_info_cleanup);
	}

	scan_dirs(SYS_BLOCK_PATH, atasmart_get_info, smart_devices);
	*sd = smart_devices;
}

