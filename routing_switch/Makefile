#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../trema

CC = gcc
CFLAGS = $(shell $(TREMA)/trema-config --cflags) -I../topology -std=gnu99 -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA)/trema-config --libs) -L../topology -ltopology

TARGET = routing_switch
SRCS = fdb.c libpathresolver.c port.c routing_switch.c
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
