#
# Copyright (C) 2011-2013 NEC Corporation
#

TREMA = ../../trema

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -std=gnu99 -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA_CONFIG) --libs)

TARGET = simple_load_balancer
SRCS = simple_load_balancer.c
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
