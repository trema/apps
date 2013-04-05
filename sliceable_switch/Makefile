#
# Copyright (C) 2011-2013 NEC Corporation
#

TREMA = ../../trema
GEM_BIN = `ruby -e 'require "rubygems"; puts Gem::bindir'`
TREMA_APPS = ..

TREMA_CONFIG = $(shell which trema-config)
ifeq ($(TREMA_CONFIG),)
  TREMA_CONFIG = $(TREMA)/trema-config
endif

CC = gcc
CFLAGS = $(shell $(TREMA_CONFIG) --cflags) -I$(TREMA_APPS)/topology -I$(TREMA_APPS)/flow_manager -std=gnu99 -g -D_GNU_SOURCE -Wall
LDFLAGS = $(shell $(TREMA_CONFIG) --libs) -L$(TREMA_APPS)/topology -ltopology -L$(TREMA_APPS)/flow_manager -lpath

TARGET = sliceable_switch
SRCS = fdb.c filter.c libpathresolver.c port.c sliceable_switch.c slice.c redirector.c
OBJS = $(SRCS:.c=.o)

FEATURES = help.feature
FEATURES += port_binding.feature
FEATURES += mac_binding.feature mac_binding_loose_mode.feature
FEATURES += port_mac_binding.feature
FEATURES += port_mac_mixed_binding.feature
FEATURES += port_port_mac_mixed_binding.feature mac_port_mac_mixed_binding.feature
FEATURES += mac_binding_multiple_slices.feature port_binding_multiple_slices.feature
FEATURES += mac_binding_65535_slices.feature
FEATURES += switch_restart.feature
FEATURES += set_reverse_flow.feature filter.feature
FEATURES += rest_if_networks.feature

LOGGING_LEVEL=info

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
	@rm -rf checker.o checker

test: $(FEATURES)

%.feature: sliceable_switch
	( \
	  file=`pwd`/features/$@; \
	  cd ${TREMA}; \
	  sudo LOGGING_LEVEL=${LOGGING_LEVEL} ${GEM_BIN}/cucumber -r ../apps/sliceable_switch/features $$file || exit 1 \
	)

checker.feature: checker

checker: checker.o
	$(CC) $< $(LDFLAGS) -o $@


-include $(DEPENDS)
