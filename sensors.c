/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sensors/sensors.h>

#define SENSORS_CONFIG_FILE		NULL

static const sensors_chip_name chipmatch = {
	.prefix		= "coretemp",
	.bus.type	= SENSORS_BUS_TYPE_ANY,
	.bus.nr		= SENSORS_BUS_NR_ANY,
	.addr		= SENSORS_CHIP_NAME_ADDR_ANY,
};

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



void sensors_test(void)
{
	int err;
	int chipno;
	const sensors_chip_name *chipname;
	int featno;
	const sensors_feature *feature;
	char *da_featname;	/* Dynamically Allocated */
	const sensors_subfeature *subfeature;
	double temp = 0.0;
	int i;
	int core_id;

	err = sensors_init(SENSORS_CONFIG_FILE);
	if ( err ) {
		fprintf(stderr, "Could not initialize lm-sensors: %d \n", err);
		return;
	}

	/* get pre-defined sensor chip */
	chipno = 0;
	chipname = sensors_get_detected_chips(&chipmatch, &chipno);
	if ( !chipname ) {
		fprintf(stderr, "Could not detect 'coretemp' chip \n");
		return;
	}

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
			fprintf(stderr, "Could not get subfeature \n");
			goto out_err;
		}

		// ----- can be re-acquired as many times as required

		err = sensors_get_value(chipname, subfeature->number, &temp);
		if ( err ) {
			fprintf(stderr, "Core %d: could not get temperature value: %d \n", core_id, err);
			goto out_err;
		}

		printf("Core %d: %g [deg] \n", core_id, temp);
	}

out_err:
	sensors_cleanup();
}

