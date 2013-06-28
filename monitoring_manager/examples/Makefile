#
# Copyright (C) 2013 TeYen Liu
#

TREMA = ../../../trema

CC = gcc
AR = ar
RANLIB = ranlib

CFLAGS = $(shell $(TREMA)/trema-config --cflags) -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -I. -I..
LDFLAGS = $(shell $(TREMA)/trema-config --libs) -L.. -lmonitoring

SRCS_COMMON = 
OBJS_COMMON = $(SRCS_COMMON:.c=.o)

TARGET_EXAMPLE1 = example1
SRCS_EXAMPLE1 = example1.c
OBJS_EXAMPLE1 = $(SRCS_EXAMPLE1:.c=.o)

TARGETS = $(TARGET_EXAMPLE1) $(TARGET_EXAMPLE2) $(TARGET_EXAMPLE3) $(TARGET_EXAMPLE4) $(TARGET_EXAMPLE5)
SRCS = $(SRCS_COMMON) $(SRCS_EXAMPLE1) $(SRCS_EXAMPLE2) $(SRCS_EXAMPLE3) $(SRCS_EXAMPLE4) $(SRCS_EXAMPLE5)
OBJS = $(OBJS_COMMON) $(OBJS_EXAMPLE1) $(OBJS_EXAMPLE2) $(OBJS_EXAMPLE3) $(OBJS_EXAMPLE4) $(OBJS_EXAMPLE5)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_EXAMPLE1): $(OBJS_EXAMPLE1) $(OBJS_COMMON)
	$(CC) $(OBJS_EXAMPLE1) $(OBJS_COMMON) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
