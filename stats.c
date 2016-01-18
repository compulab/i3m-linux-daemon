/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <string.h>
#include <stdio.h>

#include "common.h"


typedef struct {
	unsigned int show_counter;
	unsigned long i2c_trans_write;
	unsigned long i2c_trans_read;
	unsigned long watchdog_list_length;
} Statistics;


static Statistics atfp_stat = {0};


void stat_reset(void)
{
	memset(&atfp_stat, 0, sizeof(Statistics));
}

void stat_show(void)
{
	atfp_stat.show_counter++;
	slogn("ATFP Statistics: %d", atfp_stat.show_counter);
	slogn("i2c write transactions: %ld", atfp_stat.i2c_trans_write);
	slogn("i2c read transactions:  %ld", atfp_stat.i2c_trans_read);
	slogn("watchdog list length: %ld", atfp_stat.watchdog_list_length);
}

void stat_inc_i2c_write_count(void)
{
	atfp_stat.i2c_trans_write++;
}

void stat_inc_i2c_read_count(void)
{
	atfp_stat.i2c_trans_read++;
}

void stat_inc_watchdog_list_length(void)
{
	atfp_stat.watchdog_list_length++;
}

