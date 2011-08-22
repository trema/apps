#
# Forwarding database (FDB)
#
# Author: Yasuhito Takamiya <yasuhito@gmail.com>
#
# Copyright (C) 2008-2011 NEC Corporation
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

#
# Forwarding database with memcached
#
# Author: Kazushi SUGYO
#


require 'rubygems'
require 'memcache'


#
# A database that keep pairs of MAC address and port number
#
class ForwardingDB
  DEFAULT_AGE_MAX = 300


  def initialize namespace = nil
    @db = MemCache.new( '127.0.0.1:11211', { :namespace => namespace, :namespace_separator => '-' } )
  end


  def port_no_of mac
    @db[ mac.to_s ]
  end


  def learn mac, port_no
    @db.set mac.to_s, port_no, DEFAULT_AGE_MAX
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
