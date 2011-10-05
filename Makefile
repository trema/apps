#
# Copyright (C) 2011 NEC Corporation
#

SHELL = /bin/sh

SUBDIRS = topology
SUBDIRS += redirectable_routing_switch simple_load_balancer
SUBDRIS += learning_switch_memcached multi_learning_switch_memcached
SUBDIRS += routing_switch

all clean run_acceptance_test:
	@for dir in $(SUBDIRS); do \
		make -C $$dir $@; \
	done

