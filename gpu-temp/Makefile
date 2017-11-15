#
# Copyright (C) 2016 CompuLab ltd.
# Author: Andrey Gelman <andrey.gelman@compulab.co.il>
# License: GNU GPLv2 or later, at your option
#

SOURCES = gpu-thermald.c gpu-options.c ../nvml-tools.c

OBJDIR  = ../obj
BINDIR  = ../bin
OUTFILE = $(BINDIR)/gpu-thermald

NVMLDIR = ../nvml

SRC_TEMP = $(notdir $(SOURCES))
OBJS = $(SRC_TEMP:%.c=$(OBJDIR)/%.o)
CFLAGS  = -Wall -I$(NVMLDIR)
LFLAGS  = -Wall -rdynamic
LLIBS   = -ldl
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

gpu-temp: $(SOURCES) $(OBJS) $(BINDIR)
	@echo 'LD    $@'
	@$(LINK_CMD) $(OBJS) $(LLIBS) -o $(OUTFILE)
	@echo 'STRIP $@'
	@$(STRIP_CMD) $(OUTFILE)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@echo 'CC    $@'
	@$(COMPILE_CMD) -c $+ -o $@

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)
	@echo 'CC    $@'
	@$(COMPILE_CMD) -c $+ -o $@

$(OBJDIR):
	$(MKDIR_CMD) $(OBJDIR)

$(BINDIR):
	$(MKDIR_CMD) $(BINDIR)

.PHONY: clean
clean:
	$(RM_CMD) $(OBJDIR)/*
	$(RM_CMD) $(BINDIR)/*

