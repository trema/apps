#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../trema
GEM_BIN = /var/lib/gems/1.8/bin
SHEL = /bin/sh

FEATURES = learning_switch.feature

all:

depend:

clean:

run_acceptance_test:
	@for feature in $(FEATURES); do \
	  ( \
	    file=`pwd`/$$feature; \
	    cd ${TREMA}; \
	    ${GEM_BIN}/cucumber -r features $$file; \
	  ) \
	done
