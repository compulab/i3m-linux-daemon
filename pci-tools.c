/*
 * Copyright (C) 2016, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pci/pci.h>

#define DRIVER_BUF_SIZE			128

/*
 * find_driver() is highly inspired by find_driver() of pciutils project.
 */
static char *find_driver(struct pci_dev *dev, char *buf)
{
	char name[1024], *drv, *base;
	int n;

	if (dev->access->method != PCI_ACCESS_SYS_BUS_PCI)
		return NULL;

	base = pci_get_param(dev->access, "sysfs.path");
	if (!base || !base[0])
		return NULL;

	n = snprintf(name, sizeof(name), "%s/devices/%04x:%02x:%02x.%d/driver",
		base, dev->domain, dev->bus, dev->dev, dev->func);
	if (n < 0 || n >= (int)sizeof(name)) {
		return NULL;
	}

	n = readlink(name, buf, DRIVER_BUF_SIZE);
	if (n < 0)
		return NULL;
	if (n >= DRIVER_BUF_SIZE)
		return NULL;
	buf[n] = 0;

	if ((drv = strrchr(buf, '/')))
		return drv+1;
	else
		return buf;
}

static char *get_driver_name_by_pci_class(struct pci_access *pacc, char *buf, unsigned int pci_dev_class)
{
	struct pci_dev *dev;
	char *name;

	pci_scan_bus(pacc);
	for (dev = pacc->devices; dev != NULL; dev = dev->next) {
		if (dev->device_class != pci_dev_class)
			continue;

		name = find_driver(dev, buf);
		if (name != NULL)
			return name;
	}

	return NULL;
}

char *get_vga_driver_name(void)
{
	static char buffer[DRIVER_BUF_SIZE];
	static char *name = NULL;
	static bool buffer_is_initialized = false;

	struct pci_access *pacc;

	if ( buffer_is_initialized )
		return name;

	buffer_is_initialized = true;
	pacc = pci_alloc();
	pci_init(pacc);
	name = get_driver_name_by_pci_class(pacc, buffer, (0x030000 >> 8)/*VGA compatible controller*/);
	pci_cleanup(pacc);

	return name;
}

