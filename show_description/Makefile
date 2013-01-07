#
# Copyright (C) 2012-2013 NEC Corporation
#

TREMA = ../../trema
GEM_BIN = `ruby -e 'require "rubygems"; puts Gem::bindir'`
SHELL = /bin/sh

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA_CONFIG) --libs)

TARGET = show_description
SRCS = show_description.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

show_description: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)
