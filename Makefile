#
# Copyright (C) 2015 CompuLab ltd.
# Author: Andrey Gelman <andrey.gelman@compulab.co.il>
# License: GNU GPLv2 or later, at your option
#

# This project adheres to Semantic Versioning
MAJOR = 1
MINOR = 1
PATCH = 0
FPSRV_VERSION = $(MAJOR).$(MINOR).$(PATCH)

SOURCES = main.c panel.c sensors.c queue.c thread-pool.c domain-logic.c \
	i2c-tools.c stats.c cpu-freq.c vga-tools.c nvml-tools.c \
	dlist.c watchdog.c options.c hdd-info.c

SUBDIRS = gpu-temp

OBJDIR  = obj
BINDIR  = bin
OUTFILE = $(BINDIR)/airtop-fpsvc
AUTO_GENERATED_FILE = auto_generated.h

NVMLDIR = nvml

OBJS    = $(SOURCES:%.c=$(OBJDIR)/%.o)
CFLAGS  = -Wall -I$(NVMLDIR) -Wno-format-truncation
LFLAGS  = -Wall -rdynamic
LLIBS   = -lsensors -lpthread -lm -ldl -latasmart -li2c
COMPILE_CMD = gcc $(CFLAGS)
STRIP_CMD   = strip
LINK_CMD    = gcc $(LFLAGS)
MKDIR_CMD   = mkdir -p
RM_CMD      = rm -rf

ifdef debug
	CFLAGS += -g
	STRIP_CMD = @\#
else
	CFLAGS += -O2
endif

all: fpsvc subdirs

fpsvc: $(AUTO_GENERATED_FILE) $(SOURCES) $(OBJS) $(BINDIR)
	@echo 'LD    $@'
	@$(LINK_CMD) $(OBJS) $(LLIBS) -o $(OUTFILE)
	@echo 'STRIP $@'
	@$(STRIP_CMD) $(OUTFILE)

$(AUTO_GENERATED_FILE): .FORCE
	@( printf '#define VERSION "%s%s"\n' "$(FPSRV_VERSION)" \
	'$(shell ./setversion)' ) > $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@echo 'CC    $@'
	@$(COMPILE_CMD) -c $+ -o $@

$(OBJDIR):
	$(MKDIR_CMD) $(OBJDIR)

$(BINDIR):
	$(MKDIR_CMD) $(BINDIR)

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

.PHONY: clean .FORCE
clean:
	@echo 'CLEAN $(OBJDIR)'
	@$(RM_CMD) $(OBJDIR)/*
	@echo 'CLEAN $(BINDIR)'
	@$(RM_CMD) $(BINDIR)/*
	@echo 'CLEAN $(AUTO_GENERATED_FILE)'
	@$([ -f auto_generated.h ] && rm auto_generated.h)

