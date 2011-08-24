Multi learning switch with memcached
====================================

This directory includes an OpenFlow controller that emulates multiple
layer 2 switches.
Use memcached to forwarding database,
otherwise same as `trema/src/src/examples/multi_learning_switch`(Ruby version).

Requirements
------------

  - Memcached <http://memcached.org/>
  - Ruby memcache-client <http://seattlerb.rubyforge.org/memcache-client/>

  if you are using Ubuntu linux. you can install them as follows:

  Memcached

        $ sudo apt-get install memcached

  Ruby memcache-client

        $ sudo gem install memcache-client

How to build
------------

  Get Trema and Apps

        $ git clone git://github.com/trema/trema.git trema
        $ git clone git://github.com/trema/apps.git apps

  Build Trema

        $ cd trema
        $ ./build.rb
        $ cd ..

How to run
----------

  Memcached

        $ sudo /etc/init.d/memcached start

  Multi learning switch

        $ cd trema
        $ sudo ./trema run -c ../apps/multi_learning_switch_memcached/multi-learning-switch.rb

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


### Terms

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
