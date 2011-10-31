#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../trema
GEM_BIN = `ruby -e 'require "rubygems"; puts Gem::bindir'`
TREMA_APPS = ..

CC = gcc
CFLAGS = $(shell $(TREMA)/trema-config --cflags) -I$(TREMA_APPS)/topology -std=gnu99 -g -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA)/trema-config --libs) -L$(TREMA_APPS)/topology -ltopology

TARGET = sliceable_routing_switch
SRCS = fdb.c filter.c libpathresolver.c port.c sliceable_routing_switch.c slice.c redirector.c
OBJS = $(SRCS:.c=.o)

FEATURES = help.feature
FEATURES += port_binding.feature
FEATURES += mac_binding.feature mac_binding_loose_mode.feature
FEATURES += port_mac_binding.feature
FEATURES += port_mac_mixed_binding.feature
FEATURES += port_port_mac_mixed_binding.feature mac_port_mac_mixed_binding.feature
FEATURES += mac_binding_multiple_slices.feature port_binding_multiple_slices.feature

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

sliceable_routing_switch: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: 
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~
	@rm -rf checker.o checker

run_acceptance_test: $(FEATURES)

%.feature: sliceable_routing_switch
	( \
	  file=`pwd`/features/$@; \
	  cd ${TREMA}; \
	  sudo ${GEM_BIN}/cucumber -r features -r ../apps/sliceable_routing_switch/features $$file || exit 1 \
	)

checker.feature: checker

checker: checker.o
	$(CC) $< $(LDFLAGS) -o $@


-include $(DEPENDS)
