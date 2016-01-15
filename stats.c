/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <string.h>
#include <stdio.h>


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
	printf("ATFP Statistics: %d \n", atfp_stat.show_counter);
	printf("i2c write transactions: %ld \n", atfp_stat.i2c_trans_write);
	printf("i2c read transactions:  %ld \n", atfp_stat.i2c_trans_read);
	printf("watchdog list length: %ld \n", atfp_stat.watchdog_list_length);
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

