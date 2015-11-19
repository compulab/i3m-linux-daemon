/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

#include "panel.h"
#include "registers.h"

/*
 * Open i2c-connected device identified by {i2c-bus:addr} tuple.
 * Notice, there is no limitation on number of simultaneously opened devices
 * belonging to the same i2c bus.
 */
int panel_open_i2c_device(int i2c_bus, int i2c_addr)
{
	char i2c_devname[I2C_DEV_NAME_LENGTH];
	int i2c_devnum;
	int err = 0;

	snprintf(i2c_devname, I2C_DEV_NAME_LENGTH, "/dev/i2c-%d", i2c_bus);

	/* open i2c device */
	i2c_devnum = open(i2c_devname, O_RDWR);
	if (i2c_devnum < 0) {
		fprintf(stderr, "%s: could not open i2c device: %d \n", i2c_devname, i2c_devnum);
		err = i2c_devnum;
		goto i2c_out_err_0;
	}

	/* set i2c slave for our device */
	err = ioctl(i2c_devnum, I2C_SLAVE, i2c_addr);
	if (err < 0) {
		fprintf(stderr, "%s:%02x could not set i2c slave: %d \n", i2c_devname, i2c_addr, err);
		goto i2c_out_err_1;
	}

	return i2c_devnum;

i2c_out_err_1:
	close(i2c_devnum);

i2c_out_err_0:
	return err;
}

int panel_read_byte(int i2c_devnum, unsigned regno)
{
	int value;

	value = i2c_smbus_read_byte_data(i2c_devnum, regno);
	if (value < 0) {
		fprintf(stderr, "Could not read register %02x: %d \n", regno, value);
	}

	return value;
}

int panel_write_byte(int i2c_devnum, unsigned regno, int data)
{
	int err;

	err = i2c_smbus_write_byte_data(i2c_devnum, regno, data);
	if (err < 0) {
		fprintf(stderr, "Could not write register %02x: %d \n", regno, err);
	}

	return err;
}


/*
 * Return a bitmap of pending requests. 
 * In case of error: return 0 - meaning no requests. 
 */
long panel_get_pending_requests(int i2c_devnum)
{
	int value;

	value = panel_read_byte(i2c_devnum, ATFP_REG_REQ);
	if (value < 0)
		return 0L;

	/* no requests pending */
	if ( !(value & 0x01) )
		return 0L;

	value = panel_read_byte(i2c_devnum, ATFP_REG_PENDR0);
	if (value < 0)
		return 0L;

	return (long)value;
}

int panel_set_temperature(int i2c_devnum, int cpu_id, int temp)
{
	int err;
	int temp_reg = ATFP_REG_CPU0T + cpu_id;
	int valid_mask = 1 << cpu_id;

	/* assert((cpu_id >= 0) && (cpu_id <= 7)); */

	err = panel_write_byte(i2c_devnum, temp_reg, temp);
	if ( !err )
		err = panel_write_byte(i2c_devnum, ATFP_REG_CPUTS, valid_mask);

	return err;
}


#include <dirent.h>
#include <stdlib.h>
#include <string.h>


struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

//enum adt { adt_dummy, adt_isa, adt_i2c, adt_smbus, adt_unknown };


#define BUNCH	8


/* Remove trailing spaces from a string
   Return the new string length including the trailing NUL */
static int rtrim(char *s)
{
	int i;

	for (i = strlen(s) - 1; i >= 0 && (s[i] == ' ' || s[i] == '\n'); i--)
		s[i] = '\0';
	return i + 2;
}

static void free_adapters(struct i2c_adap *adapters)
{
	int i;

	for (i = 0; adapters[i].name; i++)
		free(adapters[i].name);
	free(adapters);
}

/* n must match the size of adapters at calling time */
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
//			enum adt type;
			char *px;

			px = fgets(s, 120, f);
			fclose(f);
			if (!px) {
				fprintf(stderr, "%s: read error\n", n);
				continue;
			}
			if ((px = strchr(s, '\n')) != NULL)
				*px = 0;
			if (!sscanf(de->d_name, "i2c-%d", &i2cbus))
				continue;
//			if (!strncmp(s, "ISA ", 4)) {
//				type = adt_isa;
//			} else {
//				/* Attempt to probe for adapter capabilities */
//				type = i2c_get_funcs(i2cbus);
//			}

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
//			adapters[count].funcs = adap_types[type].funcs;
//			adapters[count].algo = adap_types[type].algo;
			count++;
		}
	}
	closedir(dir);

done:
	return adapters;
}

int panel_lookup_i2c_bus(void)
{
	struct i2c_adap *adapters;
	int i, j;
	int fd;
	char signature[5];
	int ret = -1;

	adapters = gather_i2c_busses();
	for (i = 0; adapters && adapters[i].name; ++i) {

		fd = panel_open_i2c_device(adapters[i].nr, 0x21);
		if (fd < 0)
			continue;

		for (j = 0; j < 4; ++j) {
			signature[j] = i2c_smbus_read_byte_data(fd, (ATFP_REG_SIG0 + j));
			if (signature[j] < 0)
				break;
		}
		signature[j] = 0;
		close(fd);

		if (!strcmp("CLFP", signature)) {
			ret = i;
			goto lookup_out;
		}
	}

lookup_out:
	free_adapters(adapters);
	return ret;
}

