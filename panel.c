/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>
#include <i2c/smbus.h>

#include "panel.h"
#include "registers.h"
#include "i2c-tools.h"
#include "stats.h"
#include "common.h"


/* 
 * Panel descriptor. 
 * Panel descriptor is managed as a singleton object. 
 */
typedef struct {
	bool is_initialized;
	int i2c_desc;
	unsigned int i2c_delay;
} Panel;

static Panel panel = {0};


/*
 * Open i2c-connected device identified by {i2c-bus:addr} tuple.
 * Notice, there is no limitation on number of simultaneously opened devices
 * belonging to the same i2c bus.
 */
static int __panel_open_i2c_device(int i2c_bus, int i2c_addr)
{
	int i2c_devnum;
	int err = 0;

	i2c_devnum = open_i2c_dev(i2c_bus);
	if (i2c_devnum < 0) {
		sloge("i2c-%d: could not open: %d", i2c_bus, i2c_devnum);
		err = i2c_devnum;
		goto i2c_out_err0;
	}

	err = set_slave_addr(i2c_devnum, i2c_addr);
	if (err < 0) {
		sloge("i2c-%d:%02x could not set i2c slave: %d", i2c_bus, i2c_addr, err);
		goto i2c_out_err1;
	}

	return i2c_devnum;

i2c_out_err1:
	close(i2c_devnum);

i2c_out_err0:
	return err;
}

int panel_open_i2c(int i2c_bus, int i2c_addr, unsigned int i2c_delay)
{
	int i2c_devnum;

	if ( panel.is_initialized ) {
		sloge("panel i2c device is already open");
		return -EEXIST;
	}

	i2c_devnum = __panel_open_i2c_device(i2c_bus, i2c_addr);
	if (i2c_devnum < 0)
		return i2c_devnum;

	panel.is_initialized = true;
	panel.i2c_desc = i2c_devnum;
	panel.i2c_delay = i2c_delay;
	return 0;
}

void panel_close(void)
{
	if ( !panel.is_initialized )
		return;

	close(panel.i2c_desc);
	memset(&panel, 0, sizeof(Panel));
}

int panel_read_byte(unsigned regno)
{
	int value;

	value = i2c_smbus_read_byte_data(panel.i2c_desc, regno);
	if (value < 0) {
		sloge("Could not read register %02x: %d", regno, value);
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
	 * Optional delay [uSec] in order to control output rate.
	 */
	if (panel.i2c_delay > 0)
		usleep(panel.i2c_delay);

	err = i2c_smbus_write_byte_data(panel.i2c_desc, regno, data);
	if (err < 0) {
		sloge("Could not write register %02x: %d", regno, err);
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

		fd = __panel_open_i2c_device(adapters[i].nr, I2C_PANEL_INTERFACE_ADDR);
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

int panel_set_frequency(int cpu_id, int freq)
{
	int err;
	int freq_reg_lsb = ATFP_REG_CPU0F_LSB + (cpu_id * 2);
	int freq_reg_msb = ATFP_REG_CPU0F_MSB + (cpu_id * 2);

	/* assert((cpu_id >= 0) && (cpu_id <= 7)); */

	err = panel_write_byte(freq_reg_lsb, (freq & 0xFF));
	if ( !err )
		err = panel_write_byte(freq_reg_msb, (0x80 | ((freq >> 8) & 0xFF)));

	return err;
}

int panel_set_gpu_temp(int temp)
{
	int err;

	err = panel_write_byte(ATFP_REG_GPUT, temp);
	if ( !err )
		err = panel_write_byte(ATFP_REG_SENSORT, ATFP_MASK_SENSORT_GPUS);

	return err;
}

int panel_set_hdd_temp(int hdd_id, int temp)
{
	int err;
	int temp_reg = ATFP_REG_HDD0T + hdd_id;
	int valid_mask = 1 << hdd_id;

	if ((hdd_id < 0) || (hdd_id > 7)) {
		slogw("HDD: index out of range: %d", hdd_id);
		return -EINVAL;
	}

	err = panel_write_byte(temp_reg, temp);
	if ( !err )
		err = panel_write_byte(ATFP_REG_HDDTS, valid_mask);

	return err;
}

int panel_reset(void)
{
	return panel_write_byte(ATFP_REG_FPCTRL, ATFP_MASK_FPCTRL_RST);
}

int panel_store_daemon_postcode(void)
{
	int err;
	int postcode_msb;
	int postcode_lsb;

	err = panel_write_byte(ATFP_REG_POST_CODE_MSB, ATFP_DAEMON_POSTCODE_MSB);
	if ( err )
		goto postcode_out;

	err = panel_write_byte(ATFP_REG_POST_CODE_LSB, ATFP_DAEMON_POSTCODE_LSB);
	if ( err )
		goto postcode_out;

	/* selftest: read back */
	postcode_msb = panel_read_byte(ATFP_REG_POST_CODE_MSB);
	postcode_lsb = panel_read_byte(ATFP_REG_POST_CODE_LSB);
	if ((postcode_msb != ATFP_DAEMON_POSTCODE_MSB) || (postcode_lsb != ATFP_DAEMON_POSTCODE_LSB))
		err = -EINVAL;

postcode_out:
	return err;
}

