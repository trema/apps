Flow manager
============

**!!!! NOTICE !!!!**
**THIS APPLICATION IS STILL UNDER DEVELOPMENT AND MAY NOT WORK EXPECTEDLY.**

What's this?
------------

Flow manager and path management library (libpath) provide high-level
APIs on the raw OpenFlow protocol layer for managing multiple flow
entries as a single set. If you want to install flow entries across
multiple switches, flow manager guarantees that all entries are
properly installed into the switches. If any error occurs, all entries
associated with the set are removed from the switches. If all entries
are removed from the switches, a requester is notified from flow manager.

Software architecture/hierarchy is as follows:

                +--------------+ +-------------+
                |     App      | |     App     |
                +--------------+ +-------------+
                |    libpath   | | New library |
                +--------------+ +-------------+
                       |                |  Flow Manager Interface
      +-----+   +------------------------------+
      | App |   |         Flow Manager         |
      +-----+   +------------------------------+
         |                      |  OpenFlow Application Interface
      +----------------------------------------+
      |                  Trema (core)          |
      +----------------------------------------+

libpath which cooperates with flow manager allows you to set up/tear
down/look up a path which is a set of flow entries that share the same
packet match criteria easily.

Flow Manager only provides primitive functions that treat a set of
flow entries as a single group. Other libraries (e.g., libtree)
may be developed on top of Flow Manager Interface in a manner similar
to libpath.

How to use
----------

Please refer to libpath.h and examples as references for understanding
how to use the APIs.

How to build
------------

  Build Trema and slicesable routing switch

        $ git clone git://github.com/trema/trema.git trema
        $ git clone git://github.com/trema/apps.git apps
        $ cd trema
        $ ./build.rb
        $ cd ../apps/flow_manager

  Build flow manager, libpath, and examples

        $ make

How to run examples
-------------------

    $ ./trema run -c ../apps/flow_manager/flow_manager.conf -d
    $ export TREMA_HOME=`pwd`
    $ cd ../apps/flow_manager/examples
    $ ./example1
    $ ./example2
    $ ./example3
    $ ./example4
    $ ./example5

All examples show how to setup or teardown paths through libpath.

License & Terms
---------------

Copyright (C) 2011 NEC Corporation

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.


## Terms

Terms of Contributing to Trema program ("Program")

Please read the following terms before you submit to the Trema project
("Project") any original works of corrections, modifications,
additions, patches and so forth to the Program ("Contribution"). By
submitting the Contribution, you are agreeing to be bound by the
following terms.  If you do not or cannot agree to any of the terms,
please do not submit the Contribution:

1. You hereby grant to any person or entity receiving or distributing
   the Program through the Project a worldwide, perpetual,
   non-exclusive, royalty free license to use, reproduce, modify,
   prepare derivative works of, display, perform, sublicense, and
   distribute the Contribution and such derivative works.

2. You warrant that you have all rights necessary to submit the
   Contribution and, to the best of your knowledge, the Contribution
   does not infringe copyright, patent, trademark, trade secret, or
   other intellectual property rights of any third parties.

3. In the event that the Contribution is combined with third parties'
   programs, you notify to Project maintainers including NEC
   Corporation ("Maintainers") the author, the license condition, and
   the name of such third parties' programs.

4. In the event that the Maintainers incorporates the Contribution
   into the Program, your name will not be indicated in the copyright
   notice of the Program, but will be indicated in the contributor
   list on the Project website.
