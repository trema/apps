#
# Copyright (C) 2011-2013 NEC Corporation
#

TREMA = ../../trema

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -I../topology -std=gnu99 -D_GNU_SOURCE -g -Wall
LDFLAGS = $(shell $(TREMA_CONFIG) --libs) -L../topology -ltopology -lsqlite3

TARGET = redirectable_routing_switch
SRCS = authenticator.c fdb.c libpathresolver.c port.c redirectable_routing_switch.c redirector.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

run_acceptance_test:

-include $(DEPENDS)
