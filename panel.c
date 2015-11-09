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

