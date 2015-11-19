/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#ifndef _PANEL_H
#define _PANEL_H

#define I2C_PANEL_INTERFACE_ADDR	0x21

#define I2C_DEV_NAME_LENGTH		32

int panel_open_i2c_device(int i2c_bus, int i2c_addr);
int panel_read_byte(int i2c_devnum, unsigned regno);
int panel_write_byte(int i2c_devnum, unsigned regno, int data);

long panel_get_pending_requests(int i2c_devnum);
int panel_set_temperature(int i2c_devnum, int cpu_id, int temp);

int panel_lookup_i2c_bus(void);

#endif	/* _PANEL_H */

