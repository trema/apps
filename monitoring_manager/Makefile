#
# Copyright (C) 2013 NEC Corporation
#

TREMA = ../../trema
CC = gcc
AR = ar
RANLIB = ranlib
TREMA_APPS = ..

CC = gcc
CFLAGS = $(shell $(TREMA)/trema-config --cflags) -I$(TREMA_APPS)/topology -I$(TREMA_APPS)/flow_manager -std=gnu99 -g -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA)/trema-config --libs) -L$(TREMA_APPS)/topology -ltopology -L$(TREMA_APPS)/flow_manager -lpath

TARGET_MANAGER = monitoring
SRCS_MANAGER = monitoring.c libmonitoring.c monitoring_interface.c utilmonitoring.c toolutils.c
OBJS_MANAGER = $(SRCS_MANAGER:.c=.o)

TARGET_LIB = libmonitoring.a
SRCS_LIB = libmonitoring.c monitoring_interface.c toolutils.c
OBJS_LIB = $(SRCS_LIB:.c=.o)
	
EXAMPLES = examples

TARGETS = $(TARGET_MANAGER) $(TARGET_LIB)
SRCS = $(SRCS_MANAGER) $(SRCS_LIB)
OBJS = $(OBJS_MANAGER) $(OBJS_LIB)

DEPENDS = .depends

.PHONY: all depend clean examples

.SUFFIXES: .c .o

all: depend $(TARGETS) examples

$(TARGET_MANAGER): $(OBJS_MANAGER)
	$(CC) $(OBJS_MANAGER) $(LDFLAGS) -o $@

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) -cq $@ $(OBJS_LIB)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

examples:
	cd $(EXAMPLES) && $(MAKE)

-include $(DEPENDS)
