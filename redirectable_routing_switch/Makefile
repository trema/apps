#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../trema

CC = gcc
CFLAGS = $(shell $(TREMA)/trema-config --cflags) -I$(TREMA)/src/examples/topology -std=gnu99 -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA)/trema-config --libs) -L$(TREMA)/objects/examples/topology -ltopology -lsqlite3

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

-include $(DEPENDS)
