Simple REST API manager
===================

What's this?
------------

Simple REST API manager is a library and provides the ability to add 
REST APIs in any Trema application that uses this library. This Simple 
REST API manager will start a mongoose web server and developers can 
add their callback functions mapping with URL. When the relation between 
URL and callback function is built, users or other applications can 
use REST APIs to operate the functions.

How to use
----------

Please refer to simple_restapi_manager.h and examples as references for
understanding how to use the APIs.

How to build
------------

  Get Trema and Apps

    $ sudo gem install trema
    $ git clone git://github.com/teyenliu/apps.git apps

  Build transaction manager and its examples

    $ cd ./apps/simple_restapi_manager
    $ make

How to run examples
-------------------

    $ trema run ./examples/example -c ./examples/example.conf
