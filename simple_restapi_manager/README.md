Transaction manager
===================

What's this?
------------

Transaction manager is a library and provides a function to ensure any
controller-generated transaction (operation) has been executed on a
switch. Barrier request/reply messages are automatically sent by the
transaction manager to complete the transaction. When the transaction
has been completed successfully or unsuccessfully, the caller of the
function is notified.

How to use
----------

Please refer to transaction_manager.h and examples as references for
understanding how to use the APIs.

How to build
------------

  Get Trema and Apps

        $ sudo gem install trema
        $ git clone git://github.com/trema/apps.git apps

  Build transaction manager and its examples

        $ cd ./apps/transaction_manager
        $ make

How to run examples
-------------------

    $ trema run ./examples/example -c ./examples/example.conf

License
-------

Copyright (C) 2012-2013 NEC Corporation

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
