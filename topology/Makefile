#
# Copyright (C) 2011-2013 NEC Corporation
#

TREMA = ../../trema
GEM_BIN = `ruby -e 'require "rubygems"; puts Gem::bindir'`
SHELL = /bin/sh

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
AR = ar
RANLIB = ranlib

CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA_CONFIG) --libs)

TARGET_LIB = libtopology.a
SRCS_LIB = libtopology.c topology_service_interface_option_parser.c
OBJS_LIB = $(SRCS_LIB:.c=.o)

TARGET_TOPOLOGY = topology
SRCS_TOPOLOGY = service_management.c subscriber_table.c topology.c topology_management.c topology_table.c
OBJS_TOPOLOGY = $(SRCS_TOPOLOGY:.c=.o)

TARGET_DISCOVERY = topology_discovery
SRCS_DISCOVERY = lldp.c probe_timer_table.c topology_discovery.c
OBJS_DISCOVERY = $(SRCS_DISCOVERY:.c=.o)
LDFLAGS_DISCOVERY = $(LDFLAGS) -L. -ltopology

TARGET_SHOW = show_topology
SRCS_SHOW = show_topology.c show_topology_dsl.c show_topology_graph_easy.c show_topology_csv.c
OBJS_SHOW = $(SRCS_SHOW:.c=.o)
LDFLAGS_SHOW = $(LDFLAGS) -L. -ltopology

TARGETS = $(TARGET_LIB) $(TARGET_TOPOLOGY) $(TARGET_DISCOVERY) $(TARGET_SHOW)
SRCS = $(SRCS_LIB) $(SRCS_TOPOLOGY) $(SRCS_DISCOVERY) $(SRCS_SHOW)
OBJS = $(OBJS_LIB) $(OBJS_TOPOLOGY) $(OBJS_DISCOVERY) $(OBJS_SHOW)

FEATURES = topology.feature

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) -cq $@ $(OBJS_LIB)
	$(RANLIB) $@

$(TARGET_TOPOLOGY): $(OBJS_TOPOLOGY)
	$(CC) $(OBJS_TOPOLOGY) $(LDFLAGS) -o $@

$(TARGET_DISCOVERY): $(OBJS_DISCOVERY)
	$(CC) $(OBJS_DISCOVERY) $(LDFLAGS_DISCOVERY) -o $@

$(TARGET_SHOW): $(OBJS_SHOW)
	$(CC) $(OBJS_SHOW) $(LDFLAGS_SHOW) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

run_acceptance_test:
	@for feature in $(FEATURES); do \
	  ( \
	    file=`pwd`/$$feature; \
	    cd ${TREMA}; \
	    ${GEM_BIN}/cucumber -r features $$file; \
	  ) \
	done

-include $(DEPENDS)
