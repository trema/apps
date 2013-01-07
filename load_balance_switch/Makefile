#
# Copyright (C) 2011-2013 NEC Corporation
#

TREMA = ../../trema

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -I../topology -I../path_manager -std=gnu99 -D_GNU_SOURCE -Wall -g
LDFLAGS = $(shell $(TREMA_CONFIG) --libs) -L../topology -ltopology

TARGET = switch
SRCS = fdb.c path_resolver.c port.c switch.c
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

-include $(DEPENDS)
