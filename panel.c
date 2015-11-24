/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

#include "panel.h"
#include "registers.h"
#include "i2c-tools.h"
#include "stats.h"


/* 
 * Panel descriptor. 
 * Panel descriptor is managed as a singleton object. 
 */
static int panel = -1;


/*
 * Open i2c-connected device identified by {i2c-bus:addr} tuple.
 * Notice, there is no limitation on number of simultaneously opened devices
 * belonging to the same i2c bus.
 */
int panel_open_i2c_device(int i2c_bus, int i2c_addr)
{
	int i2c_devnum;
	int err = 0;

	if (panel >= 0) {
		fprintf(stderr, "panel i2c device is already open \n");
		return panel;
	}

	i2c_devnum = open_i2c_dev(i2c_bus);
	if (i2c_devnum < 0) {
		fprintf(stderr, "i2c-%d: could not open: %d \n", i2c_bus, i2c_devnum);
		err = i2c_devnum;
		goto i2c_out_err0;
	}

	err = set_slave_addr(i2c_devnum, i2c_addr);
	if (err < 0) {
		fprintf(stderr, "i2c-%d:%02x could not set i2c slave: %d \n", i2c_bus, i2c_addr, err);
		goto i2c_out_err1;
	}

	panel = i2c_devnum;
	return i2c_devnum;

i2c_out_err1:
	close(i2c_devnum);

i2c_out_err0:
	return err;
}

void panel_close(void)
{
	close(panel);
	panel = -1;
}

int panel_read_byte(unsigned regno)
{
	int value;

	value = i2c_smbus_read_byte_data(panel, regno);
	if (value < 0) {
		fprintf(stderr, "Could not read register %02x: %d \n", regno, value);
	}
	else {
		stat_inc_i2c_read_count();
	}

	return value;
}

int panel_write_byte(unsigned regno, int data)
{
	int err;

	/*
	 * Guarantee some time to finish processing of the previous transaction.
	 * TODO:
	 * The ATFP controller should implement transaction failing mechanism
	 * whenever it is not ready to accept next transaction, then we will
	 * retry after a delay.
	 */
	usleep(10000);

	err = i2c_smbus_write_byte_data(panel, regno, data);
	if (err < 0) {
		fprintf(stderr, "Could not write register %02x: %d \n", regno, err);
	}
	else {
		stat_inc_i2c_write_count();
	}

	return err;
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

		fd = panel_open_i2c_device(adapters[i].nr, I2C_PANEL_INTERFACE_ADDR);
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

/*
 * Return a bitmap of pending requests. 
 * In case of error: return 0 - meaning no requests. 
 */
long panel_get_pending_requests(void)
{
	int value;

	value = panel_read_byte(ATFP_REG_REQ);
	if (value < 0)
		return 0L;

	/* no requests pending */
	if ( !(value & 0x01) )
		return 0L;

	value = panel_read_byte(ATFP_REG_PENDR0);
	if (value < 0)
		return 0L;

	return (long)value;
}

int panel_set_temperature(int cpu_id, int temp)
{
	int err;
	int temp_reg = ATFP_REG_CPU0T + cpu_id;
	int valid_mask = 1 << cpu_id;

	/* assert((cpu_id >= 0) && (cpu_id <= 7)); */

	err = panel_write_byte(temp_reg, temp);
	if ( !err )
		err = panel_write_byte(ATFP_REG_CPUTS, valid_mask);

	return err;
}

int panel_reset(void)
{
	return panel_write_byte(ATFP_REG_FPCTRL, ATFP_MASK_FPCTRL_RST);
}

