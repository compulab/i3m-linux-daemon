/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 *
 * This code is mostly taken from i2c-tools project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>

#include "i2c-tools.h"
#include "common.h"


#define BUNCH				8
#define I2C_DEV_PATH_A			"/dev/i2c-%d"
#define I2C_DEV_PATH_B			"/dev/i2c/%d"


int open_i2c_dev(int i2cbus)
{
	char filename[I2C_DEV_NAME_LENGTH];
	int file;

	snprintf(filename, I2C_DEV_NAME_LENGTH, I2C_DEV_PATH_A, i2cbus);
	file = open(filename, O_RDWR);

	if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
		snprintf(filename, I2C_DEV_NAME_LENGTH, I2C_DEV_PATH_B, i2cbus);
		file = open(filename, O_RDWR);
	}

	if (file < 0)
		return -errno;

	return file;
}

int set_slave_addr(int file, int address)
{
	if (ioctl(file, I2C_SLAVE, address) < 0)
		return -errno;

	return 0;
}


/*
 * Remove trailing spaces from a string.
 * Return the new string length including the trailing NULL.
 */
static int rtrim(char *s)
{
	int i;

	for (i = strlen(s) - 1; i >= 0 && (s[i] == ' ' || s[i] == '\n'); i--)
		s[i] = '\0';
	return i + 2;
}

/*
 * Reallocate adapters[] to adapters[n].
 * n must match the size of adapters at calling time.
 */
static struct i2c_adap *more_adapters(struct i2c_adap *adapters, int n)
{
	struct i2c_adap *new_adapters;

	new_adapters = realloc(adapters, (n + BUNCH) * sizeof(struct i2c_adap));
	if (!new_adapters) {
		free_adapters(adapters);
		return NULL;
	}
	memset(new_adapters + n, 0, BUNCH * sizeof(struct i2c_adap));

	return new_adapters;
}

struct i2c_adap *gather_i2c_busses(void)
{
	char s[120];
	struct dirent *de, *dde;
	DIR *dir, *ddir;
	FILE *f;
	char fstype[NAME_MAX], sysfs[NAME_MAX], n[NAME_MAX];
	int foundsysfs = 0;
	int count=0;
	struct i2c_adap *adapters;

	adapters = calloc(BUNCH, sizeof(struct i2c_adap));
	if (!adapters)
		return NULL;

	/* look in /proc/bus/i2c */
	if ((f = fopen("/proc/bus/i2c", "r"))) {
		while (fgets(s, 120, f)) {
			char *algo, *name, *type, *all;
			int len_algo, len_name, len_type;
			int i2cbus;

			algo = strrchr(s, '\t');
			*(algo++) = '\0';
			len_algo = rtrim(algo);

			name = strrchr(s, '\t');
			*(name++) = '\0';
			len_name = rtrim(name);

			type = strrchr(s, '\t');
			*(type++) = '\0';
			len_type = rtrim(type);

			sscanf(s, "i2c-%d", &i2cbus);

			if ((count + 1) % BUNCH == 0) {
				/* We need more space */
				adapters = more_adapters(adapters, count + 1);
				if (!adapters)
					return NULL;
			}

			all = malloc(len_name + len_type + len_algo);
			if (all == NULL) {
				free_adapters(adapters);
				return NULL;
			}
			adapters[count].nr = i2cbus;
			adapters[count].name = strcpy(all, name);
			adapters[count].funcs = strcpy(all + len_name, type);
			adapters[count].algo = strcpy(all + len_name + len_type,
						      algo);
			count++;
		}
		fclose(f);
		goto done;
	}

	/* look in sysfs */
	/* First figure out where sysfs was mounted */
	if ((f = fopen("/proc/mounts", "r")) == NULL) {
		goto done;
	}
	while (fgets(n, NAME_MAX, f)) {
		sscanf(n, "%*[^ ] %[^ ] %[^ ] %*s\n", sysfs, fstype);
		if (strcasecmp(fstype, "sysfs") == 0) {
			foundsysfs++;
			break;
		}
	}
	fclose(f);
	if (! foundsysfs) {
		goto done;
	}

	/* Bus numbers in i2c-adapter don't necessarily match those in
	   i2c-dev and what we really care about are the i2c-dev numbers.
	   Unfortunately the names are harder to get in i2c-dev */
	strcat(sysfs, "/class/i2c-dev");
	if(!(dir = opendir(sysfs)))
		goto done;
	/* go through the busses */
	while ((de = readdir(dir)) != NULL) {
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;

		/* this should work for kernels 2.6.5 or higher and */
		/* is preferred because is unambiguous */
		sprintf(n, "%s/%s/name", sysfs, de->d_name);
		f = fopen(n, "r");
		/* this seems to work for ISA */
		if(f == NULL) {
			sprintf(n, "%s/%s/device/name", sysfs, de->d_name);
			f = fopen(n, "r");
		}
		/* non-ISA is much harder */
		/* and this won't find the correct bus name if a driver
		   has more than one bus */
		if(f == NULL) {
			sprintf(n, "%s/%s/device", sysfs, de->d_name);
			if(!(ddir = opendir(n)))
				continue;
			while ((dde = readdir(ddir)) != NULL) {
				if (!strcmp(dde->d_name, "."))
					continue;
				if (!strcmp(dde->d_name, ".."))
					continue;
				if ((!strncmp(dde->d_name, "i2c-", 4))) {
					sprintf(n, "%s/%s/device/%s/name",
						sysfs, de->d_name, dde->d_name);
					if((f = fopen(n, "r")))
						goto found;
				}
			}
		}

found:
		if (f != NULL) {
			int i2cbus;
			// enum adt type;
			char *px;

			px = fgets(s, 120, f);
			fclose(f);
			if (!px) {
				sloge("%s: read error\n", n);
				continue;
			}
			if ((px = strchr(s, '\n')) != NULL)
				*px = 0;
			if (!sscanf(de->d_name, "i2c-%d", &i2cbus))
				continue;
			// if (!strncmp(s, "ISA ", 4)) {
			// 	type = adt_isa;
			// } else {
			// 	/* Attempt to probe for adapter capabilities */
			// 	type = i2c_get_funcs(i2cbus);
			// }

			if ((count + 1) % BUNCH == 0) {
				/* We need more space */
				adapters = more_adapters(adapters, count + 1);
				if (!adapters)
					return NULL;
			}

			adapters[count].nr = i2cbus;
			adapters[count].name = strdup(s);
			if (adapters[count].name == NULL) {
				free_adapters(adapters);
				return NULL;
			}
			// adapters[count].funcs = adap_types[type].funcs;
			// adapters[count].algo = adap_types[type].algo;
			count++;
		}
	}
	closedir(dir);

done:
	return adapters;
}

void free_adapters(struct i2c_adap *adapters)
{
	int i;

	for (i = 0; adapters[i].name; i++)
		free(adapters[i].name);
	free(adapters);
}

