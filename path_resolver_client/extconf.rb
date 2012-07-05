#
# Author: Yasuhito Takamiya <yasuhito@gmail.com>
#
# Copyright (C) 2008-2012 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#


require "mkmf"


$CFLAGS = "-g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith"
$LDFLAGS = "-Wl,-Bsymbolic"


dir_config "topology"
dir_config "routing"
dir_config "openflow"
dir_config "trema"


def error_exit message
  $stderr.puts message
  exit false
end


def error_lib_missing lib, package
  return <<-EOF
ERROR: #{ lib } not found!

Please install #{ package } with following command:
% sudo apt-get install #{ package }
EOF
end


unless find_library( "pthread", "pthread_create" )
  error_exit error_lib_missing( "libpthread", "libc6-dev" )
end


unless find_library( "rt", "clock_gettime" )
  error_exit error_lib_missing( "librt", "libc6-dev" )
end


depend_src = %w( libpathresolver path-resolver-client )
depend_c_src = depend_src.map { | basename | "#{ basename }.c" }
depend_objs = depend_src.map { | basename | "#{ basename }.o" }

$distcleanfiles << "$(SRCS)"
$srcs = depend_c_src
$objs = depend_objs


create_makefile "path_resolver_client"
