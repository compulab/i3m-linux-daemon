/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _PANEL_H
#define _PANEL_H

#define I2C_PANEL_INTERFACE_ADDR	0x21

#define I2C_DEV_NAME_LENGTH		32

int panel_open_i2c(int i2c_bus, int i2c_addr);
void panel_close(void);
int panel_read_byte(unsigned regno);
int panel_write_byte(unsigned regno, int data);

long panel_get_pending_requests(void);
int panel_set_temperature(int cpu_id, int temp);
int panel_set_frequency(int cpu_id, int freq);
int panel_set_gpu_temp(int temp);
int panel_reset(void);

int panel_lookup_i2c_bus(void);

#endif	/* _PANEL_H */

