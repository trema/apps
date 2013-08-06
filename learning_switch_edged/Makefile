#
# Copyright (C) 2012-2013 NEC Corporation
#

TREMA = ../../trema-edge
SHELL = /bin/sh
TREMA_CONFIG = $(TREMA)/trema-config

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -std=gnu99 -D_GNU_SOURCE -g -Wall
LDFLAGS = $(shell $(TREMA_CONFIG) --libs)

TARGET = learning_switch
SRCS = learning_switch.c fdb.c flow_entry.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

learning_switch: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)
