/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#ifndef _STATS_H
#define _STATS_H

void stat_reset(void);
void stat_show(void);
void stat_inc_i2c_write_count(void);
void stat_inc_i2c_read_count(void);
void stat_inc_watchdog_list_length(void);

#endif	/* _STATS_H */

