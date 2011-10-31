Sliceable routing switch
========================

**!!!! NOTICE !!!!**
**THIS APPLICATION IS STILL UNDER DEVELOPMENT AND MAY NOT WORK EXPECTEDLY.**

What's this?
------------

This is a routing switch application with slicing and packet filter
function. The slicing function allows us to create multiple layer 2
domains (similar to port and MAC-based VLAN but there is no limitation
on VLAN ID space). The filter function implements an access control
list.

How to build
------------

  Build Trema and slicesable routing switch

        $ git clone git://github.com/trema/trema.git trema
        $ git clone git://github.com/trema/apps.git apps
        $ cd trema
        $ ./build.rb
        $ cd ../apps/sliceable_routing_switch

  Build sliceable routing switch and setup databases

        $ make
        $ sudo apt-get install sqlite3 libdbi-perl libdbd-sqlite3-perl
        $ ./create_tables.sh

How to configure
----------------

        Add packet filter definitions
        (See 'filter' command for details)

        Add slice definitions
        (See 'slice' command for details)

How to run
----------

        $ cd ../../trema
        $ sudo ./trema run -c ../apps/sliceable_routing_switch/sliceable_routing_switch.conf

How to enable REST-based configuration interface
------------------------------------------------

This application has a REST-based interface for configuring slice
and packet filter definitions from external application. The following
instruction shows how to configure the interface.

Note that the following instruction is only valid for Ubuntu Linux
or Debian GNU/Linux.

  Install required software packages

        $ sudo apt-get install apache2-mpm-prefork libjson-perl

  Create configuration database (if you have not created yet)

        $ ./create_tables.sh

  Configure Apache web server

        $ sudo cp apache/sliceable_routing_switch /etc/apache2/sites-available
        $ sudo a2enmod rewrite actions
        $ sudo a2ensite sliceable_routing_switch
        $ sudo mkdir -p /home/sliceable_routing_switch/script
        $ sudo mkdir /home/sliceable_routing_switch/db
        $ sudo cp Slice.pm Filter.pm config.cgi /home/sliceable_routing_switch/script
        $ sudo cp *.db /home/sliceable_routing_switch/db
        $ sudo chown -R www-data.www-data /home/sliceable_routing_switch
        $ sudo /etc/init.d/apache2 reload

License & Terms
---------------

Copyright (C) 2008-2011 NEC Corporation

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
