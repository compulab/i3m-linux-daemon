#
# Copyright (C) 2015 CompuLab ltd.
# Author: Andrey Gelman <andrey.gelman@compulab.co.il>
# License: GNU GPLv2 or later, at your option
#

SOURCES = main.c panel.c sensors.c queue.c thread-pool.c domain-logic.c \
	i2c-tools.c stats.c cpu-freq.c vga-tools.c nvml-tools.c \
	dlist.c watchdog.c options.c

OBJDIR  = obj
BINDIR  = bin
OUTFILE = $(BINDIR)/airtop-fpsvc

NVMLDIR = nvml

OBJS    = $(SOURCES:%.c=$(OBJDIR)/%.o)
CFLAGS  = -Wall -I$(NVMLDIR)
LFLAGS  = -Wall -lsensors -lpthread -lm -ldl -rdynamic
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

