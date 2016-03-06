/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 *
 * This code relies on libatasmart.
 */

#ifndef _HDD_TEMP_H
#define _HDD_TEMP_H

#include "dlist.h"


#define HDD_DEVNAME_SIZE		8


typedef struct {
	DListNode dlist_hook;
	char devname[HDD_DEVNAME_SIZE];
	unsigned int temp;	/* degC */
	unsigned int size_GB;	/* GB */
	bool temp_valid;
	bool size_valid;

} SMARTinfo;


SMARTinfo *new_SMARTinfo(void);
void delete_SMARTinfo(SMARTinfo *si);

void hdd_get_temperature(DList **sd);

#endif	/* _HDD_TEMP_H */

