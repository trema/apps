#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../

CC = gcc
CFLAGS = $(shell $(TREMA)/trema-config --cflags) -std=gnu99 -D_GNU_SOURCE -g -Wall
LDFLAGS = $(shell $(TREMA)/trema-config --libs)

TARGET = flow_dumper
SRCS = flow_dumper.c
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
