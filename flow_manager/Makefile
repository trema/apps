#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../

CC = gcc
AR = ar
RANLIB = ranlib

CFLAGS = $(shell $(TREMA)/trema-config --cflags) -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA)/trema-config --libs)

TARGET_MANAGER = flow_manager
SRCS_MANAGER = flow_manager.c flow_manager_interface.c
OBJS_MANAGER = $(SRCS_MANAGER:.c=.o)

TARGET_LIB = libpath.a
SRCS_LIB = libpath.c flow_manager_interface.c
OBJS_LIB = $(SRCS_LIB:.c=.o)

TARGETS = $(TARGET_MANAGER) $(TARGET_LIB)
SRCS = $(SRCS_MANAGER) $(SRCS_LIB)
OBJS = $(OBJS_MANAGER) $(OBJS_LIB)

EXAMPLES = examples

DEPENDS = .depends

.PHONY: all depend clean examples

.SUFFIXES: .c .o

all: depend $(TARGETS) examples

$(TARGET_MANAGER): $(OBJS_MANAGER)
	$(CC) $(OBJS_MANAGER) $(LDFLAGS) -o $@

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) -cq $@ $(OBJS_LIB)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~
	cd $(EXAMPLES) && $(MAKE) clean

examples:
	cd $(EXAMPLES) && $(MAKE)

-include $(DEPENDS)
