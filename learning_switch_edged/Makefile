#
# Copyright (C) 2012-2013 NEC Corporation
#

TREMA = ../../trema-edge
SHELL = /bin/sh
TREMA_CONFIG = $(TREMA)/trema-config

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -std=gnu99 -D_GNU_SOURCE -g \
	-fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA_CONFIG) --libs)

TARGET = learning_switch
SRCS = learning_switch.c datapath_db.c fdb.c flow_entry.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all depend clean

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
