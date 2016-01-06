/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#ifndef _SENSORS_H
#define _SENSORS_H

int sensors_show(int sens_feature_type);
int sensors_coretemp_init(void);
int sensors_coretemp_read(int *core_id, int *temp);

int sensors_nouveau_init(void);
int sensors_nouveau_read(int *temp);

#endif	/* _SENSORS_H */

