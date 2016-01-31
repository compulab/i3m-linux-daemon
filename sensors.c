/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GNU GPLv2 or later, at your option
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>

#include <sensors/sensors.h>

#include "common.h"

#define SENSORS_CONFIG_FILE		NULL
#define CORETEMP_SUBFEATURE_MAX		8

struct sensors_info {
	bool init_done;

	/* coretemp */
	const sensors_chip_name *coretemp_chipname;
	int coretemp_subfeature_ids[CORETEMP_SUBFEATURE_MAX];
	int coretemp_subfeature_num;

	/* nouveau */
	bool nouveau_sensor_detected;
	const sensors_chip_name *nouveau_chipname;
	int nouveau_subfeature_id;
};

static struct sensors_info sensors = {0};

/*
 * Show all the available sensors of a particular type.
 * Args:
 * sens_feature_type - sensor type, e.g. SENSORS_FEATURE_TEMP
 * Notice:
 * This function sends output to the console, rather than syslem log.
 */
int sensors_show(int sens_feature_type)
{
	int err;
	int chipno;
	int featno;
	const sensors_chip_name *chipname;
	const sensors_feature *feature;
	char *da_featname;	/* Dynamically Allocated */

	err = sensors_init(SENSORS_CONFIG_FILE);
	if (err) {
		fprintf(stderr, "Could not initialize lm-sensors: %d \n", err);
		goto sens_out_err_0;
	}

	chipno = 0;
	while ((chipname = sensors_get_detected_chips(NULL, &chipno)) != NULL) {
		printf("%2d: %s [ %s ] \n", chipno, chipname->prefix, chipname->path);

		featno = 0;
		while ((feature = sensors_get_features(chipname, &featno)) != NULL) {
			/* filter according to feature type, e.g. SENSORS_FEATURE_TEMP */
			if (feature->type != sens_feature_type)
				continue;

			da_featname = sensors_get_label(chipname, feature);
			printf("\t%2d: %s \n", featno, da_featname);
			free(da_featname);
		}
	}

	sensors_cleanup();

sens_out_err_0:
	return err;
}

/*
 * Coretemp - CPU core temperature
 */

int sensors_coretemp_init(void)
{
	const sensors_chip_name chipmatch = {
		.prefix		= "coretemp",
		.bus.type	= SENSORS_BUS_TYPE_ANY,
		.bus.nr		= SENSORS_BUS_NR_ANY,
		.addr		= SENSORS_CHIP_NAME_ADDR_ANY,
	};

	int err;
	int chipno;
	const sensors_chip_name *chipname;
	int featno;
	const sensors_feature *feature;
	char *da_featname;	/* Dynamically Allocated */
	const sensors_subfeature *subfeature;
	int i;
	int core_id;

	if ( !sensors.init_done ) {
		err = sensors_init(SENSORS_CONFIG_FILE);
		if ( err ) {
			sloge("Could not initialize lm-sensors: %d", err);
			return err;
		}

		sensors.init_done = true;
	}

	/* get pre-defined sensor chip */
	chipno = 0;
	chipname = sensors_get_detected_chips(&chipmatch, &chipno);
	if ( !chipname ) {
		sloge("coretemp: could not detect sensor chip");
		return -ENODEV;
	}

	sensors.coretemp_chipname = chipname;

	/* gather reference to temperature sensors */
	featno = 0;
	while ((feature = sensors_get_features(&chipmatch, &featno)) != NULL) {
		/* filter in temperature */
		if (feature->type != SENSORS_FEATURE_TEMP)
			continue;

		da_featname = sensors_get_label(chipname, feature);
		i = sscanf(da_featname, "Core %d", &core_id);
		free(da_featname);
		if (i == 0)
			continue;

		subfeature = sensors_get_subfeature(&chipmatch, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
		if ( !subfeature ) {
			sloge("coretemp: could not get subfeature");
			err = -ENODEV;
			goto out_err;
		}

		/* assert((core_id >= 0) && (core_id < CORETEMP_SUBFEATURE_MAX)); */
		sensors.coretemp_subfeature_ids[core_id] = subfeature->number;
		++sensors.coretemp_subfeature_num;
	}

	return 0;

out_err:
	sensors_cleanup();
	return err;
}

int sensors_coretemp_read(int *core_id, int *temp)
{
	int err;
	double temp0;

	/* assert((*core_id >= 0) && (*core_id < sensors.coretemp_subfeature_num)); */
	err = sensors_get_value(sensors.coretemp_chipname, sensors.coretemp_subfeature_ids[ *core_id ], &temp0);
	if ( err ) {
		sloge("coretemp: Core %d: could not get temperature value: %d", *core_id, err);
		goto out_err;
	}

	/* temp0 is _effectively_ int */
	*temp = (int)temp0;
	if (*core_id < (sensors.coretemp_subfeature_num - 1)) {
		++(*core_id);
	}
	else {
		*core_id = -1;
	}

out_err:
	return err;
}


/*
 * Nouveau - NVIDIA GPU temperature under Nouveau open source driver
 */

int sensors_nouveau_init(void)
{
	const sensors_chip_name chipmatch = {
		.prefix		= "nouveau",
		.bus.type	= SENSORS_BUS_TYPE_ANY,
		.bus.nr		= SENSORS_BUS_NR_ANY,
		.addr		= SENSORS_CHIP_NAME_ADDR_ANY,
	};

	int err;
	int chipno;
	const sensors_chip_name *chipname;
	int featno;
	const sensors_feature *feature;
	const sensors_subfeature *subfeature;

	/* get pre-defined sensor chip */
	chipno = 0;
	chipname = sensors_get_detected_chips(&chipmatch, &chipno);
	if ( !chipname ) {
		/* Ok: quite possibly nouveau driver is not in use */
		return -ENODEV;
	}

	sensors.nouveau_sensor_detected = true;
	sensors.nouveau_chipname = chipname;

	/* gather reference to temperature sensors */
	featno = 0;
	while ((feature = sensors_get_features(&chipmatch, &featno)) != NULL) {
		/* filter in temperature */
		if (feature->type != SENSORS_FEATURE_TEMP)
			continue;

		subfeature = sensors_get_subfeature(&chipmatch, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
		if ( !subfeature ) {
			sloge("nouveau: could not get subfeature");
			err = -ENODEV;
			goto out_err;
		}

		sensors.nouveau_subfeature_id = subfeature->number;

		/* it is believed nouveau has only one temperature sensor */
		break;
	}

	return 0;

out_err:
	sensors.nouveau_sensor_detected = false;
	return err;
}

int sensors_nouveau_read(int *temp)
{
	int err;
	double temp0;

	if ( !sensors.nouveau_sensor_detected )
		return -ENODEV;

	err = sensors_get_value(sensors.nouveau_chipname, sensors.nouveau_subfeature_id, &temp0);
	if ( err ) {
		sloge("nouveau: could not get temperature value: %d", err);
		goto out_err;
	}

	/* temp0 is _effectively_ int */
	*temp = (int)temp0;

out_err:
	return err;
}

