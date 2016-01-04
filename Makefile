#
# Copyright (C) 2015 CompuLab ltd.
# Author: Andrey Gelman <andrey.gelman@compulab.co.il>
# License: GPL-2
#

SOURCES = main.c panel.c sensors.c queue.c thread-pool.c domain-logic.c \
	i2c-tools.c stats.c cpu-freq.c pci-tools.c

OBJDIR  = obj
BINDIR  = bin
OUTFILE = $(BINDIR)/airtop-fpsvc

NVMLDIR = nvml

OBJS    = $(SOURCES:%.c=$(OBJDIR)/%.o)
CFLAGS  = -Wall -I$(NVMLDIR)
LFLAGS  = -Wall -lsensors -lpthread -lm -lpci -lnvidia-ml -L$(NVMLDIR)
COMPILE_CMD = gcc $(CFLAGS)
STRIP_CMD   = strip
LINK_CMD    = gcc $(LFLAGS)
MKDIR_CMD   = mkdir -p
RM_CMD      = rm -rf

ifdef debug
	CFLAGS += -g
	STRIP_CMD = @\#
endif

all: $(SOURCES) $(OBJS) $(BINDIR)
	$(LINK_CMD) $(OBJS) -o $(OUTFILE)
	$(STRIP_CMD) $(OUTFILE)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(COMPILE_CMD) -c $+ -o $@

$(OBJDIR):
	$(MKDIR_CMD) $(OBJDIR)

$(BINDIR):
	$(MKDIR_CMD) $(BINDIR)

.PHONY: clean
clean:
	$(RM_CMD) $(OBJDIR)/*
	$(RM_CMD) $(BINDIR)/*

