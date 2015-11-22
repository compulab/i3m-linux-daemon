/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 *
 * This code is mostly taken from i2c-tools project.
 */

#ifndef _I2C_TOOLS_H
#define _I2C_TOOLS_H

#define I2C_DEV_NAME_LENGTH		32

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

int open_i2c_dev(int i2cbus);
int set_slave_addr(int file, int address);

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);

#endif	/* _I2C_TOOLS_H */

